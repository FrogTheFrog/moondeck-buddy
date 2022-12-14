#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

set(EXEC_NAME ${EXEC_NAME_STREAM})
set(RESOURCES "../../resources/resources.qrc")

if(WIN32)
    list(APPEND RESOURCES "../../resources/windows/resources.rc")
endif()

add_executable(${EXEC_NAME} main.cpp ${RESOURCES})
target_link_libraries(${EXEC_NAME} PRIVATE Qt6::Core utilslib)

if(NOT DEBUG_MODE)
    if(WIN32)
        set_target_properties(${EXEC_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    else()
        message(FATAL_ERROR "OS is not supported!")
    endif()
endif()
