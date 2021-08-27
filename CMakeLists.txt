cmake_minimum_required(VERSION 3.9)
project(FuckTools)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_INTERPROCEDURAL_OPTIMIZATION True)

if (UNIX)
    add_compile_options(-fno-rtti -fno-exceptions)
    #add_compile_options(-Ofast)
    #set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -m32")
endif ()


include_directories(SYSTEM .)

add_subdirectory(FuckComponents)
add_subdirectory(FuckMachine)
add_subdirectory(FuckCompiler)
add_subdirectory(FuckMachineC)
add_subdirectory(BFCC)
add_subdirectory(BFCD)
#add_subdirectory(PreFuck)

add_subdirectory(tests)
