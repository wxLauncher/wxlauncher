# Note: assuming that this file is included in CMakeLists.txt, so
#       using IS_WIN32, IS_APPLE, IS_LINUX
# Setup the packer

if(IS_WIN32) # and WIN64
  # find the runtime files
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    # Determine the name of the pdb on the first run of CMake.
    # From Cmake mailing list:
    # http://www.cmake.org/pipermail/cmake/2007-October/016924.html
    # First, build the full name of the EXE.
    install(CODE "set(wxLauncher_pdb ${CMAKE_CURRENT_BINARY_DIR}/\${${project}_BUILD_NAME_\${CMAKE_INSTALL_CONFIG_NAME}})")
    # Then, replace .exe with .pdb.
    install(CODE "string(REPLACE .exe .pdb wxLauncher_pdb \${wxLauncher_pdb})")
    # Finally, get it installed.
    install(CODE "install(FILES ${wxLauncher_pdb} DESTINATION bin)")
    set(CMAKE_INSTALL_DEBUG_LIBRARIES TRUE)
  endif()
  include(InstallRequiredSystemLibraries)
endif() # IS_WIN32
  
set(CPACK_PACKAGE_VERSION_MAJOR ${wxlauncher_VERSION_MAJOR})
set(CPACK_PACKAGE_VERSION_MINOR ${wxlauncher_VERSION_MINOR})
set(CPACK_PACKAGE_VERSION_PATCH ${wxlauncher_VERSION_PATCH})
set(CPACK_PACKAGE_NAME "wxLauncher")
set(CPACK_PACKAGE_VENDOR "wxLauncher Team")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "wxLauncher")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Launcher and installer for the FreeSpace SCP")
set(CPACK_PACKAGE_DESCRIPTION "Launcher and installer for the FreeSpace 2 Open Source Code Project")

# TODO we need separate welcome and readme files,
# and a readme better geared towards end users

if(NOT IS_APPLE) # a license agreement popping up whenever people open the DMG will drive them crazy
  set(CPACK_RESOURCE_FILE_LICENSE ${PROJECT_SOURCE_DIR}/License.txt)
endif()

set(CPACK_RESOURCE_FILE_README ${PROJECT_SOURCE_DIR}/ReadMe.md)
set(CPACK_RESOURCE_FILE_WELCOME ${PROJECT_SOURCE_DIR}/ReadMe.md)

if(IS_WIN32)
  set(CPACK_PACKAGE_INSTALL_REGISTRY_KEY wxLauncher)
  # the \\\\ is because NSIS does not handle unix paths correctly - http://www.batchmake.org/Wiki/CMake:Packaging_With_CPack
  set(CPACK_NSIS_MUI_ICON ${PROJECT_SOURCE_DIR}/platform/win32\\\\wxlauncher.ico)
  set(CPACK_NSIS_MUI_UNIICON ${PROJECT_SOURCE_DIR}/platform/win32\\\\wxlauncher.ico)
  # the header image *must* be a bitmap
  set(CPACK_PACKAGE_ICON ${PROJECT_SOURCE_DIR}/platform/win32\\\\installer_header.bmp)
  set(CPACK_NSIS_HELP_LINK http::\\\\\\\\wxlauncher.googlecode.com)
  set(CPACK_NSIS_URL_INFO_ABOUT http::\\\\\\\\www.hard-light.net)
endif()

if(DEVELOPMENT_MODE)
  set(CPACK_PACKAGE_FILE_NAME wxlaucher-development-mode-do-not-distribute)
else()
  if(CMAKE_BUILD_TYPE MATCHES Debug)
    set(CPACK_PACKAGE_FILE_NAME wxlauncher-${wxlauncher_VERSION_STRING}-debug-withpdbs)
  elseif(CMAKE_BUILD_TYPE MATCHES RelWithDebInfo)
    set(CPACK_PACKAGE_FILE_NAME wxlauncher-${wxlauncher_VERSION_STRING})
  else()
    set(CPACK_PACKAGE_FILE_NAME wxlauncher-${wxlauncher_VERSION_STRING})
  endif()
endif()

if( IS_WIN32 AND NOT UNIX )
    set(CPACK_PACKAGE_EXECUTABLES "wxLauncher" "wxLauncher for the SCP")
endif()

if(IS_APPLE)
# currently can't get bundle name and long version string to display, grr
#  set(MACOSX_BUNDLE_BUNDLE_NAME "wxLauncher")
#  set(MACOSX_BUNDLE_LONG_VERSION_STRING "wxLauncher for the SCP, version ${MACOSX_BUNDLE_SHORT_VERSION_STRING}")
  set(MACOSX_BUNDLE_ICON_FILE wxlauncher.icns)
  set(MACOSX_BUNDLE_SHORT_VERSION_STRING ${wxlauncher_VERSION})
  set(MACOSX_BUNDLE_COPYRIGHT "Copyright © 2009-2016 ${CPACK_PACKAGE_VENDOR}")
endif(IS_APPLE)

if(IS_LINUX)
  set(CPACK_DEBIAN_PACKAGE_MAINTAINER "Iss Mneur <iss.mneur@issmneur.ca>")
  set(CPACK_PACKAGE_DEPENDS "libwxgtk2.8 libopenal1")
  set(CPACK_DEBIAN_PACKAGE_SECTION "Games")
endif()

if(IS_WIN32)
  set(CPACK_BINARY_NSIS "ON")
  option(BUILD_BINARY_ZIP "Generate a binary zip" "OFF")
  if(BUILD_BINARY_ZIP)
    set(CPACK_BINARY_ZIP "ON")
  else()
    set(CPACK_BINARY_ZIP "OFF")
  endif()
elseif(IS_APPLE)
  set(CPACK_BINARY_DRAGNDROP "ON")
  set(CPACK_BINARY_PACKAGEMAKER "OFF")
  set(CPACK_BINARY_STGZ "OFF")
  set(CPACK_BINARY_TZ "OFF")
  set(CPACK_BINARY_TGZ "OFF")
else()
  set(CPACK_BINARY_DEB "ON")
  find_program(rpmbuild_exists rpmbuild)
  if(rpmbuild_exists)
    set(CPACK_BINARY_RPM "ON")
  else()
    set(CPACK_BINARY_RPM "OFF")
    message(STATUS "rpmbuild does not exist, will not build RPM")    
  endif()
  set(CPACK_BINARY_STGZ "OFF")
  set(CPACK_BINARY_TZ "OFF")
  set(CPACK_BINARY_TGZ "OFF")
endif()

include(CPack)

if(IS_WIN32)
  install(TARGETS wxlauncher RUNTIME DESTINATION bin)
  install(FILES "${3RD_PARTY_SDL2}/bin/SDL2.dll" DESTINATION bin)
elseif(IS_APPLE)
  install(TARGETS wxlauncher 
	RUNTIME DESTINATION .
	BUNDLE DESTINATION .)
else()  
  install(TARGETS wxlauncher 
	RUNTIME DESTINATION bin)
endif()

# prototype for post-processing code that will be needed later
#   to automatically generate .dmg that doesn't have /Applications symlink
#if(IS_APPLE)
#  set(DRAGNDROP_PATH ${CMAKE_CURRENT_BINARY_DIR}/_CPack_Packages/Darwin/DragNDrop)
#  set(DMG_FOLDER_PATH ${DRAGNDROP_PATH}/${PROJECT_NAME}-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH})
#  message(${DRAGNDROP_PATH})
#  message(${DMG_FOLDER_PATH})
#  TODO the current major problem is getting the command to run at the right time
#  add_custom_command(TARGET package POST_BUILD
#    COMMAND echo post-build test
#    # TODO delete auto-generated .dmg, delete /Applications symlink, re-generate .dmg
#    )
#endif(IS_APPLE)

if(IS_WIN32)
  install(DIRECTORY resources/ DESTINATION resources)
  install(FILES ${helphtblocation} DESTINATION resources)
elseif(IS_APPLE)
  install(FILES ${PROJECT_SOURCE_DIR}/License.txt DESTINATION .)
  install(FILES ${PROJECT_SOURCE_DIR}/GPLv2.txt DESTINATION .)
#  add_custom_target(RemoveAppsLink ALL ${PROJECT_SOURCE_DIR}/platform/macosx/removeAppsLink.sh "${CMAKE_CURRENT_BINARY_DIR}/_CPack_Packages/Darwin/DragNDrop/wxlauncher-${CPACK_PACKAGE_VERSION_MAJOR}.${CPACK_PACKAGE_VERSION_MINOR}.${CPACK_PACKAGE_VERSION_PATCH}" VERBATIM) # attempt to remove link to Applications folder, will keep this as placeholder until I come up with a better idea -- maybe a postinstall script to delete the generated DMG, remove the Applications link, then create a new DMG with hdiutil? 
else()
  install(DIRECTORY resources/ DESTINATION ${RESOURCES_PATH})
  install(FILES ${helphtblocation} DESTINATION ${RESOURCES_PATH})
  if (${CMAKE_VERSION} VERSION_EQUAL 3.0 OR ${CMAKE_VERSION} VERSION_GREATER 3.0)
    # Assume a freedesktop environment
    set(WXLAUNCHER_FILE ${CMAKE_INSTALL_PREFIX}/bin/$<TARGET_FILE_NAME:wxlauncher>)
    configure_file("${PROJECT_SOURCE_DIR}/platform/freedesktop/wxLauncher.desktop.in" "${CMAKE_BINARY_DIR}/platform/freedesktop/wxlauncher.desktop")

    file(GENERATE OUTPUT "${CMAKE_BINARY_DIR}/platform/freedesktop/wxlauncher.desktop-$<CONFIG>"
            INPUT "${CMAKE_BINARY_DIR}/platform/freedesktop/wxlauncher.desktop")

    install(FILES "${CMAKE_BINARY_DIR}/platform/freedesktop/wxlauncher.desktop-$<CONFIG>"
            DESTINATION share/applications
            RENAME wxlauncher.desktop)
    install(FILES resources/wxlauncher.png
            DESTINATION share/icons/hicolor/256x256/apps)
  else()
    message(STATUS "The CMake version is too old to support desktop file generation. CMake 3.0 or newer required")
  endif()
endif()
