SET(SRC main.cpp)
source_group(fuckd ${SRC})
add_executable(fuckd ${SRC})
target_link_libraries(fuckd components)
