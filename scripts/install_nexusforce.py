#!/usr/bin/env python3
"""NexusForce 便捷安装脚本

一键完成 CMake 配置、构建和安装流程。

用法:
    python install_nexusforce.py                     # 默认 Debug 安装到 build/install
    python install_nexusforce.py --release           # Release 构建
    python install_nexusforce.py --prefix /opt/nf    # 自定义安装路径
    python install_nexusforce.py --clean             # 清空 build 目录后重新构建
    python install_nexusforce.py --config-only       # 仅运行 CMake 配置（不编译）
    python install_nexusforce.py -j 8                # 指定并行编译任务数

依赖:
    - Python 3.7+
    - CMake ≥ 3.19（PATH 中可用）
    - vcpkg（通过 config.json 配置）
"""

import argparse
import json
import os
import platform
import shlex
import shutil
import subprocess
import sys
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent

DEFAULT_BUILD_DIR = PROJECT_ROOT / "build"
DEFAULT_INSTALL_PREFIX = DEFAULT_BUILD_DIR / "install"
DEFAULT_CONFIG = "Debug"
CONFIG_FILE = PROJECT_ROOT / "config.json"


def load_config() -> dict:
    """读取项目 config.json"""
    if not CONFIG_FILE.exists():
        print(f"[警告] 未找到 {CONFIG_FILE}，使用默认值")
        return {}

    with open(CONFIG_FILE, encoding="utf-8") as f:
        return json.load(f)


def detect_vcpkg_toolchain(cfg: dict) -> Path | None:
    """根据平台从 config.json 或环境变量中解析 vcpkg toolchain 路径"""
    env_root = os.environ.get("VCPKG_ROOT")
    if env_root:
        candidate = Path(env_root) / "scripts" / "buildsystems" / "vcpkg.cmake"
        if candidate.exists():
            return candidate

    key = "windows" if platform.system() == "Windows" else "linux"
    vcpkg_root = cfg.get("vcpkg", {}).get(key, "")
    if vcpkg_root:
        candidate = Path(vcpkg_root) / "scripts" / "buildsystems" / "vcpkg.cmake"
        if candidate.exists():
            return candidate

    return None


def get_generator() -> str | None:
    """为当前平台选择合适的 CMake 生成器"""
    system = platform.system()
    if system == "Windows":
        for vs in ("Visual Studio 17 2022", "Visual Studio 16 2019"):
            if shutil.which("msbuild"):
                return vs
        if shutil.which("ninja") and shutil.which("rc"):
            return "Ninja"
    return None


def run(cmd: list[str], **kwargs) -> int:
    """运行命令并实时输出"""
    print(f"\n→ {' '.join(shlex.quote(str(c)) for c in cmd)}")
    return subprocess.call(cmd, **kwargs)


def cmake_configure(args: argparse.Namespace, toolchain: Path | None) -> int:
    """运行 CMake 配置"""
    cmd = [
        "cmake",
        "-S", str(PROJECT_ROOT),
        "-B", str(args.build_dir),
    ]

    gen = args.generator or get_generator()
    if gen:
        cmd += ["-G", gen]

    is_multi = gen and ("Visual Studio" in gen or "Xcode" in gen)
    if not is_multi:
        cmd += [f"-DCMAKE_BUILD_TYPE={args.config}"]

    if toolchain:
        cmd += [f"-DCMAKE_TOOLCHAIN_FILE={toolchain}"]

    cmd += [f"-DCMAKE_INSTALL_PREFIX={args.prefix}"]

    cmd += ["-DNEXUSFORCE_BUILD_TESTS=OFF"]
    cmd += ["-DNEXUSFORCE_BUILD_EXAMPLES=OFF"]
    cmd += ["-DNEXUSFORCE_BUILD_BENCHMARKS=OFF"]
    cmd += ["-DNEXUSFORCE_BUILD_DOCS=OFF"]

    env = os.environ.copy()
    if platform.system() == "Windows":
        env.setdefault("VSLANG", "1033")

    return run(cmd, env=env)


def cmake_build(args: argparse.Namespace) -> int:
    """运行 CMake 构建"""
    cmd = [
        "cmake", "--build", str(args.build_dir),
        "--config", args.config,
    ]
    if args.parallel:
        cmd += ["--parallel", str(args.parallel)]
    if args.target:
        cmd += ["--target", args.target]
    if args.verbose:
        cmd += ["--verbose"]

    return run(cmd)


def cmake_install(args: argparse.Namespace) -> int:
    """运行 CMake 安装"""
    cmd = [
        "cmake", "--install", str(args.build_dir),
        "--config", args.config,
        "--prefix", str(args.prefix),
    ]
    return run(cmd)


def print_summary(args: argparse.Namespace, toolchain: Path | None) -> None:
    """打印配置摘要"""
    print("=" * 64)
    print("  NexusForce 安装脚本")
    print("=" * 64)
    print(f"  项目根目录:   {PROJECT_ROOT}")
    print(f"  构建目录:     {args.build_dir}")
    print(f"  安装前缀:     {args.prefix}")
    print(f"  构建类型:     {args.config}")
    print(f"  生成器:       {args.generator or get_generator() or '(CMake 默认)'}")
    print(f"  vcpkg:        {toolchain or '未检测到'}")
    print(f"  并行任务:     {args.parallel}")
    print("=" * 64)


def main() -> int:
    cfg = load_config()
    build_cfg = cfg.get("build", {})

    parser = argparse.ArgumentParser(
        description="NexusForce 一键构建与安装",
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog=__doc__,
    )

    # ── 路径 ──
    parser.add_argument(
        "--build-dir",
        type=Path,
        default=DEFAULT_BUILD_DIR,
        help=f"CMake 构建目录（默认: {DEFAULT_BUILD_DIR}）",
    )
    parser.add_argument(
        "--prefix",
        type=Path,
        default=DEFAULT_INSTALL_PREFIX,
        help=f"安装目标前缀（默认: {DEFAULT_INSTALL_PREFIX}）",
    )

    # ── 构建控制 ──
    parser.add_argument(
        "--config", "-c",
        default=DEFAULT_CONFIG,
        choices=["Debug", "Release", "RelWithDebInfo", "MinSizeRel"],
        help=f"构建类型（默认: {DEFAULT_CONFIG}）",
    )
    parser.add_argument(
        "--generator", "-G",
        default=None,
        help="CMake 生成器（默认: 自动检测）",
    )
    parser.add_argument(
        "--parallel", "-j",
        type=int,
        default=None,
        help="并行编译任务数（默认: CMake 自动）",
    )
    parser.add_argument(
        "--target",
        default=None,
        help="仅构建指定目标（默认: 全部）",
    )

    # ── 模式 ──
    parser.add_argument(
        "--release",
        action="store_const", dest="config", const="Release",
        help="Release 模式（等同 -c Release）",
    )
    parser.add_argument(
        "--clean",
        action="store_true",
        help="构建前清空构建目录",
    )
    parser.add_argument(
        "--config-only",
        action="store_true",
        help="仅运行 CMake 配置，不编译",
    )
    parser.add_argument(
        "--build-only",
        action="store_true",
        help="仅编译，不安装",
    )
    parser.add_argument(
        "--verbose", "-v",
        action="store_true",
        help="详细构建输出",
    )

    args = parser.parse_args()

    # ── 检测 toolchain ──
    toolchain = detect_vcpkg_toolchain(cfg)

    # ── 清理 ──
    if args.clean and args.build_dir.exists():
        print(f"[清理] 删除 {args.build_dir} ...")
        shutil.rmtree(args.build_dir)

    args.build_dir.mkdir(parents=True, exist_ok=True)

    # ── 打印摘要 ──
    print_summary(args, toolchain)

    # ── 配置 ──
    rc = cmake_configure(args, toolchain)
    if rc != 0:
        print("\n✘ CMake 配置失败", file=sys.stderr)
        return rc

    if args.config_only:
        print("\n✓ CMake 配置完成（--config-only，跳过构建）")
        return 0

    # ── 构建 ──
    rc = cmake_build(args)
    if rc != 0:
        print("\n✘ 构建失败", file=sys.stderr)
        return rc

    if args.build_only:
        print("\n✓ 构建完成（--build-only，跳过安装）")
        print(f"  产物位于: {args.build_dir}")
        return 0

    # ── 安装 ──
    rc = cmake_install(args)
    if rc != 0:
        print("\n✘ 安装失败", file=sys.stderr)
        return rc

    print(f"\n✓ NexusForce 安装完成 → {args.prefix}")
    print(f"  ├── bin/  可执行文件与 DLL")
    print(f"  ├── lib/  cmake 配置与库文件")
    print(f"  └── include/NeForce/  头文件")
    return 0


if __name__ == "__main__":
    sys.exit(main())
