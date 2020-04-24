
foreach(substring LIB BIN INCLUDE CMAKE)  
  set(var INSTALL_${substring}_DIR)
  if(NOT IS_ABSOLUTE ${${var}})
    set(${var} "${CMAKE_INSTALL_PREFIX}/${${var}}")
    message(STATUS "${var}: "  "${${var}}")
  endif()  
endforeach()

set(PROJECT_CMAKE_FILES ${PROJECT_BINARY_DIR}${CMAKE_FILES_DIRECTORY})