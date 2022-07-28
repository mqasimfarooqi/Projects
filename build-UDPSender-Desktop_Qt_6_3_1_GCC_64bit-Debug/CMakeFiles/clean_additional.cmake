# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles/UDPSender_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/UDPSender_autogen.dir/ParseCache.txt"
  "UDPSender_autogen"
  )
endif()
