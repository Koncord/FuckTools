if(MSVC)
    set (CXX_OUT_PARAM "/link /out:")
else()
    set (CXX_OUT_PARAM -funroll-all-loops -O2 -o)
endif()

add_custom_target(test_fuckc
        COMMAND fuckc hello.bf hello.bfc motherfucker
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS "fuckc"
        )

add_custom_target(test_fuckmachine
        COMMAND fuckmachine hello.bfc
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS test_fuckc fuckmachine
        )

add_custom_target(test_fuckcc
        COMMAND fuckcc hello.bfc
        COMMAND ${CMAKE_C_COMPILER} hello.c ${CXX_OUT_PARAM}hello$<$<BOOL:${WIN32}>:.exe>
        COMMAND "${CMAKE_SOURCE_DIR}/tests/hello"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS test_fuckc fuckcc
        )






add_custom_target(test_mandelbrot_fuckc
        COMMAND fuckc mandelbrot.bf mandelbrot.bfc motherfucker
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS "fuckc"
        )

add_custom_target(test_mandelbrot_fuckmachine
        COMMAND fuckmachine mandelbrot.bfc
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS test_mandelbrot_fuckc fuckmachine
        )

add_custom_target(test_mandelbrot_fuckcc
        COMMAND fuckcc mandelbrot.bfc
        COMMAND ${CMAKE_C_COMPILER} mandelbrot.c ${CXX_OUT_PARAM}mandelbrot$<$<BOOL:${WIN32}>:.exe>
        COMMAND "${CMAKE_SOURCE_DIR}/tests/mandelbrot"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS test_mandelbrot_fuckc fuckcc
        )



add_custom_target(test_factor_fuckc
        COMMAND fuckc factor.bf factor.bfc motherfucker
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS "fuckc"
        )

add_custom_target(test_factor_fuckmachine
        COMMAND fuckmachine factor.bfc
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS test_factor_fuckc fuckmachine
        )

add_custom_target(test_factor_fuckcc
        COMMAND fuckcc factor.bfc
        COMMAND ${CMAKE_C_COMPILER} factor.c ${CXX_OUT_PARAM}factor$<$<BOOL:${WIN32}>:.exe>
        COMMAND "${CMAKE_SOURCE_DIR}/tests/factor"
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tests"
        DEPENDS test_factor_fuckc fuckcc
        )
