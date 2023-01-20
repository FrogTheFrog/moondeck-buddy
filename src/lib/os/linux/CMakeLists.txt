#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME osspecificlib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core DBus REQUIRED)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Header files
#----------------------------------------------------------------------------------------------------------------------

set(HEADERS
    autostarthandler.h
    cursorhandler.h
    nativepcstatehandler.h
    nativeprocesshandler.h
    nativeresolutionhandler.h
    registryfileparser.h
    registryfilewatcher.h
    steamregistryobserver.h
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
    registryfileparser.cpp
    registryfilewatcher.cpp
    steamregistryobserver.cpp
    )

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core Qt6::DBus sharedlib)
target_include_directories(${LIBNAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
