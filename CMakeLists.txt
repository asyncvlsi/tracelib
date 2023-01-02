#-------------------------------------------------------------------------
#
#  Copyright (c) 2023 Rajit Manohar
#
#  This library is free software; you can redistribute it and/or
#  modify it under the terms of the GNU Lesser General Public
#  License as published by the Free Software Foundation; either
#  version 2.1 of the License, or (at your option) any later version.
#
#  This library is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor,
#  Boston, MA  02110-1301, USA.
#
#-------------------------------------------------------------------------
cmake_minimum_required(VERSION 3.10)

project(tracelib)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/ext)

add_library(tracelib STATIC tracelib.c)

add_library(trace_vcd SHARED vcd.cc)

add_library(trace_lxt2 SHARED lxt2.c ext/lxt2_write.c)
target_link_libraries(trace_lxt2 libz.a)

message(STATUS "Prefix is " ${CMAKE_INSTALL_PREFIX})

if(DEFINED ENV{ACT_HOME})
  add_compile_options(-DTRACELIB_ENV=\"ACT_HOME\")
  include_directories($ENV{ACT_HOME}/include)
  link_directories($ENV{ACT_HOME}/lib)
  add_library(trace_atr SHARED atr.c)
  target_link_libraries(trace_atr libvlsilib_sh.so)
  install(
   TARGETS
   trace_atr
   DESTINATION lib
  )
endif()

install(
   TARGETS
   tracelib
   trace_vcd
   trace_lxt2
   DESTINATION lib
)
install(
   FILES "tracelib.h"
   DESTINATION include
)
