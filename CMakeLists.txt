cmake_minimum_required(VERSION 3.26)
project(
        ZeroLogic
        LANGUAGES CXX
)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXE_LINKER_FLAGS "-static")
add_compile_options("-mbmi2")
add_compile_options("-mbmi")

add_executable(ZeroLogic
        src/main.cpp
        src/uci.cpp
        src/tables.h
        src/eval.h
        src/gamestate.h
        src/uci.h
        src/variables.h
        src/movegen.h
        src/misc.h
        src/new_search.h
        src/new_search.cpp)