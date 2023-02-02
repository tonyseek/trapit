#!/bin/sh

set -e

SOURCE_DIR="$(pwd)"
BUILD_DIR="${SOURCE_DIR}/build"
BUILD_BIN="trapit"
PACK_DIR="${SOURCE_DIR}/build/dist"

if ! [ -f "${SOURCE_DIR}/CMakeLists.txt" ]; then
  printf >&2 'ERROR: %s is not the source directory which includes %s' \
    "${SOURCE_DIR}" "CMakeLists.txt"
  exit 2
fi

cmake_configure() {
  cmake -S "${SOURCE_DIR}" -B "${BUILD_DIR}" "$@"
}

cmake_build() {
  cmake --build "${BUILD_DIR}"
}

cmd_help() {
  printf >&2 'Usage: %s [help|clean|build|run]\n' "$0"
}

cmd_clean() {
  set -x
  rm -rf "${BUILD_DIR}"
}

cmd_build() {
  BUILD_TYPE="${1:-Release}"
  set -x
  if [ -z "$(command -v ninja)" ]; then
    cmake_configure -DCMAKE_BUILD_TYPE="$BUILD_TYPE"
  else
    cmake_configure -DCMAKE_BUILD_TYPE="$BUILD_TYPE" -G Ninja
  fi
  cmake_build
}

cmd_pack() {
  cmd_clean
  cmd_build
  (
    cd "${BUILD_DIR}"
    cpack -G TXZ --config CPackSourceConfig.cmake
    cpack -G TXZ
    if command -v dpkg > /dev/null; then
      cpack -G DEB
    else
      printf >&2 '=> Skip to pack DEB because lack of "dpkg" (dpkg)\n'
    fi
    if command -v rpmbuild > /dev/null; then
      cpack -G RPM
    else
      printf >&2 '=> Skip to pack RPM because lack of "rpm-tools" (rpmbuild)\n'
    fi
    rm -rf "${PACK_DIR}/_CPack_Packages"
  )
}

cmd_run() {
  cmd_build
  exec "${BUILD_DIR}/${BUILD_BIN}" "$@"
}

cmd_dbg() {
  cmd_build Debug
  if command -v gdb > /dev/null; then
    exec gdb --args "${BUILD_DIR}/${BUILD_BIN}" "$@"
  else
    exec lldb -- "${BUILD_DIR}/${BUILD_BIN}" "$@"
  fi
}

case "$1" in
  help)
    cmd_help
    ;;
  clean)
    cmd_clean
    ;;
  build)
    cmd_build
    ;;
  pack)
    cmd_pack
    ;;
  run)
    shift
    cmd_run "$@"
    ;;
  dbg)
    shift
    cmd_dbg "$@"
    ;;
  *)
    cmd_help
    exit 2
    ;;
esac
