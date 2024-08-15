# Avoid multiple calls to find_package to append duplicated properties to the targets
include_guard()########### VARIABLES #######################################################################
#############################################################################################
set(libsodium_FRAMEWORKS_FOUND_RELEASE "") # Will be filled later
conan_find_apple_frameworks(libsodium_FRAMEWORKS_FOUND_RELEASE "${libsodium_FRAMEWORKS_RELEASE}" "${libsodium_FRAMEWORK_DIRS_RELEASE}")

set(libsodium_LIBRARIES_TARGETS "") # Will be filled later


######## Create an interface target to contain all the dependencies (frameworks, system and conan deps)
if(NOT TARGET libsodium_DEPS_TARGET)
    add_library(libsodium_DEPS_TARGET INTERFACE IMPORTED)
endif()

set_property(TARGET libsodium_DEPS_TARGET
             APPEND PROPERTY INTERFACE_LINK_LIBRARIES
             $<$<CONFIG:Release>:${libsodium_FRAMEWORKS_FOUND_RELEASE}>
             $<$<CONFIG:Release>:${libsodium_SYSTEM_LIBS_RELEASE}>
             $<$<CONFIG:Release>:>)

####### Find the libraries declared in cpp_info.libs, create an IMPORTED target for each one and link the
####### libsodium_DEPS_TARGET to all of them
conan_package_library_targets("${libsodium_LIBS_RELEASE}"    # libraries
                              "${libsodium_LIB_DIRS_RELEASE}" # package_libdir
                              "${libsodium_BIN_DIRS_RELEASE}" # package_bindir
                              "${libsodium_LIBRARY_TYPE_RELEASE}"
                              "${libsodium_IS_HOST_WINDOWS_RELEASE}"
                              libsodium_DEPS_TARGET
                              libsodium_LIBRARIES_TARGETS  # out_libraries_targets
                              "_RELEASE"
                              "libsodium"    # package_name
                              "${libsodium_NO_SONAME_MODE_RELEASE}")  # soname

# FIXME: What is the result of this for multi-config? All configs adding themselves to path?
set(CMAKE_MODULE_PATH ${libsodium_BUILD_DIRS_RELEASE} ${CMAKE_MODULE_PATH})

########## GLOBAL TARGET PROPERTIES Release ########################################
    set_property(TARGET libsodium::libsodium
                 APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                 $<$<CONFIG:Release>:${libsodium_OBJECTS_RELEASE}>
                 $<$<CONFIG:Release>:${libsodium_LIBRARIES_TARGETS}>
                 )

    if("${libsodium_LIBS_RELEASE}" STREQUAL "")
        # If the package is not declaring any "cpp_info.libs" the package deps, system libs,
        # frameworks etc are not linked to the imported targets and we need to do it to the
        # global target
        set_property(TARGET libsodium::libsodium
                     APPEND PROPERTY INTERFACE_LINK_LIBRARIES
                     libsodium_DEPS_TARGET)
    endif()

    set_property(TARGET libsodium::libsodium
                 APPEND PROPERTY INTERFACE_LINK_OPTIONS
                 $<$<CONFIG:Release>:${libsodium_LINKER_FLAGS_RELEASE}>)
    set_property(TARGET libsodium::libsodium
                 APPEND PROPERTY INTERFACE_INCLUDE_DIRECTORIES
                 $<$<CONFIG:Release>:${libsodium_INCLUDE_DIRS_RELEASE}>)
    # Necessary to find LINK shared libraries in Linux
    set_property(TARGET libsodium::libsodium
                 APPEND PROPERTY INTERFACE_LINK_DIRECTORIES
                 $<$<CONFIG:Release>:${libsodium_LIB_DIRS_RELEASE}>)
    set_property(TARGET libsodium::libsodium
                 APPEND PROPERTY INTERFACE_COMPILE_DEFINITIONS
                 $<$<CONFIG:Release>:${libsodium_COMPILE_DEFINITIONS_RELEASE}>)
    set_property(TARGET libsodium::libsodium
                 APPEND PROPERTY INTERFACE_COMPILE_OPTIONS
                 $<$<CONFIG:Release>:${libsodium_COMPILE_OPTIONS_RELEASE}>)

########## For the modules (FindXXX)
set(libsodium_LIBRARIES_RELEASE libsodium::libsodium)
