SET(SRC main.cpp)
source_group(fuckcc ${SRC})
add_executable(fuckcc ${SRC})
target_link_libraries(fuckcc components)
