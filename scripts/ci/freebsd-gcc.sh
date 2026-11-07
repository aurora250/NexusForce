#!/usr/bin/env bash
#
# Native FreeBSD GCC build + unit test run inside a booted QEMU VM.
#
# The VM must already be reachable over SSH and the host tree is synced into
# /home/tester/nf/src-gcc; the clang-built vcpkg tree is synced into
# /home/tester/nf/vcpkg-gcc so GCC can link a libc++-compatible dependency set.
#
# usage: freebsd-gcc.sh <x64|arm64> <vm-dir> <ssh-port> <host-src-dir> <vcpkg-installed-dir> [triplet]
#
set -euo pipefail

arch=${1:?architecture}
vm_dir=${2:?vm directory}
port=${3:-2222}
host_src=${4:?host source directory}
host_vcpkg=${5:?host vcpkg installed directory}
triplet=${6:-$([ "${arch}" = arm64 ] && echo arm64-freebsd || echo x64-freebsd)}
key="${vm_dir}/id_ed25519"
if [ ! -f "${key}" ]; then
    key="${NEXUSFORCE_FREEBSD_SSH_KEY:-${HOME}/.ssh/nf-fbsd-vm}"
fi
runtime_key=$(mktemp)
cp "${key}" "${runtime_key}"
chmod 600 "${runtime_key}"
trap 'rm -f "${runtime_key}"' EXIT

ssh_opts=(-i "${runtime_key}" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null
          -o LogLevel=ERROR -o ConnectTimeout=5 -p "${port}")
SSH=(ssh "${ssh_opts[@]}")
SCP=(scp -i "${runtime_key}" -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null
     -o LogLevel=ERROR -P "${port}")
RSYNC=(rsync -az --delete --exclude=.git --exclude='cmake-build-*' --exclude='build-*'
       --exclude='vcpkg_installed' --exclude='.ci' --exclude='.idea'
       -e "ssh -i ${runtime_key} -o StrictHostKeyChecking=no -o UserKnownHostsFile=/dev/null -o LogLevel=ERROR -p ${port}")

remote_root=/home/tester/nf
remote_src="${remote_root}/src-gcc"
remote_vcpkg="${remote_root}/vcpkg-gcc"
remote_build="${remote_root}/build-gcc"

"${SSH[@]}" root@127.0.0.1 '
    set -e
    missing=""
    for tool in gcc13 g++13 cmake ninja rsync; do
        command -v "$tool" >/dev/null 2>&1 || missing="$missing $tool"
    done
    if [ -n "$missing" ]; then
        pkg install -y gcc13 cmake ninja rsync
    fi
'
"${SSH[@]}" tester@127.0.0.1 "mkdir -p ${remote_src} ${remote_vcpkg}"

"${RSYNC[@]}" "${host_src}/" "tester@127.0.0.1:${remote_src}/"
"${RSYNC[@]}" "${host_vcpkg}/" "tester@127.0.0.1:${remote_vcpkg}/"

# vcpkg's static libpq/ICU archives need their internal helper archives; add
# them to the shared library link line of the synced source tree.
patched=$(mktemp)
python3 - "${host_src}" "${patched}" <<'PATCH'
import sys

src, dst = sys.argv[1], sys.argv[2]
text = open(f"{src}/src/CMakeLists.txt", encoding="utf-8").read()
pg = "        target_link_libraries(NexusForce PUBLIC PostgreSQL::PostgreSQL)\n"
assert text.count(pg) == 1
text = text.replace(pg, pg + (
    '        get_filename_component(_pg_lib_dir "${PostgreSQL_LIBRARY}" DIRECTORY)\n'
    '        target_link_libraries(NexusForce PUBLIC "${_pg_lib_dir}/libpgcommon.a" "${_pg_lib_dir}/libpgport.a")\n'
))
icu = "            target_link_libraries(NexusForce PUBLIC ICU::uc ICU::i18n)\n"
assert text.count(icu) == 1
text = text.replace(icu, icu + (
    '            get_filename_component(_icu_lib_dir "${ICU_UC_LIBRARY_RELEASE}" DIRECTORY)\n'
    '            target_link_libraries(NexusForce PUBLIC "${_icu_lib_dir}/libicudata.a")\n'
))
open(dst, "w", encoding="utf-8").write(text)
PATCH
"${SCP[@]}" "${patched}" "tester@127.0.0.1:${remote_src}/src/CMakeLists.txt"
rm -f "${patched}"

"${SSH[@]}" tester@127.0.0.1 "
    set -e
    cd ${remote_root}
    rm -rf ${remote_build}
    cmake -S ${remote_src} -B ${remote_build} -G Ninja \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_TOOLCHAIN_FILE=${remote_src}/cmake/toolchains/freebsd-gcc.cmake \
        '-DCMAKE_PREFIX_PATH=${remote_vcpkg}/${triplet};${remote_vcpkg}/${triplet}/debug' \
        -DNEXUSFORCE_BUILD_DOCS=OFF \
        -DNEXUSFORCE_BUILD_EXAMPLES=OFF \
        -DNEXUSFORCE_BUILD_BENCHMARKS=OFF
    cmake --build ${remote_build} --target NexusForceUnitTests --parallel 2
"

set +e
"${SSH[@]}" tester@127.0.0.1 "
    cd ${remote_build}
    ulimit -c 0
    LD_LIBRARY_PATH=lib/Release timeout 3600 ./bin/Release/NexusForceUnitTests \
        --gtest_color=no --gtest_output=xml:/tmp/freebsd-gcc-results.xml
"
status=$?
set -e

"${SCP[@]}" "tester@127.0.0.1:/tmp/freebsd-gcc-results.xml" \
    "freebsd-gcc-${arch}-test-results.xml" || true

exit "${status}"
