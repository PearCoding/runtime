if(NOT DEFINED INTELFPGAOCL_ROOT_DIR)
    find_path(INTELFPGAOCL_SEARCH_PATH aocl
        PATHS ENV INTELFPGAOCLSDKROOT
        PATH_SUFFIXES bin)
    get_filename_component(INTELFPGAOCL_ROOT_DIR ${INTELFPGAOCL_SEARCH_PATH} DIRECTORY)
else()
    message(STATUS "Using user defined Intel FPGA OpenCL directory: ${INTELFPGAOCL_ROOT_DIR}")
endif()

find_program(IntelFPGAOpenCL_XOCC aoc PATHS ${INTELFPGAOCL_ROOT_DIR}/bin NO_DEFAULT_PATH)

get_filename_component(INTELFPGA_ROOT_DIR "${INTELFPGAOCL_ROOT_DIR}" DIRECTORY)

get_filename_component(IntelFPGAOpenCL_VERSION "${INTELFPGA_ROOT_DIR}" NAME)
string(REGEX REPLACE "([0-9]+)\\.[0-9\\.]+" "\\1" IntelFPGAOpenCL_MAJOR_VERSION "${IntelFPGAOpenCL_VERSION}")
string(REGEX REPLACE "[0-9]+\\.([0-9\\.]+)" "\\1" IntelFPGAOpenCL_MINOR_VERSION "${IntelFPGAOpenCL_VERSION}")

find_program(IntelFPGAOpenCL_AOC aoc
    PATHS ${INTELFPGAOCL_ROOT_DIR}/bin NO_DEFAULT_PATH)

find_program(IntelFPGAOpenCL_AOCL aocl
    PATHS ${INTELFPGAOCL_ROOT_DIR}/bin NO_DEFAULT_PATH)

execute_process(COMMAND ${IntelFPGAOpenCL_AOCL} compile-config OUTPUT_VARIABLE IntelFPGAOpenCL_INCLUDE_DIRS)
string(REGEX MATCHALL "-I[^ \t]+" IntelFPGAOpenCL_INCLUDE_DIRS "${IntelFPGAOpenCL_INCLUDE_DIRS}")
string(REPLACE "-I" "" IntelFPGAOpenCL_INCLUDE_DIRS "${IntelFPGAOpenCL_INCLUDE_DIRS}")

execute_process(COMMAND ${IntelFPGAOpenCL_AOCL} link-config OUTPUT_VARIABLE INTELFPGAOCL_LINK_FLAGS)
string(REGEX MATCHALL "-L[^ \t\n]+" INTELFPGAOCL_LINK_DIRS "${INTELFPGAOCL_LINK_FLAGS}")
string(REPLACE "-L" "" INTELFPGAOCL_LINK_DIRS "${INTELFPGAOCL_LINK_DIRS}")
string(REGEX MATCHALL "-l[^ \t\n]+" INTELFPGAOCL_LINK_LIBS "${INTELFPGAOCL_LINK_FLAGS}")
string(REPLACE "-l" "" INTELFPGAOCL_LINK_LIBS "${INTELFPGAOCL_LINK_LIBS}")

set(IntelFPGAOpenCL_LIBRARIES)
foreach(INTELFPGAOCL_LIB ${INTELFPGAOCL_LINK_LIBS})
    find_library(${INTELFPGAOCL_LIB}_PATH ${INTELFPGAOCL_LIB}
        PATHS ${INTELFPGAOCL_LINK_DIRS} NO_DEFAULT_PATH)
    set(IntelFPGAOpenCL_LIBRARIES ${IntelFPGAOpenCL_LIBRARIES} ${${INTELFPGAOCL_LIB}_PATH})
    mark_as_advanced(${INTELFPGAOCL_LIB}_PATH)
endforeach()

string(REPLACE " " ":" IntelFPGAOpenCL_RPATH "${INTELFPGAOCL_LINK_DIRS}")
set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
set(CMAKE_INSTALL_RPATH
    "${CMAKE_INSTALL_RPATH}:${INTELFPGAOCL_LINK_DIRS}")

mark_as_advanced(INTELFPGAOCL_SEARCH_PATH
    INTELFPGA_ROOT_DIR
    INTELFPGAOCL_LINK_DIRS)

set(IntelFPGAOpenCL_EXPORTS
    IntelFPGAOpenCL_AOCL
    IntelFPGAOpenCL_AOC
    IntelFPGAOpenCL_INCLUDE_DIRS
    IntelFPGAOpenCL_LIBRARIES
    IntelFPGAOpenCL_RPATH
    IntelFPGAOpenCL_VERSION
    IntelFPGAOpenCL_MAJOR_VERSION
    IntelFPGAOpenCL_MINOR_VERSION)
mark_as_advanced(IntelFPGAOpenCL_EXPORTS)

include(FindPackageHandleStandardArgs)
# Set IntelFPGAOpenCLHLS_FOUND to TRUE if all listed variables were found.
find_package_handle_standard_args(IntelFPGAOpenCL DEFAULT_MSG ${IntelFPGAOpenCL_EXPORTS})
