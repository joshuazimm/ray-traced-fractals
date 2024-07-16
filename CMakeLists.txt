cmake_minimum_required(VERSION 3.28)
project(Fractal)

# Specify the C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Set the output directories for different build types
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
set(CMAKE_SUPPRESS_REGENERATION true)

# Avoid creating unnecessary intermediate directories
foreach(OUTPUTCONFIG ${CMAKE_CONFIGURATION_TYPES})
    string(TOUPPER ${OUTPUTCONFIG} OUTPUTCONFIG)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/bin)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
    set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY_${OUTPUTCONFIG} ${CMAKE_BINARY_DIR}/lib)
endforeach()

# Find all .c and .h files in src directory
file(GLOB_RECURSE SRC_FILES "${CMAKE_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE HDR_FILES "${CMAKE_SOURCE_DIR}/src/*.h")
file(GLOB_RECURSE GLSL_FILES "${CMAKE_SOURCE_DIR}/resources/*.glsl")

# Add executable
add_executable(${PROJECT_NAME} ${SRC_FILES} ${HDR_FILES} ${GLSL_FILES})

# Set properties for GLSL files
foreach(GLSL_FILE ${GLSL_FILES})
    source_group("Shaders\\" FILES ${GLSL_FILE})
endforeach()

# Copy GLSL files to the build directory
foreach(GLSL_FILE ${GLSL_FILES})
    get_filename_component(GLSL_FILE_NAME ${GLSL_FILE} NAME)
    configure_file(${GLSL_FILE} ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/${GLSL_FILE_NAME} COPYONLY)
endforeach()

add_library(lodepng ${CMAKE_SOURCE_DIR}/dep/lodepng/lodepng.cpp)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/dep/glfw/include
    ${CMAKE_SOURCE_DIR}/dep/glew/include
    ${CMAKE_SOURCE_DIR}/dep/glm/include
    ${CMAKE_SOURCE_DIR}/dep/lodepng/
)

# Dependencies
add_subdirectory(dep/glfw)
add_subdirectory(dep/glew)
add_subdirectory(dep/glm)

set_property(TARGET glm PROPERTY FOLDER "Libs")
set_property(TARGET libglew_shared PROPERTY FOLDER "Libs")
set_property(TARGET libglew_static PROPERTY FOLDER "Libs")
set_property(TARGET glfw PROPERTY FOLDER "Libs")
set_property(TARGET uninstall PROPERTY FOLDER "Libs")
set_property(TARGET update_mappings PROPERTY FOLDER "Libs")

# Link libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    glfw
    libglew_static
    glm
    lodepng
)