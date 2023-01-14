#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME osspecificlib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Header files
#----------------------------------------------------------------------------------------------------------------------

set(HEADERS
    autostarthandler.h
    cursorhandler.h
    nativeprocesshandler.h
    pcstatehandler.h
    registryfileparser.h
    registryfilewatcher.h
    resolutionhandler.h
    steamregistryobserver.h
    )

#----------------------------------------------------------------------------------------------------------------------
# Source files
#----------------------------------------------------------------------------------------------------------------------

set(SOURCES
    autostarthandler.cpp
    cursorhandler.cpp
    nativeprocesshandler.cpp
    pcstatehandler.cpp
    registryfileparser.cpp
    registryfilewatcher.cpp
    resolutionhandler.cpp
    steamregistryobserver.cpp
    )

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core sharedlib)
target_include_directories(${LIBNAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
