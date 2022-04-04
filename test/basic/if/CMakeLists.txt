enable_testing()
include(GoogleTest)
add_executable(
        IfTest
        if_test.cc
)
gtest_discover_tests(IfTest)
if(EXISTS $ENV{ACT_HOME}/lib/libexpropt.a)
    if(EXISTS $ENV{ACT_HOME}/lib/libexproptcommercial.a)
        target_link_libraries(IfTest chpBackend dflowmap-core dflowmap-common ActLib ActCommon ExprOpt ExprOptCommercial gtest_main)
    else()
        target_link_libraries(IfTest chpBackend dflowmap-core dflowmap-common ActLib ActCommon ExprOpt gtest_main)
    endif()
else()
    target_link_libraries(IfTest chpBackend dflowmap-core dflowmap-common ActLib ActCommon gtest_main)
endif()
