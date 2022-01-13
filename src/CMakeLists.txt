add_subdirectory(backend/chp)
add_subdirectory(common)
add_subdirectory(core)

add_executable(dflowmap
        main.cc
)

target_link_libraries(dflowmap chpBackend core common ActLib ActCommon ExprOpt
        ExprOptCommercial)
