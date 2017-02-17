---
layout: post
title:  "How to embed build information from git to your program using CMake"
date:   2017-02-12 09:06:48 +0200
categories: cmake git
excerpt_separator: <!--more-->
---
It's useful to embed build information in your program. It helps your users
determine which version of the product they are using.  This is very important
when the users report defects in your code, when you build and ship frequently,
when you have continuous integration and so on.  Git provides reliable
versioning information about our code. We just need to embed this information
in our product. Bellow is how to do it using CMake. I will show a minimalistic
example which fulfils:

1. Major and minor version are computed from git tags 
2. Patch version automatically increases with each commit
2. Reliable at minimal build time
4. Git sha1 hash is embedded in each build

```console
$ ./main --version
version 0.1.3
git sha = 8ef01d3fbcde1ef46305f784a9cbe543e854a53e
git decription = v0.1-3-g8ef01d3
```
After 2 commits and some upstaged changes
```console
$ ./main --version
package 0.1.5-dirty
git sha = 23b104b091e418b62fde9db580d7d458713f9c64-dirty
git decription = v0.1-5-g23b104b-dirty
```

<!--more-->

Let's start with the main program:

program.cpp
```cpp
#include "version.h"
#include <cstdio>

int main(int argc, char **argv) {
    const Version version;
    printf("package %s\n",          version.package);
    printf("git sha = %s\n",        version.git_sha1);
    printf("git decription = %s\n", version.git_description);
    return 0;
}
```


version.h
```cpp
#pragma once
class Version {
public:
    Version();
    const char * const git_sha1;
    const char * const git_description;
    const char * const package;
};
```
At compile time the build system collects version information by invoking git.
Using this information the build system generates version.cpp. This file is then compiled and
linked into our main program. In case the content of version.cpp does not
change, recompilation will not occur. Bellow is a template from which CMake
generates the version.cpp

version.cpp.in
```cpp
#include "version.h"
Version::Version()
    : git_sha1("@GIT_SHA1@")
    , git_description("@GIT_DESCRIPTION@") 
    , package("@VERSION_MAJOR@.@VERSION_MINOR@.@VERSION_PATCH@")
    {}
```

CMake is a cross-platform build system generator. It is responsible for writing
the input files for a native build system. The native build sustem is the one
who builds our code, not CMake. On GNU/Linux the native build system can be make
or ninja. On Windows it can be Visual Studio or ninja. The native build system
needs to obtain information from git and generate version.cpp. This happens by
the following mechanism: the native build script invokes a small CMake script - 
version.cmake. The only reason this file is generated from a template is to
memorize, where to original source directory is located. There are other ways you
can achieve this, but I prefer this one. 

In version.cmake.in
```cmake
execute_process(COMMAND "@GIT_EXECUTABLE@" describe --dirty --tags --always
                WORKING_DIRECTORY "@CMAKE_SOURCE_DIR@"
                OUTPUT_VARIABLE GIT_DESCRIPTION
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND "@GIT_EXECUTABLE@" describe --match=NeVeRmAtCh --always --abbrev=40 --dirty
                WORKING_DIRECTORY "@CMAKE_SOURCE_DIR@"
                OUTPUT_VARIABLE GIT_SHA1
                ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
# Extract version from git description
if (DEFINED GIT_DESCRIPTION)
    string (REGEX MATCHALL "[0-9]+" _versionComponents "${GIT_DESCRIPTION}")
    list (LENGTH _versionComponents _len)
    if (${_len} GREATER 0)
        list(GET _versionComponents 0 VERSION_MAJOR)
    endif()
    if (${_len} GREATER 1)
        list(GET _versionComponents 1 VERSION_MINOR)
    endif()
    if (${_len} GREATER 2)
        list(GET _versionComponents 2 VERSION_PATCH)
    endif()
    string(REGEX MATCHALL "dirty" DIRTY "${GIT_DESCRIPTION}")
    if (NOT "${DIRTY}" STREQUAL "")
        set(VERSION_PATCH "${VERSION_PATCH}-dirty")
    endif()
endif()
# This will generate new version.cpp in memory. Then it will compare it with the existing file
# and write the new one to the disk, only if the two files differ.
configure_file(@CMAKE_SOURCE_DIR@/version.cpp.in @CMAKE_BINARY_DIR@/version.cpp @ONLY)
```

Finally, the main CMakelist.txt
```cmake
find_package(Git)
if(GIT_FOUND)
    # Remember where source and build directories are
    configure_file(${CMAKE_SOURCE_DIR}/version.cmake.in ${CMAKE_BINARY_DIR}/version.cmake @ONLY)
    # Generate initial version of version.cpp
    include(${CMAKE_BINARY_DIR}/version.cmake)
    # Targer that will update version, if it changed
    add_custom_target(version ${CMAKE_COMMAND} -P ${CMAKE_BINARY_DIR}/version.cmake)
    include_directories(.)
    add_executable(main main.cpp ${CMAKE_BINARY_DIR}/version.cpp)
    add_dependencies(main version)
else()
    message(FATAL_ERROR "Can't find git on this system")
endif()
```

Bellow is a demo that shows that you version automatically changes when you add a new tag to git.

```console
$ make
[  0%] Built target version
Scanning dependencies of target main
[ 50%] Building CXX object CMakeFiles/main.dir/main.o
[100%] Building CXX object CMakeFiles/main.dir/version.o
Linking CXX executable main
[100%] Built target main

$ ./main 
package 0.1.6
git sha = 6aaae267cfe55a0b0d5c625b5584e3f3049879dd
git decription = v0.1-6-g6aaae26

$ git tag -a v0.2 -m "my version 0.2"

$ make
[  0%] Built target version
Scanning dependencies of target main
[ 50%] Building CXX object CMakeFiles/main.dir/version.o
Linking CXX executable main
[100%] Built target main

$ ./main 
package 0.2.
git sha = 6aaae267cfe55a0b0d5c625b5584e3f3049879dd
git decription = v0.2
```
