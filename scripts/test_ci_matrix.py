#!/usr/bin/env python3
"""本地 CI 矩阵测试 — 在 Windows 上验证所有 MSVC / Clang-CL 配置的编译。

用法:
    python scripts/test_ci_matrix.py              # 全部 5 个配置
    python scripts/test_ci_matrix.py --keep       # 保留每个配置的 build 目录
    python scripts/test_ci_matrix.py --config msvc-full  # 只跑指定配置
"""

import argparse
import os
import shutil
import subprocess
import sys
import time
from pathlib import Path

PROJECT_ROOT = Path(__file__).resolve().parent.parent
TEST_ROOT = PROJECT_ROOT / "build" / "ci-test"

VCPKG_ROOT = Path(os.environ.get("VCPKG_ROOT", "D:/vcpkg"))
TOOLCHAIN = VCPKG_ROOT / "scripts" / "buildsystems" / "vcpkg.cmake"

# ── CI 矩阵定义 ────────────────────────────────────────────────────────────────
MATRIX = [
    {
        "id": "msvc-full",
        "generator": "Visual Studio 17 2022",
        "toolset": "",
        "cxx_std": 20,
        "flags": [
            "-DNEXUSFORCE_BUILD_DOCS=OFF",
        ],
    },
    {
        "id": "msvc-minimal",
        "generator": "Visual Studio 17 2022",
        "toolset": "",
        "cxx_std": 14,
        "flags": [
            "-DNEXUSFORCE_BUILD_DOCS=OFF",
            "-DNEXUSFORCE_ENABLE_POSTGRESQL=OFF",
            "-DNEXUSFORCE_ENABLE_MYSQL=OFF",
            "-DNEXUSFORCE_ENABLE_SQLITE3=OFF",
            "-DNEXUSFORCE_ENABLE_REDIS=OFF",
            "-DNEXUSFORCE_ENABLE_ZLIB=OFF",
            "-DNEXUSFORCE_ENABLE_LZ4=OFF",
            "-DNEXUSFORCE_USING_SSO=OFF",
            "-DNEXUSFORCE_USING_INTEL_TSX=OFF",
            "-DNEXUSFORCE_USING_SQLCIPHER=OFF",
            "-DNEXUSFORCE_BUILD_TESTS=OFF",
            "-DNEXUSFORCE_BUILD_EXAMPLES=OFF",
            "-DNEXUSFORCE_BUILD_BENCHMARKS=OFF",
        ],
    },
    {
        "id": "msvc-sqlcipher",
        "generator": "Visual Studio 17 2022",
        "toolset": "",
        "cxx_std": 20,
        "flags": [
            "-DNEXUSFORCE_BUILD_DOCS=OFF",
            "-DNEXUSFORCE_USING_SQLCIPHER=ON",
            "-DNEXUSFORCE_BUILD_TESTS=OFF",
            "-DNEXUSFORCE_BUILD_EXAMPLES=OFF",
            "-DNEXUSFORCE_BUILD_BENCHMARKS=OFF",
        ],
    },
    {
        "id": "clangcl-full",
        "generator": "Ninja",
        "toolset": "",
        "cxx_std": 20,
        "compiler_cc": "clang-cl",
        "compiler_cxx": "clang-cl",
        "flags": [
            "-DNEXUSFORCE_BUILD_DOCS=OFF",
        ],
    },
    {
        "id": "clangcl-minimal",
        "generator": "Ninja",
        "toolset": "",
        "cxx_std": 17,
        "compiler_cc": "clang-cl",
        "compiler_cxx": "clang-cl",
        "flags": [
            "-DNEXUSFORCE_BUILD_DOCS=OFF",
            "-DNEXUSFORCE_ENABLE_POSTGRESQL=OFF",
            "-DNEXUSFORCE_ENABLE_MYSQL=OFF",
            "-DNEXUSFORCE_ENABLE_SQLITE3=OFF",
            "-DNEXUSFORCE_ENABLE_REDIS=OFF",
            "-DNEXUSFORCE_ENABLE_ZLIB=OFF",
            "-DNEXUSFORCE_ENABLE_LZ4=OFF",
            "-DNEXUSFORCE_USING_SSO=OFF",
            "-DNEXUSFORCE_USING_INTEL_TSX=OFF",
            "-DNEXUSFORCE_USING_SQLCIPHER=OFF",
            "-DNEXUSFORCE_BUILD_TESTS=OFF",
            "-DNEXUSFORCE_BUILD_EXAMPLES=OFF",
            "-DNEXUSFORCE_BUILD_BENCHMARKS=OFF",
        ],
    },
]

# ── 颜色 ───────────────────────────────────────────────────────────────────────
GREEN = "\033[0;32m"
RED = "\033[0;31m"
CYAN = "\033[0;36m"
YELLOW = "\033[1;33m"
NC = "\033[0m"


def run(cmd: list[str], cwd: Path | None = None) -> tuple[int, str]:
    """运行命令，返回 (返回码, 输出)。"""
    try:
        result = subprocess.run(
            cmd, cwd=cwd, capture_output=True, text=True,
            encoding="utf-8", errors="replace",
            timeout=600,  # 10 min timeout per step
        )
        return result.returncode, result.stdout + "\n" + result.stderr
    except subprocess.TimeoutExpired:
        return -1, "[超时]"
    except Exception as e:
        return -2, str(e)


def main() -> int:
    # 强制 UTF-8 输出，避免 Windows GBK 终端 Unicode 崩溃
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8", errors="replace")

    parser = argparse.ArgumentParser(description="本地 CI 矩阵测试")
    parser.add_argument("--keep", action="store_true",
                        help="保留构建目录（默认清空）")
    parser.add_argument("--config", default=None,
                        help="仅运行指定配置（如 msvc-full）")
    args = parser.parse_args()

    if not TOOLCHAIN.exists():
        print(f"{RED}错误: vcpkg toolchain 未找到: {TOOLCHAIN}{NC}")
        print(f"请设置 VCPKG_ROOT 环境变量")
        return 1

    # 筛选配置
    entries = [e for e in MATRIX
               if args.config is None or e["id"] == args.config]
    if not entries:
        print(f"{RED}未找到配置: {args.config}{NC}")
        return 1

    os.environ.setdefault("VSLANG", "1033")

    results: dict[str, bool] = {}
    start_all = time.monotonic()

    for entry in entries:
        eid = entry["id"]
        build_dir = TEST_ROOT / eid
        gen = entry["generator"]
        toolset = entry["toolset"]
        cxx_std = entry["cxx_std"]
        flags = entry["flags"]

        desc = f"{gen}"
        if toolset:
            desc += f" / {toolset}"
        desc += f" / C++{cxx_std}"

        print(f"\n{CYAN}{'=' * 68}{NC}")
        print(f"{CYAN}  {eid}  |  {desc}{NC}")
        print(f"{CYAN}{'=' * 68}{NC}")

        if not args.keep and build_dir.exists():
            shutil.rmtree(build_dir)
        build_dir.mkdir(parents=True, exist_ok=True)

        # ── 配置 ──
        t0 = time.monotonic()
        print(f"  -> CMake 配置 ...")

        cmake_cmd = [
            "cmake",
            "-S", str(PROJECT_ROOT),
            "-B", str(build_dir),
            "-G", gen,
            f"-DCMAKE_TOOLCHAIN_FILE={TOOLCHAIN}",
            f"-DCMAKE_CXX_STANDARD={cxx_std}",
        ]
        # VS generator specific
        if "Visual Studio" in gen:
            cmake_cmd += ["-A", "x64"]
        if toolset:
            cmake_cmd += ["-T", toolset]
        # Explicit compiler (Ninja)
        if cc := entry.get("compiler_cc"):
            cmake_cmd += [f"-DCMAKE_C_COMPILER={cc}"]
        if cxx := entry.get("compiler_cxx"):
            cmake_cmd += [f"-DCMAKE_CXX_COMPILER={cxx}"]
        cmake_cmd += flags

        rc, output = run(cmake_cmd)
        t1 = time.monotonic()

        if rc != 0:
            # 提取最后几行错误
            err_lines = [l for l in output.splitlines()
                         if "error" in l.lower() or "Error" in l or "fatal" in l.lower()]
            preview = "\n".join(err_lines[-10:]) or output[-800:]
            print(f"  {RED}[FAIL] 配置失败 ({t1 - t0:.0f}s){NC}")
            print(f"  {YELLOW}{preview}{NC}")
            results[eid] = False
            continue

        print(f"  {GREEN}[OK] CMake 配置 ({t1 - t0:.0f}s){NC}")

        # ── 构建 ──
        print(f"  -> 编译 ...")
        rc, output = run([
            "cmake", "--build", str(build_dir),
            "--config", "Release",
            "--parallel",
        ])
        t2 = time.monotonic()

        if rc != 0:
            err_lines = [l for l in output.splitlines()
                         if "error" in l.lower() or "Error" in l]
            preview = "\n".join(err_lines[-10:]) or output[-800:]
            print(f"  {RED}[FAIL] 编译失败 ({t2 - t1:.0f}s){NC}")
            print(f"  {YELLOW}{preview}{NC}")
            results[eid] = False
            continue

        print(f"  {GREEN}[OK] 编译成功 ({t2 - t1:.0f}s){NC}")
        results[eid] = True

    # ── 汇总 ──
    elapsed = time.monotonic() - start_all
    passed = sum(1 for v in results.values() if v)
    failed = sum(1 for v in results.values() if not v)

    print(f"\n{'=' * 68}")
    print(f"  结果汇总  ({elapsed:.0f}s)")
    print(f"{'=' * 68}")
    for eid, ok in results.items():
        tag = f"{GREEN}[OK] 通过{NC}" if ok else f"{RED}[FAIL] 失败{NC}"
        print(f"  {tag}  {eid}")
    print(f"{'=' * 68}")
    print(f"  通过: {GREEN}{passed}{NC}  /  失败: {RED}{failed}{NC}  /  总计: {passed + failed}")
    print(f"{'=' * 68}")

    return 0 if failed == 0 else 1


if __name__ == "__main__":
    sys.exit(main())
