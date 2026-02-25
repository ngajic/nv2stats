cmake_minimum_required(VERSION 3.17)
project(nv2stats C)

set(CMAKE_C_STANDARD 17)
set(CMAKE_BUILD_TYPE Debug)

set(BUILD_SHARED_LIBS ON)

find_package(SQLite3 REQUIRED)
find_package(ZLIB REQUIRED)

add_executable(nv2stats
    main.c
    base64.h base64.c
    cypher.h cypher.c
    functions.h functions.c
    opt.h opt.c
    sqlcmds.h
    util.h util.c
    usage.h
)

add_executable(nv2ninjaremoval
    main_ninjaremoval.c
)

target_link_libraries(nv2stats PRIVATE
    SQLite::SQLite3
    ZLIB::ZLIB
)

if(WIN32)
    target_compile_definitions(nv2stats PRIVATE _CRT_SECURE_NO_WARNINGS=1)

    add_custom_command(TARGET nv2stats POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            $<TARGET_FILE:SQLite::SQLite3>
            $<TARGET_FILE:ZLIB::ZLIB>
            $<TARGET_FILE_DIR:nv2stats>
    )
endif()
