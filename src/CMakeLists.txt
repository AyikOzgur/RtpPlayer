cmake_minimum_required(VERSION 3.13)



################################################################################
## EXECUTABLE-PROJECT
## name and version
################################################################################
project(RtpPlayer LANGUAGES CXX)



################################################################################
## SETTINGS
## basic project settings before use
################################################################################
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# creating output directory architecture in accordance with GNU guidelines
set(BINARY_DIR "${CMAKE_BINARY_DIR}")
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${BINARY_DIR}/lib")



################################################################################
## TARGET
## create target and add include path
################################################################################
# create glob files for *.h, *.cpp
file (GLOB H_FILES   ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file (GLOB CPP_FILES ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
# concatenate the results (glob files) to variable
set  (SOURCES ${CPP_FILES} ${H_FILES})
if (NOT TARGET ${PROJECT_NAME})
    add_executable(${PROJECT_NAME} ${SOURCES})
endif()



################################################################################
## LINK LIBRARIES
## linking all dependencies
################################################################################
find_package(Qt6 REQUIRED COMPONENTS Widgets)
target_link_libraries(${PROJECT_NAME} Qt6::Widgets)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(OpenCV REQUIRED)
target_link_libraries(${PROJECT_NAME} ${OpenCV_LIBS})

target_link_libraries(${PROJECT_NAME} VideoCodec)
target_link_libraries(${PROJECT_NAME} RtpReceiver)


################################################################################
## ADDITIONAL SETTINGS
## additional options and project settings
################################################################################