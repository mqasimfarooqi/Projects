cmake_minimum_required(VERSION 3.10)

project(DpdkApp C)

# Find the DPDK package
find_package(PkgConfig REQUIRED)
pkg_check_modules(DPDK REQUIRED libdpdk)

# Create the executable
add_executable(DpdkApp main.c)

# Include directories and link libraries
target_include_directories(DpdkApp PRIVATE ${DPDK_INCLUDE_DIRS})
target_link_libraries(DpdkApp PRIVATE ${DPDK_LDFLAGS} pthread)

# Ensure proper compiler flags are used
target_compile_options(DpdkApp PRIVATE ${DPDK_CFLAGS_OTHER})
