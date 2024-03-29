#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME osspecificlib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()

find_library(PSAPI Psapi REQUIRED)
find_library(POWRPROF PowrProf REQUIRED)
add_compile_definitions(PSAPI_VERSION=1)

#----------------------------------------------------------------------------------------------------------------------
# Header files
#----------------------------------------------------------------------------------------------------------------------

set(HEADERS
    autostarthandler.h
    cursorhandler.h
    nativepcstatehandler.h
    nativeprocesshandler.h
    nativeresolutionhandler.h
    regkey.h
    processtracker.h
    processenumerator.h
    steamregistryobserver.h
    streamstatehandler.h
    )

#----------------------------------------------------------------------------------------------------------------------
# Source files
#----------------------------------------------------------------------------------------------------------------------

set(SOURCES
    autostarthandler.cpp
    cursorhandler.cpp
    nativepcstatehandler.cpp
    nativeprocesshandler.cpp
    nativeresolutionhandler.cpp
    regkey.cpp
    processtracker.cpp
    processenumerator.cpp
    steamregistryobserver.cpp
    streamstatehandler.cpp
    )

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core Psapi PowrProf sharedlib)
target_include_directories(${LIBNAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
