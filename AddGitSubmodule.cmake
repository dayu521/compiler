#special thanks to:
#https://www.scivision.dev/cmake-git-submodule/

cmake_minimum_required(VERSION 3.5)

function(add_git_submodule dir)
# add a Git submodule directory to CMake, assuming the
# Git submodule directory is a CMake project.
#
# Usage: in CMakeLists.txt
# 
# include(AddGitSubmodule.cmake)
# add_git_submodule(mysubmod_dir)

find_package(Git REQUIRED)

if(NOT EXISTS ${dir}/CMakeLists.txt)
  execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive --depth 1 -- ${dir}
    WORKING_DIRECTORY ${PROJECT_SOURCE_DIR}
    COMMAND_ERROR_IS_FATAL ANY)
endif()

add_subdirectory(${dir})

endfunction(add_git_submodule) 