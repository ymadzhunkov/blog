---
layout: post
title:  "How to generate coverage reports using CMake/gcov"
date:   2017-02-25 09:06:48 +0200
categories: cmake coverage
excerpt_separator: <!--more-->
---
Global option to control coverage report
```cmake
option(REPORT_COVERAGE            "Generate coverage report" OFF)
```

Compiler flags
```cmake
if(REPORT_COVERAGE)
    if(NOT (CMAKE_COMPILER_IS_GNUCC OR CMAKE_COMPILER_IS_GNUCXX) AND (NOT "${CMAKE_C_COMPILER_ID}" STREQUAL "Clang"))
        message(FATAL_ERROR "Coverage needs GCC compiler. The current compiler ${CMAKE_C_COMPILER_ID} is not GNU gcc!")
    endif()

    if(NOT CMAKE_BUILD_TYPE STREQUAL "DEBUG")
        message(FATAL_ERROR "Coverage: Code coverage results with an optimised (non-Debug) build may be misleading! Add -DCMAKE_BUILD_TYPE=Debug")
    endif()
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS}   -g -O0 -fprofile-arcs -ftest-coverage")
endif(REPORT_COVERAGE)
```

Adding unit tests
```cmake
macro(add_unit_test_target)
    cmake_parse_arguments(ARG "" "TARGET" "" ${ARGN})
    set(UNIT_TEST_TARGET ${ARG_TARGET})

    #Run unit test as part of build
    add_custom_command(TARGET ${UNIT_TEST_TARGET} POST_BUILD COMMAND $<TARGET_FILE:${UNIT_TEST_TARGET}>)

    #Register unit test in ctests
    add_test(NAME unittest_${UNIT_TEST_TARGET} COMMAND $<TARGET_FILE:${UNIT_TEST_TARGET}>)
endmacro()


macro(add_doctest_test)
    cmake_parse_arguments(ARG "" "NAME" "SOURCES;TEST_SOURCES;LIBRARIES" ${ARGN})

    set(DOCTEST_INCLUDE ${PROJECT_SOURCE_DIR}/external/doctest)
    set(DOCTEST_MAIN ${DOCTEST_INCLUDE}/doctest_main.cpp)
    set(UNIT_TEST_TARGET ${ARG_NAME})
    add_executable(${UNIT_TEST_TARGET} ${ARG_SOURCES} ${ARG_TEST_SOURCES} ${DOCTEST_MAIN})
    target_include_directories(${UNIT_TEST_TARGET} PUBLIC ${DOCTEST_INCLUDE})
    add_unit_test_target(TARGET ${UNIT_TEST_TARGET})
    if (NOT "${ARG_LIBRARIES}" STREQUAL "")
        target_link_libraries(${UNIT_TEST_TARGET} ${ARG_LIBRARIES})
    endif()
endmacro()
```

```cmake
if(REPORT_COVERAGE)
    FIND_PROGRAM(LCOV_PATH lcov)
    FIND_PROGRAM(GENHTML_PATH genhtml)

    if (NOT GENHTML_PATH) 
        message(FATAL_ERROR "Coverage needs lcov! Currently it's not found in this system")
    endif()

    if (NOT GENHTML_PATH) 
        message(FATAL_ERROR "Coverage needs genhtml! Currently it's not found in this system")
    endif()

    set(FILTER_FILES '/usr/include/*' 'doctest.h' '${PROJECT_SOURCE_DIR}/unittests/*')
    set(HTML_REPORT_ARGUMENTS --no-function-coverage --highlight --legend --branch-coverage --demangle-cpp)

    set(COVERAGE_REPORT_DIR ${PROJECT_BINARY_DIR}/tests/coverage_report)

    set(CODE_BASE_INFO   ${COVERAGE_REPORT_DIR}/code_base.info)
    set(UNFILTERED_INFO  ${COVERAGE_REPORT_DIR}/coverage_unfiltered.info)
    set(LOG              ${COVERAGE_REPORT_DIR}/log.txt)
    set(TEST_INFO        ${COVERAGE_REPORT_DIR}/tests.info)
    set(FILTERED_INFO    ${COVERAGE_REPORT_DIR}/unit_tests_suite) 

    add_custom_target(unit_test_coverage_report
        # Collect code base from the entire project
        COMMAND ${LCOV_PATH} --directory ${PROJECT_BINARY_DIR} --capture --initial -o ${CODE_BASE_INFO} > ${LOG} 2>&1
        # Zero the coverage counters.
        COMMAND ${CMAKE_COMMAND} -DPROJECT_BINARY_DIR="${PROJECT_BINARY_DIR}" -P "${PROJECT_SOURCE_DIR}/cmake/ZeroCoverageCounters.cmake" >> ${LOG} 2>&1

        # Run unit tests.
        COMMAND ${CMAKE_CTEST_COMMAND} --output-on-failure -R 'unittest_*' >> ${LOG} 2>&1
        # Collect coverage data from last run
        COMMAND ${LCOV_PATH} --directory ${PROJECT_BINARY_DIR} --capture --output-file ${TEST_INFO} >> ${LOG} 2>&1
        # Combine last coverage run with code base
        COMMAND ${LCOV_PATH} -a ${TEST_INFO} -a ${CODE_BASE_INFO} -o ${UNFILTERED_INFO} >> ${LOG} 2>&1
        # Filter 'usr/include' and other similar files
        COMMAND  ${LCOV_PATH} --remove ${UNFILTERED_INFO} ${FILTER_FILES} -o ${FILTERED_INFO} >> ${LOG} 2>&1
        # Generate html report 
        COMMAND ${GENHTML_PATH} ${FILTERED_INFO} ${HTML_REPORT_ARGUMENTS} --output-directory ${COVERAGE_REPORT_DIR} >> ${LOG} 2>&1
        # Working directory on top of binary dir, so we can discover all tests
        WORKING_DIRECTORY ${PROJECT_BINARY_DIR}
        COMMENT "Generating coverage report for unit tests -> ${COVERAGE_REPORT_DIR}/index.html"
    )

    set(FILTERED_INFO    ${COVERAGE_REPORT_DIR}/integration_tests_suite) 


endif(REPORT_COVERAGE)
```

ZeroCoverageCounters.cmake
```cmake
file(GLOB_RECURSE GCDA_FILES "${PROJECT_BINARY_DIR}/*.gcda")
if(NOT GCDA_FILES STREQUAL "")
  file(REMOVE ${GCDA_FILES})
endif()
```
