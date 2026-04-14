cmake_minimum_required(VERSION 3.17)
project(nv2stats C)

set(CMAKE_C_STANDARD 17)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

set(BUILD_SHARED_LIBS ON)

find_package(SQLite3 REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(nv2stats
    main.c
    base64.h base64.c
    cypher.h cypher.c
    functions.h functions.c
    opt.h opt.c
    util.h util.c
    usage.h
)

add_executable(nv2ninjaremoval
    main_ninjaremoval.c
)

if(MSVC)
    target_compile_options(nv2ninjaremoval PRIVATE
        /O2
        /Oi
        /GL
        /Gy
    )
    target_link_options(nv2ninjaremoval PRIVATE
        /LTCG
    )
else()
    target_compile_options(nv2ninjaremoval PRIVATE
        -Wall -Wextra -Wpedantic
        -Wshadow -Wdouble-promotion -Wconversion -Wsign-conversion
        -Wnull-dereference -Wstrict-prototypes -Wmissing-prototypes
        -Wno-unused-function -Wno-missing-braces

        -fanalyzer

        -fstack-protector-strong

        -O3
        -march=native
        -flto
        -funroll-loops
        -fomit-frame-pointer
    )
    target_link_options(nv2ninjaremoval PRIVATE
        -flto
    )

    target_compile_options(nv2stats PRIVATE
        -Wall -Wextra -Wpedantic
        -Wshadow -Wdouble-promotion -Wconversion -Wsign-conversion
        -Wnull-dereference -Wstrict-prototypes -Wmissing-prototypes
        -Wno-unused-function -Wno-missing-braces

        -fanalyzer

        -fstack-protector-strong

        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    )

    target_link_options(nv2stats PRIVATE
        $<$<CONFIG:Debug>:-fsanitize=address,undefined>
    )


endif()

target_link_libraries(nv2stats PRIVATE
    SQLite::SQLite3
    ZLIB::ZLIB
)

if(UNIX)
    target_link_libraries(nv2stats PRIVATE m)
endif()

if(WIN32)
    target_compile_definitions(nv2stats PRIVATE _CRT_SECURE_NO_WARNINGS=1)

    add_custom_command(TARGET nv2stats POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SQLite::SQLite3>
            $<TARGET_FILE:ZLIB::ZLIB>
            $<TARGET_FILE_DIR:nv2stats>
    )
endif()
