cmake_minimum_required(VERSION 3.26)
project(
        ZeroLogic
        LANGUAGES CXX
)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXE_LINKER_FLAGS "-static")

add_executable(ZeroLogic
        src/main.cpp
        src/eval.cpp
        src/gamestate.cpp
        src/misc.cpp
        src/uci.cpp
        src/tables.h
        src/eval.h
        src/gamestate.h
        src/uci.h
        src/variables.h
        src/tables.cpp
        src/movegen.h
        src/movegen.cpp
        src/misc.h
        src/misc.h
        src/new_search.h
        src/new_search.cpp)