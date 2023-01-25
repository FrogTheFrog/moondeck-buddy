#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME osspecificlib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core DBus REQUIRED)
find_package(WaylandScanner REQUIRED)
find_package(Wayland COMPONENTS Client REQUIRED)
find_package(X11 REQUIRED)
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
    waylandresolutionhandler.h
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
    steamregistryobserver.cpp
    waylandresolutionhandler.cpp
    x11resolutionhandler.cpp
    )

#----------------------------------------------------------------------------------------------------------------------
# Wayland Scanner files
#----------------------------------------------------------------------------------------------------------------------

set(WAYLAND_SOURCES)
ecm_add_wayland_client_protocol(WAYLAND_SOURCES
                                PROTOCOL "${BUDDY_RESOURCES}/linux/wlr-output-management-unstable-v1.xml"
                                BASENAME "wlr-output-management-unstable-v1")

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

include (FindPackageHandleStandardArgs)

    find_path (PROCPS_INCLUDE_DIR proc/readproc.h)
    find_library (PROCPS_LIBRARY NAMES proc procps)
    find_package_handle_standard_args (procps DEFAULT_MSG PROCPS_LIBRARY PROCPS_INCLUDE_DIR)

message(STATUS ${PROCPS_LIBRARY})
message(STATUS ${PROCPS_INCLUDE_DIR})

add_library(${LIBNAME} ${HEADERS} ${SOURCES} ${WAYLAND_SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core Qt6::DBus sharedlib ${Wayland_LIBRARIES} X11::X11 X11::Xxf86vm ${PROCPS_LIBRARY})
target_include_directories(${LIBNAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
