#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME ossharedlib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()

#----------------------------------------------------------------------------------------------------------------------
# Header/Source files
#----------------------------------------------------------------------------------------------------------------------

file(GLOB HEADERS CONFIGURE_DEPENDS "include/os/shared/*.h")
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core sharedlib)
target_include_directories(${LIBNAME} PUBLIC include)
