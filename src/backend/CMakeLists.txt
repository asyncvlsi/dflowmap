add_subdirectory(chp)
if (EXISTS $ENV{ACT_HOME}/lib/libnetlistgen.a)
    add_subdirectory(netlist)
endif ()