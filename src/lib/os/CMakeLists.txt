#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME oslib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core Widgets Network REQUIRED)
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

add_subdirectory(steam)

#----------------------------------------------------------------------------------------------------------------------
# Header/Source files
#----------------------------------------------------------------------------------------------------------------------

file(GLOB HEADERS CONFIGURE_DEPENDS "include/os/*.h")
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core Qt6::Widgets Qt6::Network osspecificlib sharedlib utilslib ossteamlib PUBLIC ossharedlib)
target_include_directories(${LIBNAME} PUBLIC include)
