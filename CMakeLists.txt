#----------------------------------------------------------------------------------------------------------------------
# Project config
#----------------------------------------------------------------------------------------------------------------------

cmake_minimum_required(VERSION 3.27)
project(MoonDeckBuddy 
        VERSION 1.8.0
        DESCRIPTION "A server-side buddy app to help control the PC and Steam from SteamDeck via the MoonDeck plugin."
        HOMEPAGE_URL "https://github.com/FrogTheFrog/moondeck-buddy"
        LANGUAGES C CXX)

set(EXEC_NAME_BUDDY MoonDeckBuddy)
set(EXEC_NAME_STREAM MoonDeckStream)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(BUDDY_RESOURCES "${CMAKE_CURRENT_LIST_DIR}/resources")
set(ENABLE_CLANG_TIDY OFF CACHE BOOL "Enable clang-tidy build (slow)")

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
# CLang-Tidy
#----------------------------------------------------------------------------------------------------------------------

if(ENABLE_CLANG_TIDY)
    if(NOT MSVC)
        message(STATUS "Clang-tidy build is enabled!")
        set(CMAKE_CXX_CLANG_TIDY clang-tidy)
        file(WRITE "${CMAKE_BINARY_DIR}/.clang-tidy" "Checks: '-*,llvm-twine-local'")
    endif()
endif()

#----------------------------------------------------------------------------------------------------------------------
# Pre-Install
#----------------------------------------------------------------------------------------------------------------------

IF (WIN32)
    # General options
    set(CPACK_PACKAGE_VENDOR "MoonDeck")
    set(CPACK_PACKAGE_INSTALL_DIRECTORY ${PROJECT_NAME})
    set(CPACK_PACKAGE_EXECUTABLES "${EXEC_NAME_BUDDY};${EXEC_NAME_BUDDY}")
    set(CPACK_OUTPUT_FILE_PREFIX "../dist")
    set(CPACK_GENERATOR "7Z;INNOSETUP")

    # INNOSETUP specific options
    set(CPACK_INNOSETUP_IGNORE_LICENSE_PAGE ON)
    set(CPACK_INNOSETUP_USE_MODERN_WIZARD ON)
    set(CPACK_INNOSETUP_SETUP_PrivilegesRequired "lowest")
    set(CPACK_INNOSETUP_SETUP_RestartApplications OFF)
    set(CPACK_INNOSETUP_SETUP_CloseApplications "Force")
    set(CPACK_INNOSETUP_SETUP_SetupIconFile "${CMAKE_CURRENT_SOURCE_DIR}/resources/icons/moondeckbuddy.ico")
    set(CPACK_INNOSETUP_SETUP_UninstallDisplayIcon "{app}/bin/${EXEC_NAME_BUDDY}.exe")
    set(CPACK_INNOSETUP_CODE_FILES "${BUDDY_RESOURCES}/windows/innocode.iss")
    set(CPACK_INNOSETUP_EXTRA_SCRIPTS "${BUDDY_RESOURCES}/windows/innoextra.iss")
ELSEIF(NOT UNIX)
    message(FATAL_ERROR "OS is not supported!")
ENDIF()

#----------------------------------------------------------------------------------------------------------------------
# Subdirectories
#----------------------------------------------------------------------------------------------------------------------

add_subdirectory(src)

#----------------------------------------------------------------------------------------------------------------------
# Install
#----------------------------------------------------------------------------------------------------------------------

IF(WIN32)
    find_package(Qt6 REQUIRED COMPONENTS Core)
    set(DEPLOY_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/deploy_${PROJECT_NAME}.cmake")
    set(BINARIES "${EXEC_NAME_BUDDY};${EXEC_NAME_STREAM}")

    file(GENERATE OUTPUT ${DEPLOY_SCRIPT} CONTENT "
    include(\"${QT_DEPLOY_SUPPORT}\")
    message(STATUS \"Running Qt deploy tool in working directory '${QT_DEPLOY_PREFIX}'\")

    set(BINARIES \"${BINARIES}\")
    foreach(BINARY \${BINARIES})
        execute_process(
            COMMAND_ECHO STDOUT
            COMMAND \"\${__QT_DEPLOY_TOOL}\" \"\${QT_DEPLOY_BIN_DIR}/\${BINARY}.exe\" --verbose 2 --dir . --libdir bin --plugindir plugins --force --no-compiler-runtime
            WORKING_DIRECTORY \"\${QT_DEPLOY_PREFIX}\"
            RESULT_VARIABLE result
        )
        if(result)
            message(FATAL_ERROR \"Executing \${__QT_DEPLOY_TOOL} failed: \${result}\")
        endif()
    endforeach()

    qt6_deploy_qt_conf(\"\${QT_DEPLOY_PREFIX}/\${QT_DEPLOY_BIN_DIR}/qt.conf\"
        PREFIX \"../\"
        BIN_DIR \"\${QT_DEPLOY_BIN_DIR}\"
        LIB_DIR \"\${QT_DEPLOY_LIB_DIR}\"
        PLUGINS_DIR \"\${QT_DEPLOY_PLUGINS_DIR}\"
        QML_DIR \"\${QT_DEPLOY_QML_DIR}\"
    )
    ")

    foreach(BINARY ${BINARIES})
        install(TARGETS ${BINARY})
    endforeach()
    install(SCRIPT ${DEPLOY_SCRIPT})
    
    include(CPack)
ELSEIF(UNIX)
    install(
        TARGETS ${EXEC_NAME_BUDDY} ${EXEC_NAME_STREAM}
        RUNTIME DESTINATION bin
    )

    install(
        FILES resources/linux/MoonDeckBuddy.desktop
        DESTINATION share/applications/
    )

    foreach(size 16;32;64;128;256)
        install(
            FILES resources/icons/moondeckbuddy-${size}.png
            DESTINATION share/icons/hicolor/${size}x${size}/apps/
            RENAME moondeckbuddy.png
        )
    endforeach()
ELSE()
    message(FATAL_ERROR "OS is not supported!")
ENDIF()
