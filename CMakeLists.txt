cmake_minimum_required(VERSION 3.9.0)
project(ErrorHandling)

set(CMAKE_CXX_FLAGS  "${CMAKE_CXX_FLAGS} -std=c++14 -O3")

set(SRC_FILES_COMMON
    src/GlobalTaskExecutor.cpp
    src/TaskSerializer.cpp
)

set(SRC_FILES_DININGPHILOSOPHERS
    ${SRC_FILES_COMMON}
    examples/DiningPhilosophers/DiningPhilosophers.cpp
)

add_executable(DiningPhilosophers ${SRC_FILES_DININGPHILOSOPHERS})
target_include_directories(DiningPhilosophers PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(DiningPhilosophers tbb)

