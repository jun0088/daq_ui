# Load the debug and release variables
file(GLOB DATA_FILES "${CMAKE_CURRENT_LIST_DIR}/libsodium-*-data.cmake")

foreach(f ${DATA_FILES})
    include(${f})
endforeach()

# Create the targets for all the components
foreach(_COMPONENT ${libsodium_COMPONENT_NAMES} )
    if(NOT TARGET ${_COMPONENT})
        add_library(${_COMPONENT} INTERFACE IMPORTED)
        message(${libsodium_MESSAGE_MODE} "Conan: Component target declared '${_COMPONENT}'")
    endif()
endforeach()

if(NOT TARGET libsodium::libsodium)
    add_library(libsodium::libsodium INTERFACE IMPORTED)
    message(${libsodium_MESSAGE_MODE} "Conan: Target declared 'libsodium::libsodium'")
endif()
# Load the debug and release library finders
file(GLOB CONFIG_FILES "${CMAKE_CURRENT_LIST_DIR}/libsodium-Target-*.cmake")

foreach(f ${CONFIG_FILES})
    include(${f})
endforeach()