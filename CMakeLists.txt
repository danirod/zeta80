# zeta80 configuration script
# This script is intented to be used by CMake
# Copyright (c) 2015, Dani Rodríguez
# All rights reserved.
# 
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions
# are met:
# 
# * Redistributions of source code must retain the above copyright notice,
#   this list of conditions and the following disclaimer.
# 
# * Redistributions in binary form must reproduce the above copyright
#   notice, this list of conditions and the following disclaimer in the
#   documentation and/or other materials provided with the distribution.
# 
# * Neither the name of the project's author nor the names of its
#   contributors may be used to endorse or promote products derived from
#   this software without specific prior written permission.
# 
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
# "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
# for a particular purpose are disclaimed. in no event shall the copyright
# holder or contributors be liable for any direct, indirect, incidental,
# special, exemplary, or consequential damages (including, but not limited
# to, procurement of substitute goods or services; loss of use, data, or
# profits; or business interruption) however caused and on any theory of
# liability, whether in contract, strict liability, or tort (including
# negligence or otherwise) arising in any way out of the use of this
# software, even if advised of the possibility of such damage.

project(zeta80)
cmake_minimum_required(VERSION 2.8)

# Include /cmake directory when looking for modules
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set(ZETA80_INCLUDE ${CMAKE_SOURCE_DIR}/include)
set(ZETA80_SRC ${CMAKE_SOURCE_DIR}/src)

add_subdirectory(src)
add_subdirectory(tests)
