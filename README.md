好的，我完全明白您的需求。这是一个非常专业且有挑战性的任务，目标是创建一个高质量、零依赖、无状态的嵌入式协议库。这要求我们不仅要实现协议的核心功能，更要深入思考其架构的灵活性、可移植性和完整性。

我将从核心设计哲学、统一架构、各协议广度设计和API范例四个方面，为您阐述我的深度思考和设计方案。

```
https://github.com/cautonwong/libedge-proto
```
---

### 1. 核心设计哲学 (Core Design Philosophy)

我们的设计将严格遵循以下三个核心原则，以满足您提出的严苛要求：

1.  **绝对无状态 (Stateless by Design)**
    *   **库本身不存储任何会话信息。** 所有的协议状态，例如DLMS的会话状态、IEC104的收发序列号(V(S), V(R))、DNP3的链路层状态等，都必须由库的使用者（上层应用）来维护。
    *   **通过上下文（`Context`）指针实现。** 库定义的每一个`XXX_context_t`结构体，都包含了该协议运行所需的所有状态变量。应用在调用库函数时，必须传入这个上下文结构体的指针。库函数只对这个`context`进行读写操作，函数返回后，状态的变更保留在`context`中，而库本身恢复“洁净”。
    *   **优点：** 这种设计天然支持多线程/多任务（每个任务管理自己的`context`）、易于在各种RTOS或裸机环境中集成、便于应用的故障恢复和状态持久化。

2.  **零依赖与IO解耦 (Zero-Dependency & I/O Decoupling)**
    *   **仅依赖C标准库和`iovec`**。不引入任何第三方库，不假设任何操作系统存在（如`pthreads`），不直接调用`socket`、`read`、`write`等IO函数。
    *   **库的职责是“协议数据的编解码”，而非“数据收发”。**
        *   **发送时：** 应用调用库函数，如`edge_modbus_build_read_req(...)`，库函数会将组装好的协议报文填充到一个或多个由应用传入的`iovec`结构体中。应用拿到`iovec`数组后，自行决定如何（`writev`, `sendmsg`）以及何时发送出去。
        *   **接收时：** 应用通过自己的IO机制（如`read`, `select`, `epoll`）收到数据后，将原始数据`buffer`和长度`len`喂给库的解析函数，如`edge_dlt698_parse_frame(...)`。
    *   **优点：** 极高的可移植性，可以运行在任何具备C编译器和标准库的环境中。应用可以自由选择IO模型（阻塞、非阻塞、异步IO如`io_uring`）。

3.  **统一接口与协议特异性并存 (Unified Interface with Protocol Specificity)**
    *   提供一个非常薄的通用层，定义通用的返回码、日志接口（宏实现，可重定向）等。
    *   每个协议都有其独立的头文件（如`edge_dlms.h`）和API，充分暴露该协议的全部功能和特性。我们不试图用一个“万能”的`read()`函数去抽象所有协议，因为这会隐藏掉各协议的丰富细节，违背了“广度”的设计初衷。

---

### 2. 统一架构设计 (Unified Architecture)

#### 2.1 上下文（Context）结构

应用层负责定义和实例化`context`。库提供`struct`定义。

```c
// 在 edge_common.h 中
typedef struct {
    void* user_data; // 回指到应用层自己的上层结构
    // ... 其他通用状态, 如最近的错误码等
} edge_base_context_t;

// 在 edge_dnp3.h 中
typedef struct {
    // DNP3 链路层状态
    bool is_master;
    uint8_t link_address[2];
    // ...

    // DNP3 传输层状态
    uint8_t transport_seq;

    // DNP3 应用层状态
    uint8_t app_seq;
    // ...
} edge_dnp3_fragments_ctx_t;

typedef struct {
    edge_base_context_t base;
    edge_dnp3_fragments_ctx_t dnp3;
} edge_dnp3_context_t;

// 应用层代码
typedef struct {
    int socket_fd;
    edge_dnp3_context_t proto_ctx;
    // ... 其他应用状态
} my_dnp3_device_t;

my_dnp3_device_t device;
// 初始化 device.proto_ctx ...
```

#### 2.2 数据流模型

**发送流程 (Encoding):**
1.  **应用:** 决定要发送一个“DNP3读I类事件”请求。
2.  **应用:** 调用 `edge_dnp3_build_class_poll_req(&device.proto_ctx, DNP3_CLASS_1, iovec_array, &iovec_count);`
3.  **库:** 在`edge_dnp3_build_class_poll_req`内部：
    a. 构建应用层APDU。
    b. 打包成传输层TPDU。
    c. 打包成链路层LPDU，计算CRC。
    d. 将最终的报文数据指针和长度填入`iovec_array`。
4.  **应用:** 拿到`iovec_array`和`iovec_count`后，调用`writev(device.socket_fd, iovec_array, iovec_count);`发送数据。

**接收流程 (Decoding):**
1.  **应用:** `n = read(device.socket_fd, buffer, sizeof(buffer));`
2.  **应用:** 调用 `edge_dnp3_parse_frame(&device.proto_ctx, buffer, n, &callbacks);`
3.  **库:** 在`edge_dnp3_parse_frame`内部：
    a. 校验链路层头部、地址、CRC，处理链路层确认（并更新`proto_ctx`中的状态）。
    b. 解开传输层，处理分包（更新`proto_ctx`中的重组缓冲区）。
    c. 当收到完整APDU时，解析应用层报文。
    d. 根据报文内容，调用`callbacks`结构中对应的回调函数，如`callbacks.on_analog_input(&device.proto_ctx, point_idx, value, flags);`
4.  **应用:** 在应用定义的回调函数`on_analog_input`中，处理获取到的数据。

---

### 3. 各协议广度设计 (Protocol Breadth Design)

这是方案的重点，特别强调覆盖每个协议的所有Features。

*   **主从实现 (Master/Slave Implementation):** 对于所有具有主从（或客户端/服务器）角色的协议（如Modbus, DNP3, IEC101/104），库将同时提供主站（Master/Client）和从站（Slave/Server）的实现。

#### Modbus
*   **物理层支持:** 逻辑上支持RTU, ASCII, TCP。CRC16和LRC计算都会提供。
*   **角色:** Master 和 Slave 的实现都将提供。
*   **功能码:** 
    *   **Public:** 完整覆盖`0x01` - `0x17`的所有标准功能码，包括读写线圈、寄存器，以及`0x17`（报告服务器ID）。
    *   **User-Defined:** 预留`0x41` - `0x48`和`0x64` - `0x6E`的用户自定义功能码接口。
    *   **异常响应:** 对所有无效请求（如非法功能码、非法地址、非法数据值）都能生成正确的异常响应。

#### DL/T 645 (以2007版为核心)
*   **帧结构:** 完整支持前导符、`68h`、地址域、控制码、数据域长度、数据域、校验和、`16h`。
*   **控制码:** 支持所有主站请求和子站应答的控制码，包括广播、读、写、密码认证、修改速率等。
*   **数据标识符(DI):** 
    *   内置GB/T 19582中所有常用数据标识符的解析。
    *   提供接口让用户可以自定义或扩展非标的数据标识符。
*   **数据格式:** 支持所有数据格式的解析和构建，如BCD码、ASCII码、浮点数、日期时间等。
*   **流程:** 支持完整的读数据、写数据、广播校时、下发参数等全业务流程。

#### IEC 60870-5-101/102/104
*   **ASDU (应用服务数据单元):** 
    *   **类型标识(Type ID):** 覆盖所有标准定义的类型，包括单点/双点信息、步位置信息、测量值（归一化、标度化、短浮点）、累计量、带时标/不带时标的各种变体。
    *   **传送原因(COT):** 全覆盖，包括突发、背景扫描、响应站召唤、激活、命中断等，并能正确处理其组合。
    *   **公共地址(COA):** 支持1字节和2字节。
*   **101 特性:** 
    *   **链路层:** FT1.2帧格式，PRM=1/0，FCB/FCV位的正确处理。
    *   **模式:** 支持非平衡模式（轮询）和平衡模式（主/从站均可发起）。`context`中将包含完整的链路层状态机。
*   **104 特性:** 
    *   **APCI:** 支持I帧、S帧、U帧的全功能，包括`STARTDT`、`STOPDT`、`TESTFR`。
    *   **会话管理:** `context`中将包含`t0`-`t3`超时计时器的状态（库只更新状态，应用负责实现物理定时器），以及收发序列号`V(S)/V(R)`。
    *   **业务流程:** 支持总召唤、分组召唤、时钟同步、遥控（选择前执行/直接执行）、遥调等所有标准流程。

#### DNP3
*   **分层实现:** 库内部逻辑严格按照Link, Transport, Application三层来组织。
*   **对象库(Object Library):** 
    *   **全覆盖:** 实现所有标准的对象组(Group)和变位(Variation)，从G1V1（二进制输入）到G122V2（安全统计）。
    *   **数据解析:** 正确解析各种数据类型，包括二进制、计数器、模拟量（16/32位整型，32/64位浮点）、CROB、Analog Output等。
    *   **静态/事件:** 正确区分和处理静态数据（Static）和事件数据（Event）。
*   **功能码(Function Code):** 支持所有功能码，包括确认、读、写、选择/操作/直接操作、冻结、分配类等。
*   **核心特性:** 
    *   **unsolicited 响应 (RBE):** 支持基于事件的非请求响应。
    *   **分级扫描:** 支持Class 0/1/2/3的数据分级轮询。
    *   **时钟同步:** 支持时钟同步流程。
    *   **文件传输:** `context`中会包含文件传输所需的状态机和序列号，支持目录读取、文件读取等。

#### DLMS/COSEM (最复杂的部分)
*   **APDU层:** 
    *   **xDLMS PDU:** 完整支持`get-request`, `set-request`, `action-request`, `data-notification`等，并支持其所有形式 (`normal`, `next`, `with-list`, `with-block`)。
    *   **Ciphered APDU:** 支持加密APDU的构建和解析。
*   **COSEM对象模型:** 
    *   **接口类(IC):** 提供对大量常用IC的完整实现，包括：IC1(Data), IC3(Register), IC4(Extended Register), IC7(Profile generic), IC8(Clock), IC15(Association LN), IC17(SAP Assignment), IC64(Security setup), IC70(Disconnect control), IC71(Limiter)等。
    *   **OBIS码:** 提供强大的OBIS码解析、匹配和生成工具。
*   **数据访问:** 支持按属性(attribute)和方法(method)访问，支持选择性访问（`selective-access`）。
*   **数据传输:** **Block Transfer**机制是必须的，`context`中会保存块号、状态等，以处理大数据对象（如曲线文件）的传输。
*   **安全性:** 
    *   **HLS (High-Level Security):** 支持`authentication-only`和`authentication-and-encryption`。
    *   **安全套件 (Security Suite):** 支持Suite 0, 1, 2，即AES-128-GCM。
    *   **密钥管理:** `context`中将保存系统标题(System Title)、帧计数器(Invocation Counter)、全局密钥等安全相关的所有状态。库将提供`GMAC`计算和加解密的核心算法。**注意：** 加密原语（如AES）本身可以封装成一个可替换的模块，以便应用层对接硬件加密引擎。
    *   支持完整的认证流程，从AARQ到应用上下文建立。

#### DL/T 698
*   **对象模型:** 内置一个全面的698对象字典，包括类ID、属性和方法。
*   **APDU服务:** 支持`GET`, `SET`, `ACTION`, `REPORT`, `PROXY`等所有应用服务单元。支持其`Request`/`Response`的`Normal`, `List`, `Next`等变体。
*   **数据类型(Data):** 提供一个强大的类型系统，能够编解码所有698定义的数据类型，包括`array`, `structure`, `choice`, `date_time_s`等复杂类型。
*   **安全性:** 支持标准定义的安全机制，包括身份认证、密钥协商和更新、APDU加密和签名。`context`将存储相关的安全状态。
*   **链路层:** 支持698.31定义的网络层和链路层封装（如果需要）。但核心逻辑与物理层无关。

---

### 4. 目录结构与构建 (Directory Structure & Build)

*   **构建系统 (Build System):** 项目将使用 `CMake` 作为统一的构建系统。

#### 目录结构

```
/libedge-proto
|-- /include
|   |-- /libedge
|   |   |-- edge_common.h      // 通用类型, 错误码
|   |   |-- edge_modbus.h
|   |   |-- edge_dlt645.h
|   |   |-- edge_dlt698.h
|   |   |-- edge_iec104.h
|   |   |-- edge_dnp3.h
|   |   `-- edge_dlms.h
|-- /src
|   |-- common
|   |   |-- crc.c              // 各种CRC算法
|   |   `-- aes_gcm.c          // AES-GCM实现
|   |-- modbus
|   |   |-- modbus_codec.c
|   ... (以此类推)
|-- /tests
|   |-- test_modbus.c
|   `-- test_dnp3.c
`-- CMakeLists.txt
```

#### API使用范例 (以Modbus TCP Master读保持寄存器为例)

```c
#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/uio.h>
#include "libedge/edge_modbus.h"

// 应用定义的回调
void on_read_holding_registers(edge_modbus_context_t* ctx, uint16_t* regs, uint8_t count) {
    printf("Received %d registers:\n", count);
    for (int i = 0; i < count; ++i) {
        printf("  Reg %d: %u\n", i, regs[i]);
    }
}

int main() {
    // 1. 初始化应用和上下文
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    // ... connect to modbus slave ...

    edge_modbus_context_t modbus_ctx;
    edge_modbus_init_context(&modbus_ctx, 1 /* slave_id */, true /* is_tcp */);

    // 2. 构建请求
    struct iovec iov[EDGE_MODBUS_MAX_IOVEC]; // 库定义宏
    int iov_count = 0;
    
    // API调用，请求从地址100开始的5个寄存器
    edge_modbus_build_read_holding_req(&modbus_ctx, 100, 5, iov, &iov_count);

    // 3. 应用负责发送
    printf("Sending Modbus TCP request...\n");
    writev(sock_fd, iov, iov_count);

    // 4. 应用负责接收
    uint8_t rx_buffer[260];
    int n = read(sock_fd, rx_buffer, sizeof(rx_buffer));

    // 5. 将收到的数据喂给库进行解析
    if (n > 0) {
        edge_modbus_callbacks_t cbs = { .on_read_holding_registers = on_read_holding_registers };
        edge_modbus_parse_frame(&modbus_ctx, rx_buffer, n, &cbs);
    }

    close(sock_fd);
    return 0;
}

```

这个范例清晰地展示了“无状态”和“IO解耦”的设计哲学：
*   `modbus_ctx` 存储了会话信息（如TCP事务ID），由`main`函数管理。
*   库函数 `edge_modbus_build...` 只负责填写`iovec`。
*   应用 `main` 函数负责 `writev` 和 `read`。
*   库函数 `edge_modbus_parse...` 负责解析，并通过回调通知应用。

---

### 5. 测试策略 (Testing Strategy)

*   **测试框架 (Testing Framework):** 所有单元测试和集成测试将使用 `cmocka` 框架编写。`cmocka` 提供了强大的断言、mocking和stubbing功能，非常适合对C代码进行白盒测试。
*   **代码覆盖 (Code Coverage):** 通过 `cmocka` 和 `gcov`/`lcov` 工具链，我们将持续追踪代码覆盖率，确保核心逻辑得到充分测试。
*   **测试目录 (Tests Directory):** 所有的测试用例将存放在项目根目录下的 `/tests` 目录中，并按照协议模块进行组织。

---

此设计方案兼顾了您对零依赖、无状态的严苛要求，以及对协议广度全面覆盖的期望。它具备极高的灵活性和可维护性，是构建一个工业级、可长期演进的协议库的坚实基础。

如果您对某个协议的特定细节或架构的某一部分有更深入的疑问，我们可以继续探讨。

```
