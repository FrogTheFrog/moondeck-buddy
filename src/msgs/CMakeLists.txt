#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME msgslib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Header files
#----------------------------------------------------------------------------------------------------------------------

set(HEADERS
    in/abortpairing.h
    in/closesteam.h
    in/hello.h
    in/launchapp.h
    in/login.h
    in/pair.h
    in/pairstatus.h
    in/pcstate.h
    in/restartpc.h
    in/shutdownpc.h
    in/steamstatus.h
    out/controltypetaken.h
    out/invalidmessage.h
    out/messageaccepted.h
    out/notpaired.h
    out/paired.h
    out/pairing.h
    out/pcstate.h
    out/steamstatus.h
    out/versionmismatch.h
    converter.h
    )

#----------------------------------------------------------------------------------------------------------------------
# Source files
#----------------------------------------------------------------------------------------------------------------------

set(SOURCES
    in/abortpairing.cpp
    in/closesteam.cpp
    in/hello.cpp
    in/launchapp.cpp
    in/login.cpp
    in/pair.cpp
    in/pairstatus.cpp
    in/pcstate.cpp
    in/restartpc.cpp
    in/shutdownpc.cpp
    in/steamstatus.cpp
    out/controltypetaken.cpp
    out/invalidmessage.cpp
    out/messageaccepted.cpp
    out/notpaired.cpp
    out/paired.cpp
    out/pairing.cpp
    out/pcstate.cpp
    out/steamstatus.cpp
    out/versionmismatch.cpp
    )

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core sharedlib)
target_include_directories(${LIBNAME} PUBLIC "${CMAKE_CURRENT_LIST_DIR}/..")
