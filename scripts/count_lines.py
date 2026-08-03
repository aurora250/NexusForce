#!/usr/bin/env python3
"""NexusForce 代码行数统计工具

统计项目源代码的行数，支持快速统计、手动分类和 cloc 集成。

用法:
    python count_lines.py                     # 自动模式（优先 cloc）
    python count_lines.py --simple            # 快速统计（仅总行数）
    python count_lines.py --manual            # 手动统计（代码 / 注释 / 空行）
    python count_lines.py --cloc              # 强制使用 cloc
    python count_lines.py --ext cpp,hpp,h     # 只统计指定类型
    python count_lines.py --dir src,tests     # 只统计指定目录
"""

import argparse
import builtins
import re
import shutil
import subprocess
import sys
from pathlib import Path

# 项目目录
PROJECT_ROOT = Path(__file__).resolve().parent.parent

# 默认值
DEFAULT_DIRS = ["include", "src", "tests"]
DEFAULT_EXTS = ["c", "cpp", "h", "hpp", "py", "java", "js", "ts", "sh"]

# ANSI 颜色
RED = "\033[0;31m"
GREEN = "\033[0;32m"
YELLOW = "\033[1;33m"
CYAN = "\033[0;36m"
NC = "\033[0m"

# 按扩展名区分的注释语法
LINE_COMMENT_PATTERNS = {
    # C 家族: // ... 和 /* ... */
    frozenset(["c", "cpp", "h", "hpp", "java", "js", "ts", "css", "scss",
               "php", "go", "rs", "swift", "kt", "scala"]):
        re.compile(r"^\s*(//|/\*|\*[^/]|\*/)"),

    # 脚本 / 配置类: # ...
    frozenset(["py", "rb", "pl", "sh", "bash", "zsh", "yaml", "yml", "toml",
               "cfg", "ini", "cmake", "txt"]):
        re.compile(r"^\s*#"),

    # SQL / Lua / Haskell: -- ...
    frozenset(["sql", "lua", "haskell"]):
        re.compile(r"^\s*--"),
}


def _get_comment_pattern(ext: str):
    """根据扩展名返回对应的注释行正则。"""
    for exts, pattern in LINE_COMMENT_PATTERNS.items():
        if ext in exts:
            return pattern
    return None


def _resolve_dirs(names: list[str]) -> list[Path]:
    """将目录名解析为相对于项目根目录的绝对路径。"""
    dirs = []
    for name in names:
        p = (PROJECT_ROOT / name).resolve()
        if p.is_dir():
            dirs.append(p)
        else:
            builtins.print(f"{YELLOW}[警告] 目录不存在，跳过: {p}{NC}")
    return dirs


def _collect_files(dirs: list[Path], exts: list[str]) -> list[Path]:
    """收集所有匹配扩展名的文件。"""
    files = []
    ext_set = {e.removeprefix(".").removeprefix("*.") for e in exts}
    for d in dirs:
        for ext in builtins.sorted(ext_set):
            files.extend(d.rglob(f"*.{ext}"))
    return builtins.sorted(files)


def count_simple(dirs: list[Path], exts: list[str]) -> None:
    """快速统计 — 只统计总行数和文件数。"""
    builtins.print(f"{CYAN}快速统计代码行数...{NC}")
    builtins.print("=" * 48)

    total_files = 0
    total_lines = 0

    for d in dirs:
        files = _collect_files([d], exts)
        dir_lines = 0
        for f in files:
            try:
                dir_lines += _count_lines(f)
            except OSError:
                pass

        tag = GREEN if files else YELLOW
        builtins.print(f"  {d.relative_to(PROJECT_ROOT)}: {tag}{dir_lines}{NC} 行"
              f" ({builtins.len(files)} 个文件)")

        total_files += builtins.len(files)
        total_lines += dir_lines

    builtins.print("-" * 48)
    builtins.print(f"总计: {GREEN}{total_lines}{NC} 行 ({total_files} 个文件)")


def count_manual(dirs: list[Path], exts: list[str]) -> None:
    """手动统计 — 区分代码行、注释行、空行。"""
    builtins.print(f"{CYAN}手动统计代码行数（代码 / 注释 / 空行）...{NC}")
    builtins.print("=" * 48)

    total_files = 0
    total_lines = 0
    total_code = 0
    total_comment = 0
    total_blank = 0

    for d in dirs:
        files = _collect_files([d], exts)
        if not files:
            builtins.print(f"\n{YELLOW}--- {d.relative_to(PROJECT_ROOT)} ---{NC}")
            builtins.print("  没有找到匹配的文件")
            continue

        builtins.print(f"\n{YELLOW}--- {d.relative_to(PROJECT_ROOT)} ---{NC}")

        dir_files = 0
        dir_lines = 0

        for f in files:
            dir_files += 1
            try:
                text = f.read_text(encoding="utf-8", errors="ignore")
            except OSError:
                continue

            lines = text.splitlines()
            dir_lines += builtins.len(lines)

            pattern = _get_comment_pattern(f.suffix.removeprefix("."))
            if pattern:
                for line in lines:
                    stripped = line.strip()
                    if not stripped:
                        total_blank += 1
                    elif pattern.match(stripped):
                        total_comment += 1
                    else:
                        total_code += 1
            else:
                # 未知类型：全算代码
                total_code += builtins.len(lines)
                total_blank += builtins.sum(1 for l in lines if not l.strip())

        builtins.print(f"  文件数: {dir_files}")
        builtins.print(f"  总行数: {dir_lines}")

        total_files += dir_files
        total_lines += dir_lines

    builtins.print(f"\n{GREEN}" + "=" * 48)
    builtins.print("统计汇总")
    builtins.print("=" * 48)
    builtins.print(f"  总文件数:     {total_files}")
    builtins.print(f"  总行数:       {total_lines}")
    builtins.print(f"  代码行数:     {total_code}")
    builtins.print(f"  注释行数:     {total_comment}")
    builtins.print(f"  空行数:       {total_blank}{NC}")


def count_cloc(dirs: list[Path], exts: list[str]) -> int:
    """使用 cloc 工具统计。返回 cloc 的退出码。"""
    if not shutil.which("cloc"):
        builtins.print(f"{RED}错误: cloc 未安装{NC}")
        builtins.print("安装: sudo apt install cloc / brew install cloc / choco install cloc")
        return 1

    builtins.print(f"{CYAN}使用 cloc 统计...{NC}")
    builtins.print("=" * 48)

    ext_list = ",".join(e.removeprefix(".").removeprefix("*.") for e in exts)
    cmd = ["cloc", "--include-ext=" + ext_list] + [str(d) for d in dirs]
    return subprocess.call(cmd)


def _count_lines(path: Path) -> int:
    """快速统计单个文件的行数。"""
    count = 0
    with builtins.open(path, "rb") as f:
        for _ in f:
            count += 1
    return count


def main() -> int:
    parser = argparse.ArgumentParser(
        description="NexusForce 代码行数统计",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    # ── 模式 ──
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument(
        "--simple", "-s",
        action="store_const", dest="mode", const="simple",
        help="快速统计模式（只统计总行数和文件数）",
    )
    mode.add_argument(
        "--manual", "-m",
        action="store_const", dest="mode", const="manual",
        help="手动统计模式（区分代码、注释和空行）",
    )
    mode.add_argument(
        "--cloc", "-c",
        action="store_const", dest="mode", const="cloc",
        help="使用 cloc 工具统计（需预先安装）",
    )

    # ── 过滤 ──
    parser.add_argument(
        "--ext", "-e",
        default=",".join(DEFAULT_EXTS),
        help=f"文件扩展名，逗号分隔（默认: {','.join(DEFAULT_EXTS)}）",
    )
    parser.add_argument(
        "--dir", "-d",
        default=",".join(DEFAULT_DIRS),
        help=f"要统计的目录，逗号分隔（默认: {','.join(DEFAULT_DIRS)}）",
    )

    args = parser.parse_args()

    exts = [e.strip() for e in args.ext.split(",") if e.strip()]
    dir_names = [d.strip() for d in args.dir.split(",") if d.strip()]
    dirs = _resolve_dirs(dir_names)

    if not dirs:
        builtins.print(f"{RED}错误: 没有找到任何可统计的目录{NC}")
        return 1

    # ── 确定模式 ──
    mode = args.mode or "auto"

    if mode == "auto":
        if shutil.which("cloc"):
            return count_cloc(dirs, exts)
        else:
            builtins.print(f"{YELLOW}提示: 安装 cloc 可获得更详细的统计信息{NC}")
            builtins.print(f"{YELLOW}安装方法: sudo apt install cloc / brew install cloc{NC}\n")
            count_simple(dirs, exts)
            return 0
    elif mode == "simple":
        count_simple(dirs, exts)
        return 0
    elif mode == "manual":
        count_manual(dirs, exts)
        return 0
    elif mode == "cloc":
        return count_cloc(dirs, exts)

    return 0


if __name__ == "__main__":
    sys.exit(main())
