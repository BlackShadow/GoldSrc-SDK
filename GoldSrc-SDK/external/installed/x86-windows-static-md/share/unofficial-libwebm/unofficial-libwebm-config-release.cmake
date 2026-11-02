#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "unofficial::libwebm::libwebm" for configuration "Release"
set_property(TARGET unofficial::libwebm::libwebm APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(unofficial::libwebm::libwebm PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_RELEASE "CXX"
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/lib/webm.lib"
  )

list(APPEND _cmake_import_check_targets unofficial::libwebm::libwebm )
list(APPEND _cmake_import_check_files_for_unofficial::libwebm::libwebm "${_IMPORT_PREFIX}/lib/webm.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
