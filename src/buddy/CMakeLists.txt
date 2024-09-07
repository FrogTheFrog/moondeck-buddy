#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

set(EXEC_NAME ${EXEC_NAME_BUDDY})
set(RESOURCES "${BUDDY_RESOURCES}/resources.qrc")

if(WIN32)
    configure_file(
            "${BUDDY_RESOURCES}/windows/version.rc.in"
            "${CMAKE_CURRENT_BINARY_DIR}/version.rc"
            @ONLY)

    list(APPEND RESOURCES "${BUDDY_RESOURCES}/windows/resources.rc")
    list(APPEND RESOURCES "${CMAKE_CURRENT_BINARY_DIR}/version.rc")
endif()

add_executable(${EXEC_NAME} main.cpp routing.h routing.cpp ${RESOURCES})
target_link_libraries(${EXEC_NAME} PRIVATE Qt6::Core Qt6::Widgets utilslib serverlib oslib sharedlib)
target_compile_definitions(${EXEC_NAME} PRIVATE EXEC_VERSION="${PROJECT_VERSION}")

if(NOT DEBUG_MODE)
    if(WIN32)
        set_target_properties(${EXEC_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    endif()
endif()
