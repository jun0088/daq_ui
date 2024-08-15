########## MACROS ###########################################################################
#############################################################################################

# Requires CMake > 3.15
if(${CMAKE_VERSION} VERSION_LESS "3.15")
    message(FATAL_ERROR "The 'CMakeDeps' generator only works with CMake >= 3.15")
endif()

if(libsodium_FIND_QUIETLY)
    set(libsodium_MESSAGE_MODE VERBOSE)
else()
    set(libsodium_MESSAGE_MODE STATUS)
endif()

include(${CMAKE_CURRENT_LIST_DIR}/cmakedeps_macros.cmake)
include(${CMAKE_CURRENT_LIST_DIR}/libsodiumTargets.cmake)
include(CMakeFindDependencyMacro)

check_build_type_defined()

foreach(_DEPENDENCY ${libsodium_FIND_DEPENDENCY_NAMES} )
    # Check that we have not already called a find_package with the transitive dependency
    if(NOT ${_DEPENDENCY}_FOUND)
        find_dependency(${_DEPENDENCY} REQUIRED ${${_DEPENDENCY}_FIND_MODE})
    endif()
endforeach()

set(libsodium_VERSION_STRING "1.0.19")
set(libsodium_INCLUDE_DIRS ${libsodium_INCLUDE_DIRS_RELEASE} )
set(libsodium_INCLUDE_DIR ${libsodium_INCLUDE_DIRS_RELEASE} )
set(libsodium_LIBRARIES ${libsodium_LIBRARIES_RELEASE} )
set(libsodium_DEFINITIONS ${libsodium_DEFINITIONS_RELEASE} )


# Only the last installed configuration BUILD_MODULES are included to avoid the collision
foreach(_BUILD_MODULE ${libsodium_BUILD_MODULES_PATHS_RELEASE} )
    message(${libsodium_MESSAGE_MODE} "Conan: Including build module from '${_BUILD_MODULE}'")
    include(${_BUILD_MODULE})
endforeach()


