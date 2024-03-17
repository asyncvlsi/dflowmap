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
        dflowchp
        STATIC
        ${SOURCES}
)
add_executable(dflowmap
        main.cc
        DflowMapPass.cc
        DflowMapPass.h)
target_link_libraries(dflowmap chpBackend dflowmap-core dflowmap-common ActPassLib ActLib ActCommon dl)
if (EXISTS $ENV{ACT_HOME}/lib/libexpropt.a)
    target_link_libraries(dflowmap ExprOpt)
    target_link_libraries(dflowmap ABC)
    target_link_libraries(dflowmap pthread)
endif ()
if (EXISTS $ENV{ACT_HOME}/lib/libdflownetgen.a)
    target_link_libraries(dflowmap DflowNetGen)
endif ()
if(DEFINED NEED_LIBCXXFS)
    target_link_libraries(dflowmap stdc++fs)
endif()
