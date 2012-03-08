cmake_minimum_required (VERSION 2.6.2)
project (test)

set (SRC
    src/OISMHandler.cpp
    src/OISMHandlerUgly.cpp
    src/OISMSimpleSerializer.cpp
    src/test/TestUtils.cpp
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
elseif (UNIX)
    set (LIBS ${LIBS}
        X11
        )
endif ()

add_definitions (-std=c++0x -Wall)

set (OISM_ENABLE_LOG "FALSE" CACHE BOOL "OISM use log")
set (OISM_ENABLE_LOG_FILE "FALSE" CACHE BOOL "OISM use log file")

if (OISM_ENABLE_LOG)
    add_definitions(-DOISM_ENABLE_LOG)
endif ()
if (OISM_ENABLE_LOG_FILE)
    add_definitions(-DOISM_ENABLE_LOG_FILE)
endif ()

add_executable (test ${SRC} src/test/Test.cpp)
target_link_libraries (test ${LIBS})

add_executable (test-speed ${SRC} src/test/TestSpeed.cpp)
target_link_libraries (test-speed ${LIBS})
