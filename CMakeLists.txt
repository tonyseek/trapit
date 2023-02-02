cmake_minimum_required(VERSION 3.22..3.25)
project(trapit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(trapit trapit.cc)
install(TARGETS trapit)

set(CPACK_PACKAGE_VENDOR "Jiangge Zhang <tonyseek@gmail.com>")
set(CPACK_PACKAGE_CONTACT "Jiangge Zhang <tonyseek@gmail.com>")
set(CPACK_PACKAGE_DIRECTORY "${CMAKE_BINARY_DIR}/dist")
set(
  CPACK_PACKAGE_DESCRIPTION_SUMMARY "Command line utility that traps \
  short-lived processes to inspect them with PID-aware tools (e.g. strace)")
set(CPACK_PACKAGE_DESCRIPTION_FILE "${CMAKE_SOURCE_DIR}/README.md")
set(CPACK_PACKAGE_HOMEPAGE_URL "https://github.com/tonyseek/trapit")
set(CPACK_STRIP_FILES TRUE)
set(
  CPACK_SOURCE_IGNORE_FILES
  "\\\\.git/" "\\\\.github/" "\\\\.sw[op]$" "/build/")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "libc6 (>= 2.2.5)")
include(CPack)
