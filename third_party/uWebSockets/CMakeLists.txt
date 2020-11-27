
# Use the external project module to build the library
include(ExternalProject)
ExternalProject_Add(uWebSockets_EXT
    SOURCE_DIR "${CMAKE_CURRENT_LIST_DIR}/src"

    CONFIGURE_COMMAND ""

    BUILD_COMMAND "make"
    BUILD_IN_SOURCE ON

    INSTALL_COMMAND ""
    )

# Create a cmake target for using the external project
add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/src
    ${CMAKE_CURRENT_LIST_DIR}/src/uSockets/src)
target_link_libraries(uWebSockets INTERFACE z ssl
    ${CMAKE_CURRENT_LIST_DIR}/src/uSockets/uSockets.a)
