# `libedge-proto` 项目执行计划 (Execution Plan)

本项目旨在创建一个零依赖、无状态、跨平台的工业与电力通信协议库。本计划将整个开发过程分解为七个主要阶段，确保高质量、可追踪的交付。

## Phase 0: 项目基础与核心架构 (Project Foundation & Core Infrastructure)

**目标:** 搭建项目的骨架，包括构建系统、测试环境和通用代码。

1.  **版本控制:** 初始化 Git 仓库，并创建 `main` 和 `develop` 分支。
2.  **构建系统 (`CMake`):**
    *   创建根 `CMakeLists.txt`。
    *   配置项目基本信息，设置 C 标准 (e.g., C99/C11)。
    *   添加一个 `ENABLE_<PROTOCOL_NAME>` 格式的 `option` 来允许用户选择性地编译每个协议模块。
    *   通过 `FetchContent` 或 `add_subdirectory` 的方式集成 `cmocka` 测试框架。
    *   配置 `CTest`，创建测试目标，以便运行 `make test` 或 `ctest`。
3.  **目录结构:** 创建规范的目录结构：
    *   `/include/libedge/`: 存放所有公共头文件。
    *   `/src/`: 存放所有源文件，并为每个协议创建子目录。
    *   `/src/common/`: 存放如CRC、AES等通用算法。
    *   `/tests/`: 存放所有测试代码。
4.  **核心通用代码 (`edge_common.h`):**
    *   定义通用的错误码 `edge_error_t`。
    *   定义基础上下文结构体 `edge_base_context_t`。
    *   提供可重定向的日志宏 (`EDGE_LOG_DEBUG`, `EDGE_LOG_ERROR` 等)。
5.  **通用工具函数:**
    *   在 `/src/common/` 中实现各种协议需要的CRC算法 (CRC16-Modbus, CRC16-CCITT, DNP3 CRC等)。
    *   编写并运行针对这些CRC算法的单元测试，确保测试框架（`cmocka`）和构建系统工作正常。

## Phase 1: Modbus 协议实现

**目标:** 实现最简单的协议，以此验证整个设计（无状态、IO解耦）和工作流（编码->测试）的正确性。

1.  **API定义 (`edge_modbus.h`):**
    *   定义 `edge_modbus_context_t`，包含TCP事务ID、从站地址等。
    *   定义应用层回调结构体 `edge_modbus_callbacks_t`。
    *   声明API函数，如 `edge_modbus_build_read_holding_req`, `edge_modbus_parse_frame`。
2.  **编码 (Master & Slave):**
    *   实现所有标准功能码的请求构建（Master）和解析（Slave）。
    *   实现响应构建（Slave）和解析（Master）。
    *   支持TCP和RTU两种模式的编解码逻辑。
3.  **测试 (`tests/test_modbus.c`):**
    *   为每个功能码编写单元测试，覆盖请求和响应的编解码。
    *   测试边界条件，如最大/最小寄存器数量。
    *   测试异常响应的生成和解析。
    *   分别测试Master和Slave角色的逻辑。

## Phase 2: DL/T 645 和 DL/T 698 协议实现

**目标:** 实现两个关键的国产计量协议。

1.  **DL/T 645-2007:**
    *   **API & 编码:** 实现 `edge_dlt645.h` 和对应的源文件。重点是68h...16h帧的封装/解封装、主/从站控制码处理、DI标识符解析以及BCD等数据格式的转换。
    *   **测试:** 编写测试用例，覆盖帧校验、地址匹配、不同DI和数据格式的解析。
2.  **DL/T 698:** (复杂度较高)
    *   **核心模块:** 优先实现并充分测试其强大的 `Data` 数据类型编解码器，这是698的基础。
    *   **API & 编码:** 实现APDU服务的编解码 (`GET`, `SET`, `ACTION`, `REPORT` 等)，同时支持Client和Server角色。
    *   **安全性:** 实现安全相关的APDU和流程，`context`中包含密钥和安全计数器。
    *   **测试:** 为 `Data` 编解码器、每种APDU服务以及安全认证流程编写详尽的测试用例。

## Phase 3: IEC 60870-5 系列协议实现

**目标:** 实现国际上广泛应用的电力系统核心协议。

1.  **IEC 104 (优先):**
    *   **API & 编码:** 实现 `edge_iec104.h`。核心是APCI（I/S/U帧）的逻辑处理，以及`context`中收发序列号`V(S)/V(R)`的正确管理。
    *   **ASDU:** 实现一个通用的ASDU编解码模块，覆盖所有常用类型标识和传送原因。
    *   **角色:** 实现主站（总召唤、控制）和从站（响应、上报）的核心流程。
    *   **测试:** 重点测试序列号的同步与翻转、不同ASDU的解析以及会话建立/拆除流程。
2.  **IEC 101:**
    *   **编码:** 复用IEC 104的ASDU模块。重点实现FT1.2帧格式和链路层状态机（用于平衡/非平衡模式）。
    *   **测试:** 测试链路层的请求、确认、数据传输等流程。
3.  **IEC 102:**
    *   根据其特定规范，实现其独特的帧格式和数据结构。测试其编解码的正确性。

## Phase 4: DNP3 协议实现

**目标:** 攻克分层结构复杂、对象库庞大的DNP3协议。

1.  **分层编码:**
    *   **链路层:** 优先实现并测试链路层的帧同步、CRC、地址校验、主从站通信等。
    *   **传输层:** 实现并测试报文的分包与重组。
    *   **应用层:** 实现功能码的解析，并着手构建庞大的对象库，逐个实现常用对象组/变位的编解码。
2.  **角色 (Master & Outstation):**
    *   实现Master的请求构建（特别是带功能码的IIN位查询）。
    *   实现Outstation的响应生成，包括事件数据的封装（unsolicited response）。
3.  **测试 (`tests/test_dnp3.c`):**
    *   为每一层编写独立的测试用例。
    *   集成测试，模拟一个完整的请求-响应周期，覆盖分包和事件上报场景。

## Phase 5: DLMS/COSEM 协议实现

**目标:** 实现最复杂、功能最丰富的DLMS协议，这是项目的难点和重点。

1.  **基础模块:**
    *   **APDU层:** 实现所有xDLMS PDU的编解码，并充分测试。
    *   **COSEM对象模型:** 设计通用的COSEM对象表示方法。
2.  **核心功能:**
    *   **接口类(IC):** 逐个实现关键的IC，如`Clock`, `Register`, `Profile generic`, `Association LN`, `Security setup`。
    *   **安全性:** 实现HLS（高级安全）认证流程，包括密钥派生、GMAC计算、帧计数器管理和APDU加解密。
    *   **数据传输:** 实现Block Transfer（块传输）机制，用于大数据量对象的传输。
3.  **测试 (`tests/test_dlms.c`):**
    *   对APDU、每个IC、HLS认证流程、Block Transfer编写独立的、详尽的测试用例。
    *   使用`cmocka`的mock功能模拟加密和socket行为，进行白盒测试。

## Phase 6: 文档、示例与发布

**目标:** 完善项目文档，提供使用示例，为最终发布做准备。

1.  **代码文档:** 使用 Doxygen/Javadoc 风格的注释，完善所有头文件中的API函数说明。
2.  **API文档:** 生成HTML格式的API参考手册。
3.  **示例代码:**
    *   创建 `/examples` 目录。
    *   为每个协议提供至少一个Master和一个Slave角色的示例程序。
    *   示例代码应清晰展示如何初始化`context`、调用API以及处理回调。
4.  **最终审查:**
    *   代码审查（Code Review），统一代码风格。
    *   性能分析与优化（如果需要）。
    *   准备版本发布（e.g., v1.0.0）。
