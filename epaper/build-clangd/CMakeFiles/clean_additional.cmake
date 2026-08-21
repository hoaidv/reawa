# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "")
  file(REMOVE_RECURSE
  "CMakeFiles/epaper_bin_autogen.dir/AutogenUsed.txt"
  "CMakeFiles/epaper_bin_autogen.dir/ParseCache.txt"
  "epaper_bin_autogen"
  )
endif()
