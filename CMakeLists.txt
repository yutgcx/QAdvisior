cmake_minimum_required(VERSION 3.14)
project(QAdvisor VERSION 1.0.0 LANGUAGES C CXX)

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Find LLVM (REQUIRED)
set(LLVM_DIR "/usr/lib/cmake/llvm")
find_package(LLVM REQUIRED CONFIG)

# Include LLVM headers and libraries
include_directories(${LLVM_INCLUDE_DIRS})
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

# Find Flex and Bison for Phase 1
find_package(FLEX REQUIRED)
find_package(BISON REQUIRED)

# Phase 1: Flex/Bison generation
BISON_TARGET(Phase1Parser 
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer/qasm.ypp 
    ${CMAKE_CURRENT_BINARY_DIR}/lexer/qasm.tab.cpp
)
FLEX_TARGET(Phase1Lexer 
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer/qasm.l 
    ${CMAKE_CURRENT_BINARY_DIR}/lexer/lex.yy.cc
)
ADD_FLEX_BISON_DEPENDENCY(Phase1Lexer Phase1Parser)

# Source files
set(PHASE1_SOURCES
    lexer/ast.cpp
    lexer/semantic.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lexer/qasm.tab.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lexer/lex.yy.cc
    lexer/main.cpp
)

set(PHASE2_SOURCES
    dependecy/dependency.cpp
    dependecy/scheduler.cpp
    dependecy/analysis.cpp
)

set(PHASE3_SOURCES
    qir/qir_generator.cpp
    qir/qubit_manager.cpp
    qir/type_manager.cpp
    qir/metadata_builder.cpp
    qir/main.cpp
)

# Phase 1 Executable
add_executable(qadvisor_phase1
    ${PHASE1_SOURCES}
)

# Phase 2 Executable
add_executable(qadvisor_phase2
    ${PHASE2_SOURCES}
    dependecy/main.cpp
    lexer/ast.cpp
    lexer/semantic.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lexer/qasm.tab.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lexer/lex.yy.cc
)

target_include_directories(qadvisor_phase2 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/dependecy
    ${CMAKE_CURRENT_BINARY_DIR}/lexer
)

# Phase 3 Executable (which might depend on all phases)
add_executable(qadvisor_phase3
    ${PHASE3_SOURCES}
    ${PHASE2_SOURCES}
    lexer/ast.cpp
    lexer/semantic.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lexer/qasm.tab.cpp
    ${CMAKE_CURRENT_BINARY_DIR}/lexer/lex.yy.cc
)

target_link_libraries(qadvisor_phase1
)

target_link_libraries(qadvisor_phase3
    LLVM-22
)

# Set include directories
target_include_directories(qadvisor_phase1 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer
    ${CMAKE_CURRENT_BINARY_DIR}/lexer
)

target_include_directories(qadvisor_phase3 PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/lexer
    ${CMAKE_CURRENT_SOURCE_DIR}/dependecy
    ${CMAKE_CURRENT_SOURCE_DIR}/qir
    ${CMAKE_CURRENT_BINARY_DIR}/lexer
)