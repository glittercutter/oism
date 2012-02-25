cmake_minimum_required (VERSION 2.6.2)
project (test)

set (SRC
    src/OISMHandler.cpp
    src/OISMHandlerUgly.cpp
    src/OISMSimpleSerializer.cpp
    src/test/main.cpp
    )

find_path (OIS_INCLUDE_DIRECTORY NAMES OIS.h PATHS /usr/include/OIS OIS)
find_library (OIS_LIBRARY OIS)

include_directories (
    ${CMAKE_SOURCE_DIR}/src
    ${OIS_INCLUDE_DIRECTORY}
    )

set (LIBS ${LIBS}
    ${OIS_LIBRARY}
    )

if (WIN32)
    set (LIBS ${LIBS}
        winmm
        ole32
        oleaut32
        dinput8
        dxguid
        )
endif()

set (CMAKE_CXX_FLAGS_RELEASE -O2)
set (CMAKE_CXX_FLAGS -O0)
add_definitions (-std=c++0x -Wall)

add_executable (test ${SRC})
target_link_libraries (test ${LIBS})
