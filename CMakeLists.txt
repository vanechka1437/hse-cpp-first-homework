cmake_minimum_required(VERSION 3.13)

project(program)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Для основного проекта
add_executable(program main memdb.cpp tokenization.cpp exceptions.cpp memdb.h exceptions.h)

# Для тестов добавляем флаг отладки
add_executable(tests tests.cpp memdb.h memdb.cpp exceptions.h exceptions.cpp tokenization.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
