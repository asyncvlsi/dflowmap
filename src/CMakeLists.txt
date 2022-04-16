include_directories( $ENV{ACT_HOME}/include)
add_subdirectory(backend)
add_subdirectory(common)
add_subdirectory(core)
add_executable(dflowmap
        main.cc
)
if(EXISTS $ENV{ACT_HOME}/lib/libexpropt.a)
    if(EXISTS $ENV{ACT_HOME}/lib/libexproptcommercial.a)
        target_link_libraries(dflowmap chpBackend netlistBackend dflowmap-core dflowmap-common ActLib ActCommon ExprOpt
                ExprOptCommercial)
    else()
        target_link_libraries(dflowmap chpBackend netlistBackend dflowmap-core dflowmap-common ActLib ActCommon ExprOpt)
    endif()
else()
    target_link_libraries(dflowmap chpBackend netlistBackend dflowmap-core dflowmap-common ActLib ActCommon)
endif()