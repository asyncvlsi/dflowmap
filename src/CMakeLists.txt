include_directories($ENV{ACT_HOME}/include)
add_subdirectory(backend)
add_subdirectory(common)
add_subdirectory(core)
file(
        GLOB SOURCES
        common/*.cc
        common/*.h
        core/*.cc
        core/*.h
        backend/chp/*.cc
        backend/chp/*.h
)
add_library(
        dflowmap-lib
        STATIC
        ${SOURCES}
)
add_executable(dflowmap
        main.cc
        )
target_link_libraries(dflowmap chpBackend dflowmap-core dflowmap-common ActLib ActCommon)
if (EXISTS $ENV{ACT_HOME}/lib/libexpropt.a)
    target_link_libraries(dflowmap ExprOpt)
endif ()
if (EXISTS $ENV{ACT_HOME}/lib/libexproptcommercial.a)
    target_link_libraries(dflowmap ExprOptCommercial)
endif ()
if (EXISTS $ENV{ACT_HOME}/lib/libnetlistgen.a)
    target_link_libraries(dflowmap netlistgen)
endif ()
