cmake_minimum_required(VERSION 3.15)
set(CMAKE_C_STANDARD 99)
# =========================================================================
# 1. 工具链前置设置 (必须在 project() 之前)
# =========================================================================
set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR riscv)

# 允许裸机编译时跳过标准的 CMake 编译器测试
set(CMAKE_TRY_COMPILE_TARGET_TYPE "STATIC_LIBRARY")

# MounRiver 编译器路径
set(TOOLCHAIN_PATH "/home/dxxdx/公共/RISC-V Embedded GCC15")

if(CMAKE_HOST_WIN32)
    set(EXE_SUFFIX ".exe")
else()
    set(EXE_SUFFIX "")
endif()

# 工具链可执行文件定义
set(CMAKE_C_COMPILER    "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-gcc${EXE_SUFFIX}")
set(CMAKE_CXX_COMPILER  "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-g++${EXE_SUFFIX}")
set(CMAKE_ASM_COMPILER  "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-gcc${EXE_SUFFIX}")
set(CMAKE_OBJCOPY      "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-objcopy${EXE_SUFFIX}")
set(CMAKE_OBJDUMP      "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-objdump${EXE_SUFFIX}")
set(CMAKE_SIZE         "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-size${EXE_SUFFIX}")

# 【核心修复】：显式覆盖内置的静态库打包工具为 GCC-LTO 专用的 ar / ranlib
# 这样才能保证你在下面捞出来的 .a 静态库可以无缝参与 LTO 全局解构与去虚化
set(CMAKE_AR           "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-gcc-ar${EXE_SUFFIX}" CACHE FILEPATH "LTO AR tool" FORCE)
set(CMAKE_RANLIB       "${TOOLCHAIN_PATH}/bin/riscv32-wch-elf-gcc-ranlib${EXE_SUFFIX}" CACHE FILEPATH "LTO RANLIB tool" FORCE)

# =========================================================================
# 2. 项目声明
# =========================================================================
project(CH572_BLE_Project C CXX ASM)

# 全局宏定义
add_compile_definitions(
        DEBUG=1
        CH572
        CH573
)

# =========================================================================
# 3. 链接脚本路径
# =========================================================================
set(LINKER_SCRIPT "${CMAKE_CURRENT_SOURCE_DIR}/Ld/Link.ld")

# =========================================================================
# 4. 编译与链接标志（已注入极致优化与真·LTO）
# =========================================================================
set(ARCH_FLAGS "-march=rv32imc_zba_zbb_zbc_zbs_xw -mabi=ilp32 -mcmodel=medany -msmall-data-limit=8")

# 【优化开关】：升级为 -O3 并强势注入 -flto。同时保留 -g3 提供完整的符号追踪能力
set(OPTIMIZATION_FLAGS "-Og -flto -g3")

# C 编译标志
set(COMMON_C_FLAGS "${ARCH_FLAGS} -mno-save-restore -fmax-errors=20 ${OPTIMIZATION_FLAGS} -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections -fno-common --param=highcode-gen-section-name=1")
set(CMAKE_C_FLAGS "${COMMON_C_FLAGS}" CACHE INTERNAL "c flags")

# 汇编编译标志
set(CMAKE_ASM_FLAGS "${ARCH_FLAGS} -x assembler-with-cpp -g" CACHE INTERNAL "asm flags")

# 链接标志：必须在链接的最末尾同样带上 ${OPTIMIZATION_FLAGS} 触发链接期全代码去虚化
set(LINK_FLAGS "${ARCH_FLAGS} -T \"${LINKER_SCRIPT}\" -nostartfiles -Xlinker --gc-sections -Xlinker --print-memory-usage --param=highcode-gen-section-name=1 -Wl,-Map=${PROJECT_NAME}.map --specs=nano.specs --specs=nosys.specs ${OPTIMIZATION_FLAGS}")
set(CMAKE_EXE_LINKER_FLAGS "${LINK_FLAGS}" CACHE INTERNAL "exe linker flags")

# =========================================================================
# 5. 源文件搜寻
# =========================================================================
file(GLOB_RECURSE SRC_APP          "${CMAKE_CURRENT_SOURCE_DIR}/APP/*.c")
file(GLOB_RECURSE SRC_PROFILE      "${CMAKE_CURRENT_SOURCE_DIR}/Profile/*.c")
file(GLOB_RECURSE SRC_HAL          "${CMAKE_CURRENT_SOURCE_DIR}/HAL/*.c")
file(GLOB_RECURSE SRC_DRIVER       "${CMAKE_CURRENT_SOURCE_DIR}/StdPeriphDriver/*.c")
file(GLOB_RECURSE SRC_STARTUP      "${CMAKE_CURRENT_SOURCE_DIR}/Startup/*.S" "${CMAKE_CURRENT_SOURCE_DIR}/Startup/*.s")
file(GLOB SRC_ROOT                 "${CMAKE_CURRENT_SOURCE_DIR}/*.c")

set(PROJECT_SOURCES
        ${SRC_APP} ${SRC_PROFILE} ${SRC_HAL} ${SRC_DRIVER} ${SRC_STARTUP} ${SRC_ROOT}
        MPU6050.c
        MPU6050.h
        I2CCtrl.h
        I2C_HW.h
        dataTxTask.h
        dataCollectTask.c
        dataCollectTask.h
        dataRxTask.c
        dataRxTask.h
        misc.c
)

add_executable(${PROJECT_NAME} ${PROJECT_SOURCES})

# =========================================================================
# 6. 头文件依赖包含
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
# 7. 外部静态库引入
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
# 8. 后处理命令 (生成 HEX/LST/ASM 文件并查看内存)
# =========================================================================
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        # 1. 生成烧录用 hex 文件
        COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${PROJECT_NAME}.hex

        # 2. 生成标准的 C/汇编 交错文本文件 (.lst)
        COMMAND sh -c "\"${CMAKE_OBJDUMP}\" --source --all-headers --demangle -M xw --line-numbers --wide \"$<TARGET_FILE:${PROJECT_NAME}>\" > \"${PROJECT_NAME}.lst\""

        # 3. 生成你要看的纯反汇编文件 (.asm)，并完美适配 WCH 扩展指令集汇编器 (-M xw)
        COMMAND sh -c "\"${CMAKE_OBJDUMP}\" -S -d -M xw \"$<TARGET_FILE:${PROJECT_NAME}>\" > \"${PROJECT_NAME}.asm\""

        # 4. 打印 Berkeley 格式的最终固件尺寸，当场看 LTO 瘦身效果
        COMMAND ${CMAKE_SIZE} --format=berkeley $<TARGET_FILE:${PROJECT_NAME}>

        # 5. 备份 ELF 到源码根目录
        COMMAND ${CMAKE_COMMAND} -E copy $<TARGET_FILE:${PROJECT_NAME}> "${CMAKE_CURRENT_SOURCE_DIR}/${PROJECT_NAME}.elf"

        # 6. 运行 eanaly 内存足迹分析（如果可用）
        COMMAND ${EANALY_EXE} $<TARGET_FILE:${PROJECT_NAME}>

        WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
        COMMENT "Generating HEX/LST/ASM files, displaying memory usage, and running LTO post-analysis..."
)