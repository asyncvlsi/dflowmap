#
# This file is part of the ACT library
#
# Copyright (c) 2021 Rui Li
#
# This program is free software; you can redistribute it and/or
# modify it under the terms of the GNU General Public License
# as published by the Free Software Foundation; either version 2
# of the License, or (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program; if not, write to the Free Software
# Foundation, Inc., 51 Franklin Street, Fifth Floor,
# Boston, MA  02110-1301, USA.
#

cmake_minimum_required(VERSION 3.15)
project(dflowmap)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")
add_compile_options(-Wall
        -Wextra
        -Wconversion-null
        -Wmissing-declarations
        -Woverlength-strings)
add_compile_options(
        -Wpointer-arith
        -Wunused-local-typedefs
        -Wunused-result
        -Wvarargs
        -Wvla
        -Wwrite-strings
        -Wformat-security
        -Wundef)
add_compile_options(-O2)

include_directories(./)
add_library(ActLib SHARED IMPORTED)
set_target_properties(ActLib PROPERTIES IMPORTED_LOCATION $ENV{ACT_HOME}/lib/libact.a)
add_library(ActCommon SHARED IMPORTED)
set_target_properties(ActCommon PROPERTIES IMPORTED_LOCATION $ENV{ACT_HOME}/lib/libvlsilib.a)
if (EXISTS $ENV{ACT_HOME}/lib/libexpropt.a)
    add_library(ExprOpt SHARED IMPORTED)
    set_target_properties(ExprOpt PROPERTIES IMPORTED_LOCATION $ENV{ACT_HOME}/lib/libexpropt.a)
else ()
    message(
            "libexpropt.a missing from $ENV{ACT_HOME}/lib. The tool cannot do logic optimizations or performance modeling for the customized function unit processes!")
endif ()
if (EXISTS $ENV{ACT_HOME}/lib/libexproptcommercial.a)
    message(
            "Found commercial tool to do logic optimizations and performance modeling for the customized function unit processes!")
    add_library(ExprOptCommercial SHARED IMPORTED)
    set_target_properties(ExprOptCommercial PROPERTIES IMPORTED_LOCATION $ENV{ACT_HOME}/lib/libexproptcommercial.a)
endif ()

if (EXISTS $ENV{ACT_HOME}/lib/libdflownetgen.a)
    message("Found dataflow netlist generator!")
    add_library(DflowNetGen STATIC IMPORTED)
    set_target_properties(DflowNetGen PROPERTIES IMPORTED_LOCATION $ENV{ACT_HOME}/lib/libdflownetgen.a)
endif ()

# Set the output directory of static libraries
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Set the output directory of executables
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin)

set(CMAKE_INSTALL_PREFIX $ENV{ACT_HOME} CACHE PATH "installation path" FORCE)

add_subdirectory(src)

# specify install targets
install(
        TARGETS dflowmap
        DESTINATION bin
)

install(
        TARGETS dflowchp
        DESTINATION lib
)

install(
        DIRECTORY src/common/
        DESTINATION include/act/dflow/common
        COMPONENT Development
        FILES_MATCHING
        PATTERN "common.h"
        PATTERN "Constant.h"
        PATTERN "Helper.h")

install(
        DIRECTORY dflow_std/
        DESTINATION act/std/dflow
        COMPONENT Development
        FILES_MATCHING
        PATTERN "lib*.act")
