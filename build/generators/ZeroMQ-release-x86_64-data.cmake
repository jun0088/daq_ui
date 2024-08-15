########### AGGREGATED COMPONENTS AND DEPENDENCIES FOR THE MULTI CONFIG #####################
#############################################################################################

list(APPEND zeromq_COMPONENT_NAMES libzmq-static)
list(REMOVE_DUPLICATES zeromq_COMPONENT_NAMES)
if(DEFINED zeromq_FIND_DEPENDENCY_NAMES)
  list(APPEND zeromq_FIND_DEPENDENCY_NAMES libsodium)
  list(REMOVE_DUPLICATES zeromq_FIND_DEPENDENCY_NAMES)
else()
  set(zeromq_FIND_DEPENDENCY_NAMES libsodium)
endif()
set(libsodium_FIND_MODE "NO_MODULE")

########### VARIABLES #######################################################################
#############################################################################################
set(zeromq_PACKAGE_FOLDER_RELEASE "C:/Users/prm/.conan2/p/zerom79d983c7bec8c/p")
set(zeromq_BUILD_MODULES_PATHS_RELEASE )


set(zeromq_INCLUDE_DIRS_RELEASE "${zeromq_PACKAGE_FOLDER_RELEASE}/include")
set(zeromq_RES_DIRS_RELEASE )
set(zeromq_DEFINITIONS_RELEASE "-DZMQ_STATIC")
set(zeromq_SHARED_LINK_FLAGS_RELEASE )
set(zeromq_EXE_LINK_FLAGS_RELEASE )
set(zeromq_OBJECTS_RELEASE )
set(zeromq_COMPILE_DEFINITIONS_RELEASE "ZMQ_STATIC")
set(zeromq_COMPILE_OPTIONS_C_RELEASE )
set(zeromq_COMPILE_OPTIONS_CXX_RELEASE )
set(zeromq_LIB_DIRS_RELEASE "${zeromq_PACKAGE_FOLDER_RELEASE}/lib")
set(zeromq_BIN_DIRS_RELEASE )
set(zeromq_LIBRARY_TYPE_RELEASE STATIC)
set(zeromq_IS_HOST_WINDOWS_RELEASE 1)
set(zeromq_LIBS_RELEASE libzmq-v143-mt-s-4_3_4)
set(zeromq_SYSTEM_LIBS_RELEASE iphlpapi ws2_32)
set(zeromq_FRAMEWORK_DIRS_RELEASE )
set(zeromq_FRAMEWORKS_RELEASE )
set(zeromq_BUILD_DIRS_RELEASE )
set(zeromq_NO_SONAME_MODE_RELEASE FALSE)


# COMPOUND VARIABLES
set(zeromq_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${zeromq_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${zeromq_COMPILE_OPTIONS_C_RELEASE}>")
set(zeromq_LINKER_FLAGS_RELEASE
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${zeromq_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${zeromq_SHARED_LINK_FLAGS_RELEASE}>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${zeromq_EXE_LINK_FLAGS_RELEASE}>")


set(zeromq_COMPONENTS_RELEASE libzmq-static)
########### COMPONENT libzmq-static VARIABLES ############################################

set(zeromq_libzmq-static_INCLUDE_DIRS_RELEASE "${zeromq_PACKAGE_FOLDER_RELEASE}/include")
set(zeromq_libzmq-static_LIB_DIRS_RELEASE "${zeromq_PACKAGE_FOLDER_RELEASE}/lib")
set(zeromq_libzmq-static_BIN_DIRS_RELEASE )
set(zeromq_libzmq-static_LIBRARY_TYPE_RELEASE STATIC)
set(zeromq_libzmq-static_IS_HOST_WINDOWS_RELEASE 1)
set(zeromq_libzmq-static_RES_DIRS_RELEASE )
set(zeromq_libzmq-static_DEFINITIONS_RELEASE "-DZMQ_STATIC")
set(zeromq_libzmq-static_OBJECTS_RELEASE )
set(zeromq_libzmq-static_COMPILE_DEFINITIONS_RELEASE "ZMQ_STATIC")
set(zeromq_libzmq-static_COMPILE_OPTIONS_C_RELEASE "")
set(zeromq_libzmq-static_COMPILE_OPTIONS_CXX_RELEASE "")
set(zeromq_libzmq-static_LIBS_RELEASE libzmq-v143-mt-s-4_3_4)
set(zeromq_libzmq-static_SYSTEM_LIBS_RELEASE iphlpapi ws2_32)
set(zeromq_libzmq-static_FRAMEWORK_DIRS_RELEASE )
set(zeromq_libzmq-static_FRAMEWORKS_RELEASE )
set(zeromq_libzmq-static_DEPENDENCIES_RELEASE libsodium::libsodium)
set(zeromq_libzmq-static_SHARED_LINK_FLAGS_RELEASE )
set(zeromq_libzmq-static_EXE_LINK_FLAGS_RELEASE )
set(zeromq_libzmq-static_NO_SONAME_MODE_RELEASE FALSE)

# COMPOUND VARIABLES
set(zeromq_libzmq-static_LINKER_FLAGS_RELEASE
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,SHARED_LIBRARY>:${zeromq_libzmq-static_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,MODULE_LIBRARY>:${zeromq_libzmq-static_SHARED_LINK_FLAGS_RELEASE}>
        $<$<STREQUAL:$<TARGET_PROPERTY:TYPE>,EXECUTABLE>:${zeromq_libzmq-static_EXE_LINK_FLAGS_RELEASE}>
)
set(zeromq_libzmq-static_COMPILE_OPTIONS_RELEASE
    "$<$<COMPILE_LANGUAGE:CXX>:${zeromq_libzmq-static_COMPILE_OPTIONS_CXX_RELEASE}>"
    "$<$<COMPILE_LANGUAGE:C>:${zeromq_libzmq-static_COMPILE_OPTIONS_C_RELEASE}>")