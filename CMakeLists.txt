cmake_minimum_required (VERSION 2.6.2)
project (test)

# src
set (SRC
    src/OISMHandler.cpp
    src/OISMHandlerUgly.cpp
    src/OISMSimpleSerializer.cpp
    src/test/main.cpp
)

# flag
set (CMAKE_CXX_FLAGS_RELEASE -O2)
set (CMAKE_CXX_FLAGS -O0)
add_definitions (-std=c++0x -Wall)


if (WIN32)
    set (SYSTEM_LIBS
        "${SYSTEM_LIBS}
        winmm
        ole32
        oleaut32
        dinput8
        dxguid"
    )

    if (UNIX)
        # both Cygwin and Msys need -DNOMINMAX ???
        add_definitions (-DNOMINMAX)
    endif (UNIX)

    # boost thread
    #add_definitions (-D_WIN32_WINNT=0x0501 -DBOOST_THREAD_USE_LIB)
    #set (CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} pthreadGCE2.dll")
endif (WIN32)

add_executable (test ${SRC})
target_link_libraries (test
    OIS
)
