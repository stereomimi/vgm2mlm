add_library(vgm2mlm_lib vgmcom.c vgm2mlm.c utils.c driver.m1.c driver.p1.c driver.s1.c)

if(MSVC)
  target_compile_options(vgm2mlm_lib PRIVATE /W4)
else()
  target_compile_options(vgm2mlm_lib PRIVATE -Wall)
endif()

target_link_libraries(vgm2mlm_lib m)