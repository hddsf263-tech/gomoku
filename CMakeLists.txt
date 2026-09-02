cmake_minimum_required(VERSION 3.16)
project(Gomoku VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Widgets Concurrent QUIET)
if(NOT Qt6_FOUND)
    find_package(Qt5 5.15 COMPONENTS Core Gui Widgets Concurrent REQUIRED)
endif()

set(SOURCES
    src/main.cpp
    src/MainWindow.cpp
    src/BoardWidget.cpp
    src/Game.cpp
    src/GameModeDialog.cpp
    src/Board.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_MainWindow.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_BoardWidget.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/moc_GameModeDialog.cpp
)

set(HEADERS
    src/MainWindow.h
    src/BoardWidget.h
    src/GameModeDialog.h
    include/Game.h
    include/Board.h
    include/ChessPiece.h
)

# 手动 moc：当前环境 g++ -E 不可用，AutoMoc 的 moc_predefs 生成会失败
if(Qt6_FOUND)
    set(MOC_EXECUTABLE Qt6::moc)
    set(QT_MOC_TARGET Qt6::Core)
else()
    set(MOC_EXECUTABLE Qt5::moc)
    set(QT_MOC_TARGET Qt5::Core)
endif()

get_target_property(QT_MOC_INCLUDES ${QT_MOC_TARGET} INTERFACE_INCLUDE_DIRECTORIES)
set(MOC_INCLUDE_ARGS)
foreach(dir ${QT_MOC_INCLUDES})
    list(APPEND MOC_INCLUDE_ARGS -I ${dir})
endforeach()
list(APPEND MOC_INCLUDE_ARGS -I ${CMAKE_CURRENT_SOURCE_DIR}/include -I ${CMAKE_CURRENT_SOURCE_DIR}/src)

add_custom_command(
    OUTPUT
        ${CMAKE_CURRENT_BINARY_DIR}/moc_MainWindow.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/moc_BoardWidget.cpp
        ${CMAKE_CURRENT_BINARY_DIR}/moc_GameModeDialog.cpp
    COMMAND ${MOC_EXECUTABLE} ${MOC_INCLUDE_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_MainWindow.cpp
    COMMAND ${MOC_EXECUTABLE} ${MOC_INCLUDE_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/src/BoardWidget.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_BoardWidget.cpp
    COMMAND ${MOC_EXECUTABLE} ${MOC_INCLUDE_ARGS} ${CMAKE_CURRENT_SOURCE_DIR}/src/GameModeDialog.h -o ${CMAKE_CURRENT_BINARY_DIR}/moc_GameModeDialog.cpp
    DEPENDS
        ${CMAKE_CURRENT_SOURCE_DIR}/src/MainWindow.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/BoardWidget.h
        ${CMAKE_CURRENT_SOURCE_DIR}/src/GameModeDialog.h
    VERBATIM
)
add_executable(Gomoku ${SOURCES} ${HEADERS})
set_target_properties(Gomoku PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

target_include_directories(Gomoku PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

if(Qt6_FOUND)
    target_link_libraries(Gomoku PRIVATE Qt6::Core Qt6::Gui Qt6::Widgets Qt6::Concurrent ai_core)
else()
    target_link_libraries(Gomoku PRIVATE Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Concurrent ai_core)
endif()

# AI Core（Phase 2A：独立静态库，不接入 UI / Gomoku）
set(AI_CORE_SOURCES
    src/ai/MoveGenerator.cpp
    src/ai/Evaluation.cpp
    src/ai/SearchEngine.cpp
    src/ai/ZobristHash.cpp
    src/ai/TranspositionTable.cpp
    src/ai/AIPlayer.cpp
    src/ai/HumanPlayer.cpp
    src/GameController.cpp
)

set(AI_CORE_HEADERS
    include/ai/AIConfig.h
    include/ai/GameStateSnapshot.h
    include/ai/MoveGenerator.h
    include/ai/Evaluation.h
    include/ai/SearchEngine.h
    include/ai/SearchResult.h
    include/ai/TranspositionTable.h
    include/ai/ZobristHash.h
    include/ai/IPlayer.h
    include/ai/AIPlayer.h
    include/ai/HumanPlayer.h
    include/GameController.h
)

add_library(ai_core STATIC ${AI_CORE_SOURCES} ${AI_CORE_HEADERS})
target_include_directories(ai_core PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_compile_features(ai_core PUBLIC cxx_std_17)
set_target_properties(ai_core PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)

enable_testing()
add_executable(ai_core_tests tests/ai_core_tests.cpp src/Board.cpp src/Game.cpp)
set_target_properties(ai_core_tests PROPERTIES AUTOMOC OFF AUTOUIC OFF AUTORCC OFF)
target_include_directories(ai_core_tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/include
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)
target_link_libraries(ai_core_tests PRIVATE ai_core)
add_test(NAME ai_core_tests COMMAND ai_core_tests)