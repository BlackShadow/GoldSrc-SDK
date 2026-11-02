#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::libwebm::libwebm" for configuration "Debug"
set_property(TARGET unofficial::libwebm::libwebm APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(unofficial::libwebm::libwebm PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/debug/lib/webm.lib"
  )

list(APPEND _cmake_import_check_targets unofficial::libwebm::libwebm )
list(APPEND _cmake_import_check_files_for_unofficial::libwebm::libwebm "${_IMPORT_PREFIX}/debug/lib/webm.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
