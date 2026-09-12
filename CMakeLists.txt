cmake_minimum_required(VERSION 3.16)
project(Gomoku CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS
    Core Gui Widgets Network Concurrent Multimedia)

add_executable(Gomoku WIN32
    src/main.cpp
    src/AppSettings.cpp
    src/BoardWidget.cpp
    src/GomokuCore.cpp
    src/MainWindow.cpp
    src/NetLink.cpp
    src/OnlineSession.cpp
    include/NetLink.h
    include/NetProtocol.h
    include/OnlineSession.h
    src/SkinDialog.cpp
    src/SoundManager.cpp
)

target_include_directories(Gomoku PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(Gomoku PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    Qt6::Concurrent
    Qt6::Multimedia
)

enable_testing()

add_executable(gomoku_core_tests
    tests/core_tests.cpp
    src/GomokuCore.cpp
)
target_include_directories(gomoku_core_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(gomoku_core_tests PRIVATE Qt6::Core Qt6::Network)
add_test(NAME gomoku_core_tests COMMAND gomoku_core_tests)

add_executable(online_session_tests
    tests/online_session_tests.cpp
    src/GomokuCore.cpp
    src/NetLink.cpp
    src/OnlineSession.cpp
    include/NetLink.h
    include/NetProtocol.h
    include/OnlineSession.h
)
target_include_directories(online_session_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(online_session_tests PRIVATE Qt6::Core Qt6::Network)
add_test(NAME online_session_tests COMMAND online_session_tests)
