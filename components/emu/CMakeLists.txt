#Note: this is for the ESP32 build.


idf_component_register(SRCS emu.c cpu.c cpu_addr_space.c load_exe.c scheduler.c trace.c music.c
        ibxm/ibxm.c pf_vars.c
        REQUIRES log
        INCLUDE_DIRS "." "ibxm" )

target_compile_options(${COMPONENT_LIB} PRIVATE -DPROJECT_NAME="${CMAKE_PROJECT_NAME}")
