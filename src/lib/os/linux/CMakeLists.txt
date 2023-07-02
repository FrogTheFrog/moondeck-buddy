#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME osspecificlib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core DBus REQUIRED)
find_package(X11 REQUIRED)
find_library(PROCPS_LIBRARY NAMES proc procps REQUIRED)
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
    steamprocesslistobserver.h
    steamregistryobserver.h
    x11resolutionhandler.h
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
    steamprocesslistobserver.cpp
    steamregistryobserver.cpp
    x11resolutionhandler.cpp
    )

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES} ${WAYLAND_SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core Qt6::DBus X11::X11 X11::Xrandr ${PROCPS_LIBRARY} sharedlib)
target_include_directories(${LIBNAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
