# GLFW
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
add_subdirectory(glfw)

# KissFFT
add_subdirectory(kissfft)

# libsndfile - we'll assume it's installed via brew
find_package(PkgConfig REQUIRED)
pkg_check_modules(SNDFILE REQUIRED sndfile)
include_directories(${SNDFILE_INCLUDE_DIRS})
link_directories(${SNDFILE_LIBRARY_DIRS})