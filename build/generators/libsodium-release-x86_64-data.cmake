########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

set(libsodium_COMPONENT_NAMES "")
if(DEFINED libsodium_FIND_DEPENDENCY_NAMES)
  list(APPEND libsodium_FIND_DEPENDENCY_NAMES )
  list(REMOVE_DUPLICATES libsodium_FIND_DEPENDENCY_NAMES)
else()
  set(libsodium_FIND_DEPENDENCY_NAMES )
endif()

########### VARIABLES #######################################################################
#############################################################################################
set(libsodium_PACKAGE_FOLDER_RELEASE "C:/Users/prm/.conan2/p/libsod7e5db4c9d86c/p")
set(libsodium_BUILD_MODULES_PATHS_RELEASE )


set(libsodium_INCLUDE_DIRS_RELEASE )
set(libsodium_RES_DIRS_RELEASE )
set(libsodium_DEFINITIONS_RELEASE "-DSODIUM_STATIC")
set(libsodium_SHARED_LINK_FLAGS_RELEASE )
set(libsodium_EXE_LINK_FLAGS_RELEASE )
set(libsodium_OBJECTS_RELEASE )
set(libsodium_COMPILE_DEFINITIONS_RELEASE "SODIUM_STATIC")
set(libsodium_COMPILE_OPTIONS_C_RELEASE )
set(libsodium_COMPILE_OPTIONS_CXX_RELEASE )
set(libsodium_LIB_DIRS_RELEASE "${libsodium_PACKAGE_FOLDER_RELEASE}/lib")
set(libsodium_BIN_DIRS_RELEASE )
set(libsodium_LIBRARY_TYPE_RELEASE STATIC)
set(libsodium_IS_HOST_WINDOWS_RELEASE 1)
set(libsodium_LIBS_RELEASE libsodium)
set(libsodium_SYSTEM_LIBS_RELEASE )
set(libsodium_FRAMEWORK_DIRS_RELEASE )
set(libsodium_FRAMEWORKS_RELEASE )
set(libsodium_BUILD_DIRS_RELEASE )
set(libsodium_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(libsodium_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${libsodium_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${libsodium_COMPILE_OPTIONS_C_RELEASE}>")
set(libsodium_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${libsodium_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${libsodium_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${libsodium_EXE_LINK_FLAGS_RELEASE}>")


set(libsodium_COMPONENTS_RELEASE )