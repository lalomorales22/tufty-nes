add_library(st7789 INTERFACE)

target_sources(st7789 INTERFACE ${CMAKE_CURRENT_LIST_DIR}/st7789.c)

target_link_libraries(st7789 INTERFACE hardware_pio hardware_clocks hardware_dma)

target_include_directories(st7789 INTERFACE
        ${CMAKE_CURRENT_LIST_DIR}
)

if(TFT_PARALLEL)
    pico_generate_pio_header(st7789
            ${CMAKE_CURRENT_LIST_DIR}/st7789_parallel.pio
    )
    target_compile_definitions(st7789 INTERFACE TFT_PARALLEL)
else()
    pico_generate_pio_header(st7789
            ${CMAKE_CURRENT_LIST_DIR}/st7789.pio
    )
endif()
