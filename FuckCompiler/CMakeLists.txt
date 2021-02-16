SET(SRC main.cpp fuckoptimizer.cpp fuckoptimizer.hpp fucktranslator.hpp cistring.hpp)
source_group(fuckc ${SRC})
add_executable(fuckc ${SRC})
target_link_libraries(fuckc components)
