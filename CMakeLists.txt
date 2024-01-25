cmake_minimum_required(VERSION 3.26)
project(
        ZeroLogic
        LANGUAGES CXX
)

enable_language(CXX)
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_EXE_LINKER_FLAGS "-static")
set(CMAKE_CXX_FLAGS_RELEASE "-Ofast -DNDEBUG")
set(CMAKE_CXX_FLAGS "-DUSE_INTRIN")
add_compile_options(-mbmi2 -mbmi)

add_executable(ZeroLogic
        src/main.cpp
        src/tables.h
        src/eval.h
        src/uci.h
        src/stuff.h
        src/movegenerator.h
        src/misc.h
        src/perft_callback.h
        src/static.h
        src/search_callback.h
        src/search.h
        src/position.h
        src/hash.h
        src/tests.h
        src/limit.h
        src/game.h)