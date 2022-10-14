#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Core Widgets)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Subdirectories
#----------------------------------------------------------------------------------------------------------------------

add_subdirectory(shared)
add_subdirectory(msgs)
add_subdirectory(server)
add_subdirectory(utils)
add_subdirectory(os)

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

set(RESOURCES "../resources/resources.qrc")

if(WIN32)
    list(APPEND RESOURCES "../resources/windows.rc")
endif()

add_executable(${PROJECT_NAME} main.cpp ${RESOURCES})
target_link_libraries(${PROJECT_NAME} PRIVATE Qt6::Core Qt6::Widgets utilslib serverlib oslib)

if(NOT DEBUG_MODE)
    if(WIN32)
        set_target_properties(${PROJECT_NAME} PROPERTIES WIN32_EXECUTABLE TRUE)
    else()
        message(FATAL_ERROR "OS is not supported!")
    endif()
endif()
