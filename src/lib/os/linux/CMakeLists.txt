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
# Header/Source files
#----------------------------------------------------------------------------------------------------------------------

file(GLOB HEADERS CONFIGURE_DEPENDS "include/os/linux/*.h")
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PRIVATE Qt6::Core Qt6::DBus X11::X11 X11::Xrandr ${PROCPS_LIBRARY} ossharedlib sharedlib)
target_include_directories(${LIBNAME} PUBLIC include)
