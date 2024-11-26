cmake_minimum_required(VERSION 3.13)

project(program)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Для основного проекта
add_executable(program memdb.cpp)

# Для тестов добавляем флаг отладки
add_executable(tests tests.cpp)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
