#----------------------------------------------------------------------------------------------------------------------
# Project config
#----------------------------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.21)
project(MoonDeckBuddy 
        VERSION 1.0.0
        DESCRIPTION "A server-side buddy app to help control the PC and Steam from SteamDeck via the MoonDeck plugin."
        HOMEPAGE_URL "https://github.com/FrogTheFrog/moondeck-buddy"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

if(MSVC)
    # warning level 4 and all warnings as errors
    add_compile_options(/W4 /WX)
else()
    # lots of warnings and all warnings as errors
    add_compile_options(-Wall -Wextra -pedantic -Werror)
endif()

#----------------------------------------------------------------------------------------------------------------------
# Build type parsing
#----------------------------------------------------------------------------------------------------------------------

set(DEBUG_MODE FALSE)
string(TOLOWER "${CMAKE_BUILD_TYPE}" CMAKE_BUILD_TYPE_LOWER)
if(CMAKE_BUILD_TYPE_LOWER MATCHES "debug")
    set(DEBUG_MODE TRUE)
endif()

#----------------------------------------------------------------------------------------------------------------------
# Static linking
#----------------------------------------------------------------------------------------------------------------------

if(NOT DEBUG_MODE)
    if(MINGW)
        set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    else()
        message(FATAL_ERROR "Compiler is not supported, yet...")
    endif()
endif()

#----------------------------------------------------------------------------------------------------------------------
# CLang-Tidy
#----------------------------------------------------------------------------------------------------------------------

if(NOT DEBUG_MODE)
    set(CMAKE_CXX_CLANG_TIDY clang-tidy)
    file(WRITE "${CMAKE_BINARY_DIR}/.clang-tidy" "Checks: '-*,llvm-twine-local'")
endif()

#----------------------------------------------------------------------------------------------------------------------
# Pre-Install
#----------------------------------------------------------------------------------------------------------------------

IF (WIN32)
    set(CPACK_GENERATOR "7Z;NSIS")
    set(CPACK_PACKAGE_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons\\\\app.ico")
    set(CPACK_PACKAGE_NAME ${PROJECT_NAME})
    set(CPACK_PACKAGE_VENDOR "MoonDeck")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
    set(CPACK_PACKAGE_EXECUTABLES "${PROJECT_NAME};${PROJECT_NAME}")
    set(CPACK_NSIS_IGNORE_LICENSE_PAGE ON)
    set(CPACK_NSIS_DISPLAY_NAME ${PROJECT_NAME})
    set(CPACK_NSIS_MUI_ICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons\\\\app.ico")
    set(CPACK_NSIS_MUI_UNIICON "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons\\\\app.ico")
    set(CPACK_NSIS_INSTALLED_ICON_NAME "bin\\${PROJECT_NAME}.exe")
    set(CPACK_NSIS_INSTALL_ROOT "\$%LOCALAPPDATA%\\Programs")
    set(CPACK_NSIS_ENABLE_UNINSTALL_BEFORE_INSTALL ON)
ELSE()
    message(FATAL_ERROR "OS is not supported!")
ENDIF()

#----------------------------------------------------------------------------------------------------------------------
# Subdirectories
#----------------------------------------------------------------------------------------------------------------------

add_subdirectory(src)

#----------------------------------------------------------------------------------------------------------------------
# Install
#----------------------------------------------------------------------------------------------------------------------

find_package(Qt6 REQUIRED COMPONENTS Core)
set(deploy_script "${CMAKE_CURRENT_BINARY_DIR}/deploy_${PROJECT_NAME}.cmake")

file(GENERATE OUTPUT ${deploy_script} CONTENT "
# Including the file pointed to by QT_DEPLOY_SUPPORT ensures the generated
# deployment script has access to qt_deploy_runtime_dependencies()
include(\"${QT_DEPLOY_SUPPORT}\")
qt_deploy_runtime_dependencies(
    EXECUTABLE \"\${QT_DEPLOY_BIN_DIR}/$<TARGET_FILE_NAME:${PROJECT_NAME}>\"
    GENERATE_QT_CONF
    VERBOSE
)")

install(TARGETS ${PROJECT_NAME})
install(SCRIPT ${deploy_script})
include(CPack)
