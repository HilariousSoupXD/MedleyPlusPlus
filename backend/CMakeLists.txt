cmake_minimum_required(VERSION 3.15)
project(medley_backend)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Include directories
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/libs/Crow/include)

# Source files
set(SOURCES
    src/main.cpp
    src/DataLoader.cpp
    src/UserProfiler.cpp
    src/InteractionStore.cpp
    src/ContentBased.cpp
    src/UserBased.cpp
    src/Hybrid.cpp
)

# Create executable
add_executable(app ${SOURCES})

# Link libraries (pthread for threading)
target_link_libraries(app pthread)

# Set output directory
set_target_properties(app PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
