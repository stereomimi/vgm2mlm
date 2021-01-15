add_library(vgm2mlm_lib vgmcom.c vgm2mlm.c utils.c driver.m1.c driver.p1.c driver.s1.c)

target_link_libraries(vgm2mlm_lib m)