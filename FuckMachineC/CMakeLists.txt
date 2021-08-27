add_executable(fuckmachinec main.c)
set_property(TARGET fuckmachinec PROPERTY MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
