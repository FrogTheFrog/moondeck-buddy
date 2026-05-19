#----------------------------------------------------------------------------------------------------------------------
# Lib config
#----------------------------------------------------------------------------------------------------------------------

set(LIBNAME jsonlib)

#----------------------------------------------------------------------------------------------------------------------
# External dependencies
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 COMPONENTS Core REQUIRED)
qt_standard_project_setup()

include(FetchContent)
FetchContent_Declare(
        glaze
        GIT_REPOSITORY https://github.com/stephenberry/glaze.git
        GIT_TAG cmake-glaze-install-option
        GIT_SHALLOW TRUE
)
FetchContent_MakeAvailable(glaze)

#----------------------------------------------------------------------------------------------------------------------
# Header/Source files
#----------------------------------------------------------------------------------------------------------------------

file(GLOB HEADERS CONFIGURE_DEPENDS "include/json/*.h")
file(GLOB SOURCES CONFIGURE_DEPENDS "*.cpp")

#----------------------------------------------------------------------------------------------------------------------
# Target config
#----------------------------------------------------------------------------------------------------------------------

add_library(${LIBNAME} ${HEADERS} ${SOURCES})
target_link_libraries(${LIBNAME} PUBLIC glaze::glaze PRIVATE Qt6::Core)
target_include_directories(${LIBNAME} PUBLIC include)
