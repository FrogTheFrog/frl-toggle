#----------------------------------------------------------------------------------------------------------------------
# Project config
#----------------------------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.27)
project(frltoggle 
        VERSION 1.2.0
        DESCRIPTION "Command line app for Windows to toggle Nvidia's Frame Rate Limiter."
        HOMEPAGE_URL "https://github.com/FrogTheFrog/frl-toggle"
        LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

#----------------------------------------------------------------------------------------------------------------------
# Compile settings
#----------------------------------------------------------------------------------------------------------------------

if(MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

#----------------------------------------------------------------------------------------------------------------------
# Subdirectories
#----------------------------------------------------------------------------------------------------------------------

add_subdirectory(externals/nvapi-wrapper)
add_subdirectory(src)
