#!/usr/bin/env bash
#
# Run an x86_64 macOS test binary on a Linux host through Darling.
#
# Darling mounts the host root at /Volumes/SystemRoot, so the absolute dylib
# path embedded by the build tree cannot be resolved.  This script copies the
# test binary, rewrites its libNexusForce dependency to @rpath and adds the
# matching @loader_path rpath before starting Darling.
#
# Usage:
#   scripts/run_darling_tests.sh [build-dir] [gtest arguments...]
#
# Example:
#   scripts/run_darling_tests.sh build-macos-x86_64 --gtest_filter=ThreadTest.*
#
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD="${1:-${ROOT}/build-macos-x86_64}"
if [ $# -gt 0 ]; then
    shift
fi
BUILD="$(realpath "${BUILD}")"

BIN="$(find "${BUILD}/bin" -maxdepth 2 -type f -name NexusForceUnitTests -printf '%T@ %p\n' 2>/dev/null | sort -nr | head -1 | cut -d' ' -f2-)"
if [ -z "${BIN}" ]; then
    echo "error: NexusForceUnitTests not found under ${BUILD}/bin" >&2
    echo "build it with: cmake --build ${BUILD} --target NexusForceUnitTests" >&2
    exit 1
fi

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
INSTALL_NAME_TOOL="${OSXCROSS_ROOT}/bin/x86_64-apple-darwin25.5-install_name_tool"
OTOOL="${OSXCROSS_ROOT}/bin/x86_64-apple-darwin25.5-otool"
if [ ! -x "${INSTALL_NAME_TOOL}" ] || [ ! -x "${OTOOL}" ]; then
    echo "error: osxcross x86_64 tools not found; set NEXUSFORCE_MACOS_OSXCROSS_ROOT" >&2
    exit 1
fi

DEP="$("${OTOOL}" -L "${BIN}" | awk '/libNexusForce/{print $1; exit}')"
if [ -z "${DEP}" ]; then
    echo "error: ${BIN} does not depend on libNexusForce" >&2
    exit 1
fi

PATCHED="${BIN}.darling"
cp "${BIN}" "${PATCHED}"
"${INSTALL_NAME_TOOL}" -change "${DEP}" "@rpath/$(basename "${DEP}")" "${PATCHED}"
RELATIVE_LIB="$(realpath --relative-to="$(dirname "${PATCHED}")" "$(dirname "${DEP}")")"
"${INSTALL_NAME_TOOL}" -add_rpath "@loader_path/${RELATIVE_LIB}" "${PATCHED}" 2>/dev/null || true

# Tests resolve these paths through the compile-time absolute path; inside
# Darling the host root is mounted at /Volumes/SystemRoot, so expose overrides.
if [ -z "${NEFORCE_SNAPSHOT_DIR:-}" ] && [ -d "${ROOT}/tests/unit/snapshots" ]; then
    export NEFORCE_SNAPSHOT_DIR="/Volumes/SystemRoot${ROOT}/tests/unit/snapshots"
fi

PLUGIN="$(find "${BUILD}/lib" -type f -path '*/plugins/*' 2>/dev/null | head -1)"
if [ -n "${PLUGIN}" ]; then
    PLUGIN_DEP="$("${OTOOL}" -L "${PLUGIN}" | awk '/libNexusForce/{print $1; exit}')"
    if [ -n "${PLUGIN_DEP}" ]; then
        "${INSTALL_NAME_TOOL}" -change "${PLUGIN_DEP}" "@rpath/$(basename "${PLUGIN_DEP}")" "${PLUGIN}"
        RELATIVE_PLUGIN_LIB="$(realpath --relative-to="$(dirname "${PLUGIN}")" "$(dirname "${PLUGIN_DEP}")")"
        "${INSTALL_NAME_TOOL}" -add_rpath "@loader_path/${RELATIVE_PLUGIN_LIB}" "${PLUGIN}" 2>/dev/null || true
    fi
    export NEFORCE_TEST_PLUGIN_PATH="/Volumes/SystemRoot${PLUGIN}"
fi

export NEFORCE_DARLING=1

exec darling "${PATCHED}" "$@"
