cmake_minimum_required(VERSION 3.15)

# =========================================================================
# 1. 交叉编译器工具链设置 (★必须放在 project() 之前)
# =========================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# 告诉 CMake 检查编译器时不要尝试链接，只编译成静态库即可
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# MounRiver 内部工具链路径
set(TOOLCHAIN_PATH "/home/dxxdx/公共/RISC-V Embedded GCC15")

#这个自己改，我没啥说的


if(CMAKE_HOST_WIN32)
    set(EXE_SUFFIX ".exe")
else()
    set(EXE_SUFFIX "")
endif()

# 核心编译器指针重定向
set(CMAKE_C_COMPILER   "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-gcc${EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-g++${EXE_SUFFIX}")
set(CMAKE_ASM_COMPILER "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-gcc${EXE_SUFFIX}")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-objcopy${EXE_SUFFIX}")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-objdump${EXE_SUFFIX}")
set(CMAKE_SIZE         "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-size${EXE_SUFFIX}")



#傻逼wch，一会用none一会又用自己魔改的gcc，自己看情况选

# =========================================================================
# 2. 初始化工程与芯片宏定义
# =========================================================================
project(CH572_BLE_Project C CXX ASM)

# 芯片级全局宏定义
add_compile_definitions(
        DEBUG=1
        CH572
        CH573
)

# =========================================================================
# 3. 链接脚本位置
# =========================================================================
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/Ld/Link.ld")

# =========================================================================
# 4. 编译与链接参数
# =========================================================================
set(ARCH_FLAGS "-march=rv32imc_zba_zbb_zbc_zbs_xw -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8")

# C 编译参数
set(COMMON_C_FLAGS "${ARCH_FLAGS} -mno-save-restore -fmax-errors=20 -Os -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common --param=highcode-gen-section-name=1 -g")
set(CMAKE_C_FLAGS "${COMMON_C_FLAGS}" CACHE INTERNAL "c flags")

# 汇编编译参数
set(CMAKE_ASM_FLAGS "${ARCH_FLAGS} -x assembler-with-cpp -g" CACHE INTERNAL "asm flags")

# 链接参数
set(LINK_FLAGS "${ARCH_FLAGS} -T \"${LINKER_SCRIPT}\" -nostartfiles -Xlinker --gc-sections -Xlinker --print-memory-usage --param=highcode-gen-section-name=1 -Wl,-Map=${PROJECT_NAME}.map --specs=nano.specs --specs=nosys.specs")
set(CMAKE_EXE_LINKER_FLAGS "${LINK_FLAGS}" CACHE INTERNAL "exe linker flags")

# =========================================================================
# 5. 收集源文件
# =========================================================================
file(GLOB_RECURSE SRC_APP          "${CMAKE_CURRENT_SOURCE_DIR}/APP/*.c")
file(GLOB_RECURSE SRC_PROFILE      "${CMAKE_CURRENT_SOURCE_DIR}/Profile/*.c")
file(GLOB_RECURSE SRC_HAL          "${CMAKE_CURRENT_SOURCE_DIR}/HAL/*.c")
file(GLOB_RECURSE SRC_DRIVER       "${CMAKE_CURRENT_SOURCE_DIR}/StdPeriphDriver/*.c")
file(GLOB_RECURSE SRC_STARTUP      "${CMAKE_CURRENT_SOURCE_DIR}/Startup/*.S" "${CMAKE_CURRENT_SOURCE_DIR}/Startup/*.s")

# 仅仅抓取可能直接放在项目根目录下的 .c 文件
file(GLOB SRC_ROOT                 "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

set(PROJECT_SOURCES
        ${SRC_APP} ${SRC_PROFILE} ${SRC_HAL} ${SRC_DRIVER} ${SRC_STARTUP} ${SRC_ROOT}
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# =========================================================================
# 6. 头文件包含路径
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
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

# =========================================================================
# 7. 链接本地静态库
# =========================================================================
file(GLOB_RECURSE ALL_LIB_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.a")

if(ALL_LIB_FILES)
    list(FILTER ALL_LIB_FILES EXCLUDE REGEX "cmake-build-debug")
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
        -Wl,--whole-archive
        ${ALL_LIB_FILES}
        -Wl,--no-whole-archive
)

# =========================================================================
# 8. 后处理：自动生成 HEX、LST 并打印内存（【修正】给 sh -c 内部的命令加上了硬核防碎转义双引号）
# =========================================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex
        COMMAND sh -c "\"${CMAKE_OBJDUMP}\" --source --all-headers --demangle -M xw --line-numbers --wide \"$<TARGET_FILE:${PROJECT_NAME}>\" > \"${PROJECT_NAME}.lst\""
        COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>
        COMMENT "Generating HEX and LST files, and displaying memory usage..."
)