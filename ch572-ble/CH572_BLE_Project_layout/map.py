from __future__ import annotations

import csv
from collections import defaultdict
from pathlib import Path

try:
    from elftools.elf.elffile import ELFFile
    from elftools.elf.sections import SymbolTableSection
except ModuleNotFoundError:
    raise SystemExit(
        "未安装 pyelftools。\n"
        "请在当前 PyCharm 虚拟环境执行：\n"
        "python -m pip install pyelftools"
    )


# ============================ 配置区 ============================

ELF_FILE = Path(
    "/home/dxxdx/CLionProjects/realtime-motion-tracking-MPU6050/"
    "ch572-ble/CH572_BLE_Project.elf"
)

# 从你的 linker script 的 MEMORY {} 中复制。
# 格式是：区域名: (ORIGIN, LENGTH)
#
# 不填也能运行，脚本依然会输出真实 VMA / LMA；
# 只是不会自动标注 RAM / FLASH 名称。
#
# 下面地址只是示例，必须替换成你工程 .ld 文件里的真实值。
MEMORY_REGIONS = {
    # "FLASH": (0x00000000, 0x00040000),
    # "RAM":   (0x20000000, 0x00004000),
}

# 打印这些 section 中最大的函数 / 静态对象。
FOCUS_SECTIONS = (
    ".highcode",
    ".text",
    ".data",
    ".sdata",
    ".bss",
    ".sbss",
)

TOP_SYMBOLS_PER_SECTION = 20

# ===============================================================


SHF_WRITE = 0x1
SHF_ALLOC = 0x2
SHF_EXECINSTR = 0x4


def hex_addr(value: int | None) -> str:
    if value is None:
        return "-"
    return f"0x{value:08X}"


def region_of(address: int | None) -> str:
    if address is None:
        return "-"

    for name, (origin, length) in MEMORY_REGIONS.items():
        if origin <= address < origin + length:
            return name

    return "?"


def section_flags(flags: int) -> str:
    result = ""
    if flags & SHF_ALLOC:
        result += "A"
    if flags & SHF_WRITE:
        result += "W"
    if flags & SHF_EXECINSTR:
        result += "X"
    return result or "-"


def section_kind(flags: int, file_bytes: int, section_type: str) -> str:
    if flags & SHF_EXECINSTR:
        if file_bytes:
            return "CODE"
        return "CODE(no image)"

    if flags & SHF_WRITE:
        if section_type == "SHT_NOBITS" or file_bytes == 0:
            return "RAM zero-init"
        return "RAM init"

    return "RODATA"


def segment_flags(flags: int) -> str:
    # ELF PF_X=1, PF_W=2, PF_R=4
    result = ""
    if flags & 4:
        result += "R"
    if flags & 2:
        result += "W"
    if flags & 1:
        result += "X"
    return result or "-"


def find_load_segment(elf: ELFFile, vma: int):
    """找包含这个 VMA 的 PT_LOAD segment。"""
    for segment in elf.iter_segments():
        if segment["p_type"] != "PT_LOAD":
            continue

        seg_start = int(segment["p_vaddr"])
        seg_end = seg_start + int(segment["p_memsz"])

        if seg_start <= vma < seg_end:
            return segment

    return None


def is_focus_section(name: str) -> bool:
    for prefix in FOCUS_SECTIONS:
        if name == prefix or name.startswith(prefix + "."):
            return True
    return False


def write_csv(path: Path, rows: list[dict]) -> None:
    if not rows:
        return

    with path.open("w", newline="", encoding="utf-8") as f:
        writer = csv.DictWriter(f, fieldnames=rows[0].keys())
        writer.writeheader()
        writer.writerows(rows)


def main() -> None:
    if not ELF_FILE.is_file():
        raise FileNotFoundError(f"ELF 不存在：{ELF_FILE}")

    output_dir = ELF_FILE.parent / f"{ELF_FILE.stem}_layout"
    output_dir.mkdir(exist_ok=True)

    section_rows: list[dict] = []
    symbol_rows: list[dict] = []

    with ELF_FILE.open("rb") as f:
        elf = ELFFile(f)

        print("=" * 112)
        print(f"ELF: {ELF_FILE}")
        print(f"ELF class: {elf.elfclass}-bit, little_endian={elf.little_endian}")
        print("=" * 112)

        if MEMORY_REGIONS:
            print("\n[Configured memory regions]")
            for name, (origin, length) in MEMORY_REGIONS.items():
                print(
                    f"  {name:<10} "
                    f"{hex_addr(origin)} - {hex_addr(origin + length - 1)} "
                    f"({length} bytes)"
                )
        else:
            print(
                "\n[Configured memory regions]\n"
                "  未配置。VMA/LMA 仍然准确，但 Region 列会显示为 ?。\n"
                "  建议从 linker script 的 MEMORY {} 填入 FLASH/RAM 的 ORIGIN 和 LENGTH。"
            )

        # -------------------------------------------------------
        # 1. Program Headers：最底层的 VMA / LMA 视图
        # -------------------------------------------------------
        print("\n[PT_LOAD program headers]")
        print(
            f"{'VMA':<12} {'LMA(p_paddr)':<14} "
            f"{'FileSz':>10} {'MemSz':>10} {'Flags':<6}"
        )
        print("-" * 64)

        for segment in elf.iter_segments():
            if segment["p_type"] != "PT_LOAD":
                continue

            vma = int(segment["p_vaddr"])
            lma = int(segment["p_paddr"])
            filesz = int(segment["p_filesz"])
            memsz = int(segment["p_memsz"])
            flags = int(segment["p_flags"])

            print(
                f"{hex_addr(vma):<12} "
                f"{hex_addr(lma):<14} "
                f"{filesz:>10} {memsz:>10} "
                f"{segment_flags(flags):<6}"
            )

        # -------------------------------------------------------
        # 2. Sections：实际输出 section 布局
        # -------------------------------------------------------
        print("\n[Allocated sections]")
        print(
            f"{'Section':<25} {'Size':>9} "
            f"{'VMA(runtime)':<14} {'Run':<8} "
            f"{'LMA(image)':<14} {'Load':<8} "
            f"{'Image':>9} {'Flags':<6} {'Kind'}"
        )
        print("-" * 125)

        section_by_index: dict[int, dict] = {}

        for index in range(elf.num_sections()):
            section = elf.get_section(index)

            name = section.name or f"<section_{index}>"
            flags = int(section["sh_flags"])
            size = int(section["sh_size"])
            vma = int(section["sh_addr"])
            section_type = str(section["sh_type"])

            if not (flags & SHF_ALLOC):
                continue

            segment = find_load_segment(elf, vma)

            lma = None
            image_bytes = 0

            # SHT_NOBITS 典型就是 .bss/.sbss：
            # 有 VMA，占运行时 RAM；没有加载镜像。
            if section_type != "SHT_NOBITS" and segment is not None:
                seg_vma = int(segment["p_vaddr"])
                seg_lma = int(segment["p_paddr"])
                seg_file_end = seg_vma + int(segment["p_filesz"])

                lma = seg_lma + (vma - seg_vma)
                image_bytes = max(0, min(vma + size, seg_file_end) - vma)

            run_region = region_of(vma)
            load_region = region_of(lma) if image_bytes else "-"
            kind = section_kind(flags, image_bytes, section_type)

            row = {
                "section_index": index,
                "section": name,
                "size_bytes": size,
                "vma": hex_addr(vma),
                "runtime_region": run_region,
                "lma": hex_addr(lma) if image_bytes else "-",
                "load_region": load_region,
                "image_bytes": image_bytes,
                "flags": section_flags(flags),
                "type": section_type,
                "kind": kind,
                "_vma_int": vma,
                "_lma_int": lma,
            }

            section_rows.append(row)
            section_by_index[index] = row

            print(
                f"{name:<25.25} {size:>9} "
                f"{hex_addr(vma):<14} {run_region:<8} "
                f"{row['lma']:<14} {load_region:<8} "
                f"{image_bytes:>9} {row['flags']:<6} {kind}"
            )

        # -------------------------------------------------------
        # 3. 汇总：运行时占用 / 镜像占用
        # -------------------------------------------------------
        runtime_summary = defaultdict(int)
        image_summary = defaultdict(int)

        for row in section_rows:
            runtime_summary[row["runtime_region"]] += row["size_bytes"]
            if row["image_bytes"]:
                image_summary[row["load_region"]] += row["image_bytes"]

        print("\n[Runtime allocation by VMA]")
        for region, size in sorted(runtime_summary.items(), key=lambda x: x[1], reverse=True):
            print(f"  {region:<10} {size:>10} bytes")

        print("\n[Image payload by LMA]")
        for region, size in sorted(image_summary.items(), key=lambda x: x[1], reverse=True):
            print(f"  {region:<10} {size:>10} bytes")

        # -------------------------------------------------------
        # 4. 符号：定位 .highcode / .data / .bss 内具体是谁最大
        # -------------------------------------------------------
        symtab = None

        for section in elf.iter_sections():
            if isinstance(section, SymbolTableSection) and section.name == ".symtab":
                symtab = section
                break

        if symtab is None:
            for section in elf.iter_sections():
                if isinstance(section, SymbolTableSection):
                    symtab = section
                    break

        if symtab is None:
            print("\n[Symbols]\nELF 中没有 .symtab，无法按函数/变量列出 section 内成员。")
        else:
            for symbol in symtab.iter_symbols():
                symbol_type = symbol["st_info"]["type"]
                symbol_size = int(symbol["st_size"])
                symbol_name = symbol.name

                if symbol_type not in ("STT_FUNC", "STT_OBJECT"):
                    continue

                if not symbol_name or symbol_size == 0:
                    continue

                section_index = symbol["st_shndx"]

                if not isinstance(section_index, int):
                    continue

                section_row = section_by_index.get(section_index)
                if section_row is None:
                    continue

                section_name = section_row["section"]
                if not is_focus_section(section_name):
                    continue

                symbol_vma = int(symbol["st_value"])
                symbol_lma = None

                if section_row["_lma_int"] is not None:
                    offset_in_section = symbol_vma - section_row["_vma_int"]
                    if offset_in_section >= 0:
                        symbol_lma = section_row["_lma_int"] + offset_in_section

                symbol_rows.append(
                    {
                        "section": section_name,
                        "symbol_type": "FUNC" if symbol_type == "STT_FUNC" else "OBJECT",
                        "symbol": symbol_name,
                        "size_bytes": symbol_size,
                        "vma": hex_addr(symbol_vma),
                        "runtime_region": section_row["runtime_region"],
                        "lma": hex_addr(symbol_lma),
                        "load_region": section_row["load_region"],
                    }
                )

            grouped_symbols = defaultdict(list)
            for row in symbol_rows:
                grouped_symbols[row["section"]].append(row)

            print("\n[Largest symbols in focused sections]")

            if not grouped_symbols:
                print("  没找到带大小的函数/对象符号。可能 ELF 被 strip，或符号尺寸为 0。")

            for section_name in sorted(grouped_symbols):
                rows = sorted(
                    grouped_symbols[section_name],
                    key=lambda x: x["size_bytes"],
                    reverse=True,
                )[:TOP_SYMBOLS_PER_SECTION]

                print(f"\n  [{section_name}]")
                print(
                    f"  {'Type':<8} {'Size':>9} "
                    f"{'VMA':<12} {'LMA':<12} Symbol"
                )
                print("  " + "-" * 85)

                for row in rows:
                    print(
                        f"  {row['symbol_type']:<8} "
                        f"{row['size_bytes']:>9} "
                        f"{row['vma']:<12} "
                        f"{row['lma']:<12} "
                        f"{row['symbol']}"
                    )

    # CSV 里不要输出内部临时字段
    csv_sections = []
    for row in section_rows:
        csv_sections.append(
            {
                key: value
                for key, value in row.items()
                if not key.startswith("_")
            }
        )

    write_csv(output_dir / "sections.csv", csv_sections)
    write_csv(output_dir / "symbols.csv", symbol_rows)

    print("\n" + "=" * 112)
    print(f"CSV 已写入：{output_dir}")
    print("  sections.csv  : section 布局，可直接按 Size/VMA/LMA 排序")
    print("  symbols.csv   : .highcode/.text/.data/.bss 内函数和对象")
    print("=" * 112)

    print(
        "\n判读规则：\n"
        "  .highcode  VMA 在 RAM、LMA 在 FLASH  => 运行时 RAM + 固件镜像 FLASH\n"
        "  .data      VMA 在 RAM、LMA 在 FLASH  => 运行时 RAM + 初值 FLASH\n"
        "  .bss       VMA 在 RAM、Image=0        => 只占 RAM\n"
        "  .text      VMA/LMA 都在 FLASH         => 只占 FLASH\n"
        "\n注意：stack/heap 若仅靠 linker symbol 预留、没有实际 section，"
        "不会完整体现在本脚本的 section 汇总中；那部分仍要结合 .map 和 linker script 看。"
    )


if __name__ == "__main__":
    main()