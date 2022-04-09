add_library( dflowmap-core
        DflowGenerator.cc
        DflowGenerator.h
        ProcGenerator.cc
        ProcGenerator.h
        Metrics.cc
        Metrics.h
        NameGenerator.cc
        NameGenerator.h)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS}")
