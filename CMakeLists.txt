cmake_minimum_required(VERSION 2.8)
project(drm_test)

#set(CMAKE_CXX_STANDARD 11)
set(CMAKE_SYSTEM_NAME Linux)
set(CMAKE_BUILD_TYPE Release)

set(CMAKE_C_COMPILER gcc)
set(CMAKE_CXX_COMPILER g++)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11 -march=native -O3 -pthread")
SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")

include_directories(${CMAKE_SOURCE_DIR}/
					#/usr/include
					/usr/include/libdrm)
					#../samples-rklinux-opengl-master/native/include)

				#link_directories(#/usr/lib
				#/usr/lib/aarch64-linux-gnu
				# ../samples-rklinux-opengl-master/native/lib)
set(link_libs /usr/lib/aarch64-linux-gnu/libdrm.so
					   pthread
			)
			  #/usr/lib/aarch64-linux-gnu/libdrm_amdgpu.so
			  #/usr/lib/aarch64-linux-gnu/libdrm_freedreno.so
			  #/usr/lib/aarch64-linux-gnu/libdrm_nouveau.so)
		      #/usr/lib/aarch64-linux-gnu/libdrm_radeon.so)
			  #/usr/lib/aarch64-linux-gnu/libdrm_tegra.so)

aux_source_directory(${CMAKE_SOURCE_DIR}/ _FILE)
add_executable(drm_test drm_test.cpp ${_FILE})
target_link_libraries(drm_test ${link_libs})
#target_include_directories(glsample_gbm PUBLIC ./native/include/EGLs/winsys_gbm)

#target_link_libraries(glsample_gbm mali-gbm drm)
