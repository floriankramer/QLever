
add_library(uSockets STATIC
    src/uSockets/src/bsd.c
    src/uSockets/src/context.c
    src/uSockets/src/loop.c
    src/uSockets/src/socket.c
#    src/uSockets/src/crypto/openssl.c
#    src/uSockets/src/crypto/sni_tree.cpp
#    src/uSockets/src/crypto/wolfssl.c
    src/uSockets/src/eventing/epoll_kqueue.c
    src/uSockets/src/eventing/gcd.c
    src/uSockets/src/eventing/libuv.c
    )

target_compile_definitions(uSockets PUBLIC LIBUS_NO_SSL)

target_include_directories(uSockets PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src/uSockets/src/)
set_target_properties(uSockets PROPERTIES CMAKE_CXX_STANDARD 17)
target_link_libraries(uSockets z)

add_library(uWebSockets INTERFACE)
target_include_directories(uWebSockets INTERFACE
    ${CMAKE_CURRENT_LIST_DIR}/src/src)
target_link_libraries(uWebSockets INTERFACE uSockets)
