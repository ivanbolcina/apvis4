cmake_minimum_required(VERSION 3.5)
#other cmake files
list(APPEND CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake )
#remember name of theproject
set(PROJECT_NAME apvis4)
#set name of the project
project(${PROJECT_NAME})


#get packages
find_package(PkgConfig REQUIRED)
find_package(Threads REQUIRED)
find_package(OpenGL REQUIRED)
pkg_check_modules(GTKMM gtkmm-3.0)

#include conan
include(conan/conanbuildinfo.cmake)
conan_basic_setup()

#gtkmm
link_directories(${GTKMM_LIBRARY_DIRS})
include_directories(${GTKMM_INCLUDE_DIRS})
include_directories( ${OPENGL_INCLUDE_DIRS} )
include_directories(include)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++11")

#include(findepoxy)

#resources, why GRESOURCEDEPENDS is not passed right
include(GResource)
gresource(${CMAKE_CURRENT_SOURCE_DIR} resources.gresource.xml
        ${CMAKE_BINARY_DIR} RESOURCE_FILE)

include(sphinx)

#add sources
file(GLOB SOURCES "${PROJECT_SOURCE_DIR}/src/*.cpp" )

#executable
add_executable(${PROJECT_NAME} ${SOURCES} ${RESOURCE_FILE})

#libs
target_link_libraries(${PROJECT_NAME} ${OPENGL_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${GTKMM_LIBRARIES})
target_link_libraries(${PROJECT_NAME} ${CMAKE_THREAD_LIBS_INIT})
target_link_libraries(${PROJECT_NAME} pulse-simple pulse)
target_link_libraries(${PROJECT_NAME} ${CONAN_LIBS})

#target_link_libraries(${PROJECT_NAME} ${LIBEPOXY_LIBRARIES})

# Install the executable
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

