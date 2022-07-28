# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/SocketApp_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/SocketApp_autogen.dir/ParseCache.txt"
  "SocketApp_autogen"
  )
endif()
