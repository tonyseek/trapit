cmake_minimum_required(VERSION 3.22..3.25)
project(trapit)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

add_executable(trapit trapit.cc)
install(TARGETS trapit)
