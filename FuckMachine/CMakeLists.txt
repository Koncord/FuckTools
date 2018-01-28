SET(SRC main.cpp)
source_group(fuckmachine ${SRC})
add_executable(fuckmachine ${SRC})
target_link_libraries(fuckmachine components)
