cmake_minimum_required(VERSION 3.15)

# =========================================================================
# 1. 交叉编译器工具链设置 (★必须放在 project() 之前)
# =========================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# 告诉 CMake 检查编译器时不要尝试链接，只编译成静态库即可，防止报 broken 错误
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# 你的 MounRiver 内部工具链路径
set(TOOLCHAIN_PATH "D:/MounRiver_Studio2/resources/app/resources/win32/components/WCH/Toolchain/RISC-V Embedded GCC")

if(CMAKE_HOST_WIN32)
    set(EXE_SUFFIX ".exe")
else()
    set(EXE_SUFFIX "")
endif()

# 核心编译器指针重定向
set(CMAKE_C_COMPILER   "${TOOLCHAIN_PATH}/bin/riscv-none-embed-gcc${EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/bin/riscv-none-embed-g++${EXE_SUFFIX}")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/bin/riscv-none-embed-gcc${EXE_SUFFIX}")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PATH}/bin/riscv-none-embed-objcopy${EXE_SUFFIX}")
set(CMAKE_SIZE         "${TOOLCHAIN_PATH}/bin/riscv-none-embed-size${EXE_SUFFIX}")

# =========================================================================
# 2. 初始化工程
# =========================================================================
project(CH592_BLE_Project C CXX ASM)
# 恢复 DEBUG 打印和芯片定义
add_compile_definitions(
        DEBUG=0        # 恢复所有的 PRINTF，找回那 6.7KB 代码
        CH592            # 确保 SDK 知道当前是 CH592 芯片
)


# =========================================================================
# 3. 纯本地相对路径定义 (全部基于当前项目根目录)
# =========================================================================
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/Ld/Link.ld")

# =========================================================================
# 4. 编译与链接参数
# =========================================================================
set(ARCH_FLAGS "-march=rv32imac -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8")

# C 编译参数
set(COMMON_C_FLAGS "${ARCH_FLAGS} -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common -g")
set(CMAKE_C_FLAGS "${COMMON_C_FLAGS}" CACHE INTERNAL "c flags")

# 汇编编译参数
set(CMAKE_ASM_FLAGS "${ARCH_FLAGS} -x assembler-with-cpp -g" CACHE INTERNAL "asm flags")

# 链接参数
set(LINK_FLAGS "${ARCH_FLAGS} -T \"${LINKER_SCRIPT}\" -nostartfiles -Xlinker --gc-sections -Xlinker --print-memory-usage -Wl,-Map=${PROJECT_NAME}.map --specs=nano.specs --specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${LINK_FLAGS}" CACHE INTERNAL "exe linker flags")

# =========================================================================
# 5. 收集全本地源文件 (已修正根目录递归漏洞)
# =========================================================================
file(GLOB_RECURSE SRC_APP          "${CMAKE_CURRENT_SOURCE_DIR}/APP/*.c")
file(GLOB_RECURSE SRC_PROFILE      "${CMAKE_CURRENT_SOURCE_DIR}/Profile/*.c")
file(GLOB_RECURSE SRC_HAL          "${CMAKE_CURRENT_SOURCE_DIR}/HAL/*.c")
file(GLOB_RECURSE SRC_DRIVER       "${CMAKE_CURRENT_SOURCE_DIR}/StdPeriphDriver/*.c")
file(GLOB_RECURSE SRC_STARTUP      "${CMAKE_CURRENT_SOURCE_DIR}/Startup/*.S" "${CMAKE_CURRENT_SOURCE_DIR}/Startup/*.s")
file(GLOB_RECURSE SRC_RVMSIS       "${CMAKE_CURRENT_SOURCE_DIR}/RVMSIS/*.c")

# 【★核心修改】必须去掉 _RECURSE！只搜根目录本层，严禁钻进 cmake-build-debug 乱捞 main 函数
file(GLOB SRC_ROOT                 "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

set(PROJECT_SOURCES
        ${SRC_APP} ${SRC_PROFILE} ${SRC_HAL} ${SRC_DRIVER} ${SRC_STARTUP} ${SRC_RVMSIS} ${SRC_ROOT}
        dataTask.c
        dataTask.h
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# =========================================================================
# 6. 头文件包含路径 (完美保留你调整后的精细化子目录)
# =========================================================================
target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}"
        "${CMAKE_CURRENT_SOURCE_DIR}/APP/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/Profile/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/HAL/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/StdPeriphDriver/inc"
        "${CMAKE_CURRENT_SOURCE_DIR}/RVMSIS"
        "${CMAKE_CURRENT_SOURCE_DIR}/Startup"
        "${CMAKE_CURRENT_SOURCE_DIR}/LIB"
)

# =========================================================================
# 7. 链接本地静态库
# =========================================================================
file(GLOB_RECURSE ALL_LIB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.a")

if(ALL_LIB_FILES)
    list(FILTER ALL_LIB_FILES EXCLUDE REGEX "cmake-build-debug")
endif()


# 【核心修改】：使用 --whole-archive 强行把蓝牙库里的所有代码（包括中断）塞进去
target_link_libraries(${PROJECT_NAME} PRIVATE
        -Wl,--whole-archive      # 告诉链接器：接下来的库，哪怕看着像废代码也不许删！
        ${ALL_LIB_FILES}
        -Wl,--no-whole-archive   # 告诉链接器：好了，后面的常规库恢复正常优化
)
# =========================================================================
# 8. 后处理：自动生成 HEX 并打印内存
# =========================================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
        COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "正在本地生成目标 HEX 并通过 size 打印 Flash 和 RAM 占用..."
)