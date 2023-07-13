#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Header files
#----------------------------------------------------------------------------------------------------------------------

set(HEADERS
    routing.h
    )

#----------------------------------------------------------------------------------------------------------------------
# Source files
#----------------------------------------------------------------------------------------------------------------------

set(SOURCES
    routing.cpp
    )

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

set(EXEC_NAME ${EXEC_NAME_BUDDY})
set(RESOURCES "${BUDDY_RESOURCES}/resources.qrc")

if(WIN32)
    list(APPEND RESOURCES "${BUDDY_RESOURCES}/windows/resources.rc")
endif()

add_executable(${EXEC_NAME} main.cpp ${HEADERS} ${SOURCES} ${RESOURCES})
target_link_libraries(${EXEC_NAME} PRIVATE Qt6::Core Qt6::Widgets utilslib serverlib oslib)
target_compile_definitions(${EXEC_NAME} PRIVATE EXEC_VERSION="${PROJECT_VERSION}")

if(NOT DEBUG_MODE)
    if(WIN32)
        set_target_properties(${EXEC_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()
