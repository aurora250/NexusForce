#!/usr/bin/env bash
#
# Build the target dependencies required by the macOS cross build:
# OpenSSL (static), PCRE2 (static, 8-bit, JIT), zlib (static) and GTest.
#
# The output prefix defaults to <repo>/build-macos-deps for arm64 and
# <repo>/build-macos-deps-x86_64 for x86_64.
#
# Usage:
#   scripts/build_macos_deps.sh [install-prefix]
#
# Environment:
#   NEXUSFORCE_MACOS_ARCH           arm64 (default) or x86_64
#   NEXUSFORCE_MACOS_OSXCROSS_ROOT  osxcross target directory
#   NEXUSFORCE_MACOS_SDK            macOS SDK (default: newest under osxcross)
#   NEXUSFORCE_MACOS_CLANG          host clang >= 19 (default: clang-20/19/clang)
#   NEXUSFORCE_MACOS_DEPLOYMENT_TARGET  default 11.0
#   VCPKG_ROOT                      optional source of the release tarballs
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ARCH="${NEXUSFORCE_MACOS_ARCH:-arm64}"
case "${ARCH}" in
    arm64)
        TRIPLE="arm64-apple-darwin25.5"
        OPENSSL_TARGET="darwin64-arm64-cc"
        DEPS_DEFAULT="${ROOT}/build-macos-deps"
        ;;
    x86_64)
        TRIPLE="x86_64-apple-darwin25.5"
        OPENSSL_TARGET="darwin64-x86_64-cc"
        DEPS_DEFAULT="${ROOT}/build-macos-deps-x86_64"
        ;;
    *)
        echo "error: unsupported NEXUSFORCE_MACOS_ARCH='${ARCH}' (expected arm64 or x86_64)" >&2
        exit 1
        ;;
esac
DEPS="${1:-${DEPS_DEFAULT}}"
SRC_DIR="${ROOT}/build-macos-deps-src-${ARCH}"
OPENSSL_VERSION="3.6.1"
PCRE2_VERSION="10.47"
ZLIB_VERSION="1.3.1"
GTEST_VERSION="1.17.0"
SLJIT_COMMIT="45f910b78c6605ebf5b53d3ec7cb00f2312fe417"
DEPLOYMENT_TARGET="${NEXUSFORCE_MACOS_DEPLOYMENT_TARGET:-11.0}"
JOBS="${JOBS:-$(nproc)}"

OSXCROSS_ROOT="${NEXUSFORCE_MACOS_OSXCROSS_ROOT:-}"
if [ -z "${OSXCROSS_ROOT}" ]; then
    for candidate in "${OSXCROSS_TARGET_DIR:-}" "${HOME}/osxcross/target" "${HOME}/Workspace/osxcross/target" \
        "/opt/osxcross/target" "/usr/local/osxcross/target"; do
        if [ -n "${candidate}" ] && [ -d "${candidate}/SDK" ]; then
            OSXCROSS_ROOT="${candidate}"
            break
        fi
    done
fi
if [ -z "${OSXCROSS_ROOT}" ] || [ ! -d "${OSXCROSS_ROOT}/SDK" ]; then
    echo "error: osxcross not found; set NEXUSFORCE_MACOS_OSXCROSS_ROOT" >&2
    exit 1
fi

SDK="${NEXUSFORCE_MACOS_SDK:-$(ls -d "${OSXCROSS_ROOT}"/SDK/MacOSX*.sdk 2>/dev/null | sort -V | tail -1 || true)}"
if [ -z "${SDK}" ] || [ ! -f "${SDK}/usr/include/stdio.h" ]; then
    echo "error: no usable macOS SDK under ${OSXCROSS_ROOT}/SDK" >&2
    exit 1
fi

CLANG="${NEXUSFORCE_MACOS_CLANG:-$(command -v clang-20 || command -v clang-19 || command -v clang)}"
CLANGXX="${CLANG/clang/clang++}"
if [ ! -x "${CLANG}" ] || [ ! -x "${CLANGXX}" ]; then
    echo "error: clang/clang++ not found (macOS SDK headers need clang >= 19)" >&2
    exit 1
fi

LD="${OSXCROSS_ROOT}/bin/${TRIPLE}-ld"
CROSS_AR="${OSXCROSS_ROOT}/bin/${TRIPLE}-ar"
CROSS_RANLIB="${OSXCROSS_ROOT}/bin/${TRIPLE}-ranlib"
if [ ! -x "${LD}" ] || [ ! -x "${CROSS_AR}" ] || [ ! -x "${CROSS_RANLIB}" ]; then
    echo "error: osxcross binutils not found under ${OSXCROSS_ROOT}/bin" >&2
    exit 1
fi

mkdir -p "${DEPS}" "${SRC_DIR}"
WORK="${SRC_DIR}/work"
mkdir -p "${WORK}"

cat > "${WORK}/macos-clang" <<EOF
#!/bin/sh
exec "${CLANG}" --target=${TRIPLE} --sysroot=${SDK} -mmacosx-version-min=${DEPLOYMENT_TARGET} --ld-path=${LD} "\$@"
EOF
cat > "${WORK}/macos-clang++" <<EOF
#!/bin/sh
exec "${CLANGXX}" --target=${TRIPLE} --sysroot=${SDK} -mmacosx-version-min=${DEPLOYMENT_TARGET} --ld-path=${LD} "\$@"
EOF
chmod +x "${WORK}/macos-clang" "${WORK}/macos-clang++"

fetch() {
    local name="$1" url="$2"
    if [ -f "${SRC_DIR}/${name}" ]; then
        return
    fi
    if [ -n "${VCPKG_ROOT:-}" ] && [ -f "${VCPKG_ROOT}/downloads/${name}" ]; then
        cp "${VCPKG_ROOT}/downloads/${name}" "${SRC_DIR}/${name}"
        return
    fi
    echo "downloading ${name}"
    curl -fL --retry 3 -o "${SRC_DIR}/${name}" "${url}"
}

fetch "openssl-openssl-openssl-${OPENSSL_VERSION}.tar.gz" \
    "https://github.com/openssl/openssl/releases/download/openssl-${OPENSSL_VERSION}/openssl-${OPENSSL_VERSION}.tar.gz"
fetch "PCRE2Project-pcre2-pcre2-${PCRE2_VERSION}.tar.gz" \
    "https://github.com/PCRE2Project/pcre2/releases/download/pcre2-${PCRE2_VERSION}/pcre2-${PCRE2_VERSION}.tar.gz"
fetch "sljit-${SLJIT_COMMIT}.tar.gz" \
    "https://github.com/zherczeg/sljit/archive/${SLJIT_COMMIT}.tar.gz"
fetch "madler-zlib-v${ZLIB_VERSION}.tar.gz" \
    "https://github.com/madler/zlib/releases/download/v${ZLIB_VERSION}/zlib-${ZLIB_VERSION}.tar.gz"
fetch "google-googletest-v${GTEST_VERSION}.tar.gz" \
    "https://github.com/google/googletest/archive/refs/tags/v${GTEST_VERSION}.tar.gz"

rm -rf "${SRC_DIR}/openssl" "${SRC_DIR}/pcre2" "${SRC_DIR}/zlib" "${SRC_DIR}/googletest"
mkdir -p "${SRC_DIR}/openssl" "${SRC_DIR}/pcre2" "${SRC_DIR}/zlib" "${SRC_DIR}/googletest"
tar -xzf "${SRC_DIR}/openssl-openssl-openssl-${OPENSSL_VERSION}.tar.gz" -C "${SRC_DIR}/openssl" --strip-components=1
tar -xzf "${SRC_DIR}/PCRE2Project-pcre2-pcre2-${PCRE2_VERSION}.tar.gz" -C "${SRC_DIR}/pcre2" --strip-components=1

# PCRE2's release tarball does not bundle sljit; JIT needs the sljit sources.
rm -rf "${SRC_DIR}/pcre2/deps/sljit"
mkdir -p "${SRC_DIR}/pcre2/deps/sljit"
tar -xzf "${SRC_DIR}/sljit-${SLJIT_COMMIT}.tar.gz" -C "${SRC_DIR}/pcre2/deps/sljit" --strip-components=1
tar -xzf "${SRC_DIR}/madler-zlib-v${ZLIB_VERSION}.tar.gz" -C "${SRC_DIR}/zlib" --strip-components=1
tar -xzf "${SRC_DIR}/google-googletest-v${GTEST_VERSION}.tar.gz" -C "${SRC_DIR}/googletest" --strip-components=1

echo "building OpenSSL ${OPENSSL_VERSION} for ${TRIPLE}"
(
    cd "${SRC_DIR}/openssl"
    CC="${WORK}/macos-clang" CXX="${WORK}/macos-clang++" \
        ./Configure "${OPENSSL_TARGET}" no-shared no-module no-tests no-apps \
        --prefix="${DEPS}" --openssldir="${DEPS}/ssl"
    make -j"${JOBS}"
    make install_sw
)

echo "building PCRE2 ${PCRE2_VERSION} for ${TRIPLE}"
cmake -S "${SRC_DIR}/pcre2" -B "${SRC_DIR}/pcre2/build" -G Ninja \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DCMAKE_SYSTEM_PROCESSOR="${ARCH}" \
    -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
    -DCMAKE_OSX_SYSROOT="${SDK}" \
    -DCMAKE_C_COMPILER="${WORK}/macos-clang" \
    -DCMAKE_CXX_COMPILER="${WORK}/macos-clang++" \
    -DCMAKE_AR="${CROSS_AR}" \
    -DCMAKE_RANLIB="${CROSS_RANLIB}" \
    -DCMAKE_INSTALL_PREFIX="${DEPS}" \
    -DBUILD_SHARED_LIBS=OFF \
    -DPCRE2_BUILD_PCRE2_8=ON \
    -DPCRE2_BUILD_PCRE2_16=OFF \
    -DPCRE2_BUILD_PCRE2_32=OFF \
    -DPCRE2_BUILD_TESTS=OFF \
    -DPCRE2_BUILD_PCRE2GREP=OFF \
    -DPCRE2_SUPPORT_JIT=ON
cmake --build "${SRC_DIR}/pcre2/build" -j"${JOBS}"
cmake --install "${SRC_DIR}/pcre2/build"

echo "building zlib ${ZLIB_VERSION} for ${TRIPLE}"
cmake -S "${SRC_DIR}/zlib" -B "${SRC_DIR}/zlib/build" -G Ninja \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DCMAKE_SYSTEM_PROCESSOR="${ARCH}" \
    -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
    -DCMAKE_OSX_SYSROOT="${SDK}" \
    -DCMAKE_C_COMPILER="${WORK}/macos-clang" \
    -DCMAKE_AR="${CROSS_AR}" \
    -DCMAKE_RANLIB="${CROSS_RANLIB}" \
    -DCMAKE_INSTALL_PREFIX="${DEPS}" \
    -DBUILD_SHARED_LIBS=OFF
cmake --build "${SRC_DIR}/zlib/build" -j"${JOBS}"
cmake --install "${SRC_DIR}/zlib/build"
# zlib's CMake project always builds a shared library as well; the cross build
# links the static archive so the result has no absolute dependency on the
# dependency prefix.
rm -f "${DEPS}"/lib/libz*.dylib

echo "building GTest ${GTEST_VERSION} for ${TRIPLE}"
cmake -S "${SRC_DIR}/googletest" -B "${SRC_DIR}/googletest/build" -G Ninja \
    -DCMAKE_SYSTEM_NAME=Darwin \
    -DCMAKE_SYSTEM_PROCESSOR="${ARCH}" \
    -DCMAKE_OSX_ARCHITECTURES="${ARCH}" \
    -DCMAKE_OSX_SYSROOT="${SDK}" \
    -DCMAKE_C_COMPILER="${WORK}/macos-clang" \
    -DCMAKE_CXX_COMPILER="${WORK}/macos-clang++" \
    -DCMAKE_AR="${CROSS_AR}" \
    -DCMAKE_RANLIB="${CROSS_RANLIB}" \
    -DCMAKE_INSTALL_PREFIX="${DEPS}" \
    -DBUILD_GMOCK=OFF \
    -DINSTALL_GTEST=ON
cmake --build "${SRC_DIR}/googletest/build" -j"${JOBS}"
cmake --install "${SRC_DIR}/googletest/build"

# OpenSSL installs GNU archives; cctools' ld64 rejects their long-name table.
for archive in "${DEPS}"/lib/*.a; do
    [ -e "${archive}" ] || continue
    repack="${WORK}/repack-$(basename "${archive}" .a)"
    rm -rf "${repack}"
    mkdir -p "${repack}"
    (
        cd "${repack}"
        ar x "${archive}"
        rm -f "${archive}"
        "${CROSS_AR}" qc "${archive}" ./*.o
        "${CROSS_RANLIB}" "${archive}"
    )
done

echo "target dependencies installed in ${DEPS}"
