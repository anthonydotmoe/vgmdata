project(vgmdata)

add_subdirectory(TinyVGM)

add_executable(vgmdata vgmdata.c)

target_link_libraries(vgmdata LINK_PUBLIC TinyVGM)
