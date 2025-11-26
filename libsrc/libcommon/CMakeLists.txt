add_library(common STATIC
    itoa.c
    itoa_pad.c
    u8tohex.c

    date.c

    max.c
    min.c

    mem_cpy.c
    mem_set.c

    put_c.c
    put_s.c
    put_sn.c
    put_u8.c

    str_cat.c
    str_cmp.c
    str_cpy.c
    str_ends_with.c
    str_len.c
    str_pos.c
)

target_include_directories(common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_options(common PRIVATE --nostdlib)