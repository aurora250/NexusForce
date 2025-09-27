#!/bin/bash
set -e

if [ "$(id -u)" -ne 0 ]; then
    echo "❌ 请使用 sudo 权限运行此脚本"
    exit 1
fi

echo "=============================================="
echo "          MSTL 第三方依赖库安装脚本              "
echo "          适用系统：Ubuntu 20.04+/Debian 11+   "
echo "=============================================="
echo -e "\n🔄 正在更新系统包列表..."
apt update -y


echo -e "\n🔧 安装基础编译工具链..."
apt install -y \
    build-essential \          # GCC、G++、Make
    clang clang-format \       # Clang
    cmake ninja-build \        # CMake
    gdb
    pkg-config


echo -e "\n📦 安装 MSTL 核心依赖库..."
apt install -y \
    # Boost
    libboost-system-dev \
    libboost-filesystem-dev \
    \
    # MySQL
    libmysqlclient-dev \
    mysql-client \
    \
    # SQLite3
    libsqlite3-dev \
    \
    # Redis
    libhiredis-dev \
    \
    # Qt6
    qt6-base-dev \             # Qt6 Core/Widgets/Gui
    libgl1-mesa-dev \          # OpenGL
    qt6-tools-dev \            # Qt6 工具链

echo -e "\n=============================================="
echo "✅ 所有 MSTL 依赖库安装完成！"
echo "=============================================="
