SET(SRC main.cpp)
source_group(fuckc ${SRC})
add_executable(fuckc ${SRC})
target_link_libraries(fuckc components)
