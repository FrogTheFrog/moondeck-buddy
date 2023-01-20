#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME oslib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core Widgets REQUIRED)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Subdirectories
#----------------------------------------------------------------------------------------------------------------------

add_subdirectory(shared)

IF(WIN32)
    add_subdirectory(win)
ELSEIF(UNIX)
    add_subdirectory(linux)
ELSE()
    message(FATAL_ERROR "OS is not supported!")
ENDIF()

#----------------------------------------------------------------------------------------------------------------------
# Header files
#----------------------------------------------------------------------------------------------------------------------

set(HEADERS
    autostarthandlerinterface.h
    cursorhandlerinterface.h
    nativepcstatehandlerinterface.h
    nativeprocesshandlerinterface.h
    nativeresolutionhandlerinterface.h
    pccontrol.h
    pcstatehandler.h
    processhandler.h
    resolutionhandler.h
    steamhandler.h
    steamregistryobserverinterface.h
    streamstatehandlerinterface.h
    )

#----------------------------------------------------------------------------------------------------------------------
# Source files
#----------------------------------------------------------------------------------------------------------------------

set(SOURCES
    pccontrol.cpp
    pcstatehandler.cpp
    processhandler.cpp
    resolutionhandler.cpp
    steamhandler.cpp
    )

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core Qt6::Widgets osspecificlib ossharedlib sharedlib)
target_include_directories(${LIBNAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
