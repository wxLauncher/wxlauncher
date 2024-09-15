# let's go with lower case for commands (or at least be consistent)
# arbitrarily following http://techbase.kde.org/Policies/CMake_Coding_Style#Upper.2Flower_casing

cmake_minimum_required(VERSION 2.8)
if(CMAKE_MAJOR_VERSION GREATER 2)
  # Continue to support compatiable interface for _DEBUG on targets
  # see: http://www.cmake.org/cmake/help/v3.0/policy/CMP0043.html
  cmake_policy(SET CMP0043 OLD)
endif()

# We don't use C but CMake on linux complains if we don't have it enabled
project(wxlauncher CXX C) 

find_program(GIT_EXECUTABLE git)
if(NOT GIT_EXECUTABLE)
	message(FATAL_ERROR "Unable to locate 'git' commandline tool. "
		"Please:\n"
		" - Install commandline git\n"
		" - Make sure that commandline git is on your path\n"
		" - Call cmake with -DGIT_EXECUTABLE=<full path to git>")
endif()

include(cmake/wxl_platforms.cmake)
# Special source directories
set(LAUNCHER_CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR}/cmake)
include(${LAUNCHER_CMAKE_SOURCE_DIR}/setup_submodules.cmake)
include(${LAUNCHER_CMAKE_SOURCE_DIR}/modules/modules.cmake)
list(INSERT CMAKE_MODULE_PATH 0 "${LAUNCHER_CMAKE_SOURCE_DIR}/include")
set(3RD_PARTY_SOURCE_DIR ${CMAKE_SOURCE_DIR}/vendor)

include(${LAUNCHER_CMAKE_SOURCE_DIR}/GetVersionFromGitTag.cmake)

message("--- Configuring wxLauncher ${wxlauncher_VERSION_STRING_FULL}")

option(USE_3RDPARTY "Use the libraries in the 3rd party directory"
	${USE_3RDPARTY_DEFAULT})
if(USE_3RDPARTY)
	message("--- Using 3rd party source directory")
	execute_process(COMMAND ${CMAKE_COMMAND}
		-P ${3RD_PARTY_SOURCE_DIR}/build_deps.cmake
		WORKING_DIRECTORY ${3RD_PARTY_SOURCE_DIR}
		RESULT_VARIABLE USE_3RDPARTY_BUILD
		)
	if(USE_3RDPARTY_BUILD)
		message(FATAL_ERROR "3rd party lib build failed: ${USE_3RDPARTY_BUILD}")
	endif()
	include(${3RD_PARTY_SOURCE_DIR}/dirs.cmake)
	set(ENV{wxWidgets_ROOT_DIR} ${3RD_PARTY_WXWIDGETS})
	set(ENV{SDL2DIR} ${3RD_PARTY_SDL2})
	set(ENV{OPENALDIR} ${3RD_PARTY_OPENAL})
else()
	message("--- NOT using 3rd party source directory")
endif()

if(DEFINED WXVER AND NOT WXVER MATCHES "PLATFORM")
  # we are given an explict wxWidgets version we must use
  # which means we are probably on CI builder
  message("-- Explicitly told to use WXVER ${WXVER}")
  if(NOT DEFINED wxWidgets_ROOT_DIR OR NOT wxWidgets_ROOT_DIR)
    set(wxWidgets_ROOT_DIR $ENV{WXWIDGETS${WXVER}})
  endif()
  message("--- Searching for wxWidgets in ${wxWidgets_ROOT_DIR}")
  if(WXVER MATCHES "2.8")
    find_package(wxWidgets 2.8.10
      COMPONENTS base core net xml html adv qa richtext)
  elseif(WXVER MATCHES "3.0")
    find_package(wxWidgets 3.0.2
      COMPONENTS base core net xml html adv qa richtext)
  else()
    message(FATAL_ERROR "Unknown WXVER forced")
  endif()

  #ensure we have the correct version with or without stl as requested
  if(NOT DEFINED HAVE_WXUSE_STL 
        OR NOT wxWidgets_ROOT_DIR MATCHES STL_TEST_WX_ROOT)
    # Save for later to test if root changes
    set(STL_TEST_WX_ROOT ${wxWidgets_ROOT_DIR})

    message("-- Testing wxUSE_STL")
    try_compile(HAVE_WXUSE_STL ${CMAKE_BINARY_DIR}
      SOURCES ${CMAKE_SOURCE_DIR}/cmake/test_wxstl.cpp
      COMPILE_DEFINITIONS "/I${wxWidgets_LIB_DIR}/${wxWidgets_CONFIGURATION} /I${wxWidgets_ROOT_DIR}/include")

    if(WXVER MATCHES "stl")
      if(NOT HAVE_WXUSE_STL)
        message(FATAL_ERROR "wxUSE_STL required but not provided")
      endif()
    else()
      if(HAVE_WXUSE_STL)
        message(FATAL_ERROR "wxUSE_STL enabled but was not requested")
      endif()
    endif()
  endif()
else()
  message("-- Searching for wxWidgets")
  find_package(wxWidgets
    COMPONENTS base core net xml html adv qa richtext)
endif()
if(NOT wxWidgets_FOUND)
  message(FATAL_ERROR "Unable to locate wxWidgets")
endif()
  
if(NOT PYTHON_EXECUTABLE)
  # Only try to find python iff not already given
  include(FindPythonInterp) #PYTHON_EXECUTABLE
endif()

option(onlinehelpmaker_debug "Have the onlinehelpmaker.py script output debug information" OFF)
if(onlinehelpmaker_debug)
  set(HELPMAKER_DEBUG "-d")
  set(HELPMAKER_QUIET "")
else(onlinehelpmaker_debug)
  set(HELPMAKER_DEBUG "")
  option(onlinehelpmaker_quiet "Have onlinehelpmaker.py produce even less output than normal" ON)
  if(onlinehelpmaker_quiet)
    set(HELPMAKER_QUIET "-q")
  else()
    set(HELPMAKER_QUIET "")
  endif()
endif(onlinehelpmaker_debug)

# This needs to be set so that VS2008 will link against the most recent
# version of the CRT, the one that is being distrubuted in the installer
if(IS_WIN32)
  add_definitions("/D_BIND_TO_CURRENT_CRT_VERSION=1")
  add_definitions("/D_CRT_SECURE_NO_WARNINGS")
endif(IS_WIN32)

set(helphtblocation ${CMAKE_CURRENT_BINARY_DIR}/generated/onlinehelp.htb)
option(DEVELOPMENT_MODE "Doing development (cannot build installer in this mode)" OFF)
if(DEVELOPMENT_MODE)
  set(HELP_HTB_LOCATION ${helphtblocation})
  set(RESOURCES_PATH ${PROJECT_SOURCE_DIR}/resources)
else(DEVELOPMENT_MODE)
  if (NOT DEFINED RESOURCES_PATH)
    if(IS_WIN32)
      set(RESOURCES_PATH resources)
    elseif(IS_APPLE)
      set(RESOURCES_PATH wxlauncher.app/Contents/Resources)
    else()
      set(RESOURCES_PATH ${CMAKE_INSTALL_PREFIX}/share/wxlauncher/)
    endif()
  endif()
  set(HELP_HTB_LOCATION ${RESOURCES_PATH}/onlinehelp.htb)
endif(DEVELOPMENT_MODE)

option(PROFILE_DEBUGGING "Extra verbose debug logs that include snapshots of profile contents at important steps while auto-save is off" OFF)

if(DEFINED $ENV{OPTIONS} AND $ENV{OPTIONS} STREQUAL "DisableAll")
  set(OPTION_DEFAULT OFF)
else()
  set(OPTION_DEFAULT ON)
endif()

if(IS_WIN32)
  # Don't offer features that can only work on Windows
  option(USE_SPEECH "Build launcher with speech support?" ${OPTION_DEFAULT})
endif()

option(USE_JOYSTICK "Build launcher with joystick support?" ${OPTION_DEFAULT})
option(USE_OPENAL "Build launcher with OpenAL support?" ${OPTION_DEFAULT})
if(USE_OPENAL)
  find_package(OpenAL REQUIRED)
  include_directories(${OPENAL_INCLUDE_DIR})
endif(USE_OPENAL)

find_package(SDL2 REQUIRED)
include_directories(${SDL2_INCLUDE_DIR})
if(IS_APPLE)
    # from https://github.com/scp-fs2open/fs2open.github.com/blob/master/cmake/platform-darwin.cmake
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.9")
    # required to get SDL2.framework to work
    SET(CMAKE_SKIP_RPATH FALSE)
    SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
    SET(CMAKE_INSTALL_RPATH @loader_path/../Frameworks/)
endif()

# Make PLATFORM_USES_REGISTRY true only on windows, so that wxLauncher
# will not try to compile the registry code on non Win32 systems.
# Allow the user to define this as false even on windows if they want
if((IS_WIN32) AND (NOT DEFINED PLATFORM_USES_REGISTRY))
  set(PLATFORM_USES_REGISTRY TRUE)
endif()

include(${wxWidgets_USE_FILE})
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR}/code)

# Check to see if we have a GIT executable so that we will be able to
# generate the version.cpp.  If we don't have a hg binary, notify and
# create the target to copy a template version.
if(GIT_EXECUTABLE)
  add_custom_target(version.cpp.maker ALL
    COMMAND ${PYTHON_EXECUTABLE}
	 scripts/version.cpp.maker.py build
	 ${CMAKE_CURRENT_BINARY_DIR}/generated/version_strings.cpp
	 ${CMAKE_CURRENT_BINARY_DIR}/version.cpp.maker.temp
	 --gitpath=${GIT_EXECUTABLE}
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    )
else()
  add_custom_target(version.cpp.maker ALL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
	${CMAKE_SOURCE_DIR}/code/global/version_strings.cpp.in
	${CMAKE_CURRENT_BINARY_DIR}/generated/version_strings.cpp
    )
  message( "Cannot find a commandline Mercurial client (hg). "
    "version_strings.cpp will be generated by copying a dummy file.")
endif()

set_property(DIRECTORY APPEND PROPERTY ADDITIONAL_MAKE_CLEAN_FILES
	${CMAKE_CURRENT_BINARY_DIR}/generated/version_strings.cpp
	${CMAKE_CURRENT_BINARY_DIR}/version.cpp.maker.temp
	)

add_custom_target(helpmaker ALL
  COMMAND ${PYTHON_EXECUTABLE} scripts/onlinehelpmaker.py ${HELPMAKER_QUIET} ${HELPMAKER_DEBUG} build  ${helphtblocation} ${CMAKE_SOURCE_DIR}/onlinehelp -t ${CMAKE_CURRENT_BINARY_DIR}/onlinehelpmaker -c ${CMAKE_CURRENT_BINARY_DIR}/generated/helplinks.cpp
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  )
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${CMAKE_CURRENT_BINARY_DIR}/generated/helplinks.cpp)
set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${helphtblocation})
file(GLOB_RECURSE helpmaker_temp_files ${CMAKE_CURRENT_BINARY_DIR}/onlinehelpmaker/*.*)

# Call out the files that are built a part of the build
# The HTBs do not need to be run through a complier
set_source_files_properties(
	${helphtblocation}
	PROPERTIES GENERATED true EXTERNAL_OBJECT true)
set_source_files_properties(
	${CMAKE_CURRENT_BINARY_DIR}/generated/helplinks.cpp
	${CMAKE_CURRENT_BINARY_DIR}/generated/version_strings.cpp
	PROPERTIES GENERATED true)

set(TAB_CODE_FILES
  code/tabs/AdvSettingsPage.h
  code/tabs/AdvSettingsPage.cpp
  code/tabs/BasicSettingsPage.h
  code/tabs/BasicSettingsPage.cpp
  code/tabs/InstallPage.h
  code/tabs/InstallPage.cpp
  code/tabs/ModsPage.h
  code/tabs/ModsPage.cpp
  code/tabs/WelcomePage.h
  code/tabs/WelcomePage.cpp
  )
source_group(Tabs FILES ${TAB_CODE_FILES})
set(GUI_CONTROL_CODE_FILES
  code/controls/BottomButtons.h
  code/controls/BottomButtons.cpp
  code/controls/FlagListBox.h
  code/controls/FlagListBox.cpp
  code/controls/LightingPresets.h
  code/controls/LightingPresets.cpp
  code/controls/Logger.h
  code/controls/Logger.cpp
  code/controls/ModList.h
  code/controls/ModList.cpp
  code/controls/StatusBar.h
  code/controls/StatusBar.cpp
  code/controls/TruncatableChoice.h
  code/controls/TruncatableChoice.cpp
  )
source_group("GUI Controls" FILES ${GUI_CONTROL_CODE_FILES})
set(GLOBAL_CODE_FILES
  code/global/configure_launcher.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/configure_launcher.h
  code/global/BasicDefaults.h
  code/global/BasicDefaults.cpp
  code/global/ids.h
  code/global/MemoryDebugging.h
  code/global/ModDefaults.h
  code/global/ModDefaults.cpp
  code/global/ModIniKeys.h
  code/global/ModIniKeys.cpp
  code/global/ProfileKeys.h
  code/global/ProfileKeys.cpp
  code/global/RegistryKeys.h
  code/global/RegistryKeys.cpp
  code/global/SkinDefaults.h
  code/global/SkinDefaults.cpp
  code/global/targetver.h
  code/global/Utils.h
  code/global/Utils.cpp
  code/global/version.h
  code/global/version.cpp
  code/global/Compatibility.cpp
  code/global/Compatibility.h)
source_group(Global FILES ${GLOBAL_CODE_FILES})
set(DATASTRUCTURE_CODE_FILES
  code/datastructures/FlagInfo.cpp
  code/datastructures/FlagFileData.h
  code/datastructures/FlagFileData.cpp
  code/datastructures/FSOExecutable.h
  code/datastructures/FSOExecutable.cpp
  code/datastructures/NewsSource.h
  code/datastructures/NewsSource.cpp
  code/datastructures/ResolutionMap.h
  code/datastructures/ResolutionMap.cpp
  )
source_group("Data Structures" FILES ${DATASTRUCTURE_CODE_FILES})
set(API_CODE_FILES
  code/apis/CmdLineManager.h
  code/apis/CmdLineManager.cpp
  code/apis/EventHandlers.h
  code/apis/EventHandlers.cpp
  code/apis/FlagListManager.h
  code/apis/FlagListManager.cpp
  code/apis/FREDManager.h
  code/apis/FREDManager.cpp
  code/apis/HelpManager.h
  code/apis/HelpManager.cpp
  code/apis/JoystickManager.h
  code/apis/JoystickManager.cpp
  code/apis/OpenALManager.h
  code/apis/OpenALManager.cpp
  code/apis/ProfileManager.h
  code/apis/ProfileManagerOperator.h
  code/apis/ProfileManager.cpp
  code/apis/ProfileManagerOperator.cpp
  code/apis/ProfileProxy.h
  code/apis/ProfileProxy.cpp
  code/apis/resolution_manager.hpp
  code/apis/resolution_manager.cpp
  code/apis/SkinManager.h
  code/apis/SkinManager.cpp
  code/apis/SpeechManager.h
  code/apis/SpeechManager.cpp
  code/apis/TCManager.h
  code/apis/TCManager.cpp
  code/apis/PlatformProfileManager.h
  code/apis/RegistryProfileManager.cpp
  code/apis/FileProfileManager.cpp
  code/apis/PlatformProfileManagerShared.cpp
  )
source_group(Apis FILES ${API_CODE_FILES})
set(RESOURCE_FILES
  platform/win32/wxlauncher.rc
  ${CMAKE_CURRENT_BINARY_DIR}/generated/version_strings.cpp
  ${CMAKE_CURRENT_BINARY_DIR}/generated/helplinks.cpp
  )
source_group(Resources FILES ${RESOURCE_FILES})
set(CODE_FILES
  code/MainWindow.h
  code/MainWindow.cpp
  code/wxLauncherApp.h
  code/wxLauncherApp.cpp
  )
source_group("Main Code Files" FILES ${CODE_FILES})

add_executable(wxlauncher WIN32 MACOSX_BUNDLE
  ${TAB_CODE_FILES}
  ${GUI_CONTROL_CODE_FILES}
  ${GLOBAL_CODE_FILES}
  ${DATASTRUCTURE_CODE_FILES}
  ${API_CODE_FILES}
  ${RESOURCE_FILES}
  ${CODE_FILES}
  )
# C++11 is required for wxWidgets 3.0 and wxLauncher's new code
# (the auto_type specifically)
if (COMMAND target_compile_features)
	# target_compile_features added in CMake 3.1
	target_compile_features(wxlauncher PRIVATE cxx_auto_type)
else()
	set_property(TARGET wxlauncher APPEND_STRING
		PROPERTY COMPILE_FLAGS "-std=c++11")
endif()
  
set_target_properties(wxlauncher
  PROPERTIES LINKER_LANGUAGE CXX
  POSITION_INDEPENDENT_CODE ON)
# Files that are not to be compiled directly
set_source_files_properties(
  ${CMAKE_CURRENT_BINARY_DIR}/generated/helplinks.cpp   
  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/onlinehelp.htb 
  ${CMAKE_CURRENT_BINARY_DIR}/generated/configure_launcher.h 
  code/datastructures/FlagInfo.cpp
  code/global/configure_launcher.h.in
  PROPERTIES HEADER_FILE_ONLY true)

add_dependencies(wxlauncher helpmaker version.cpp.maker)

configure_file(${CMAKE_SOURCE_DIR}/code/global/configure_launcher.h.in
  ${CMAKE_CURRENT_BINARY_DIR}/generated/configure_launcher.h)

foreach(temp_file ${helpmaker_temp_files})
  set_directory_properties(PROPERTIES ADDITIONAL_MAKE_CLEAN_FILES ${temp_file})
endforeach(temp_file)

target_link_libraries(wxlauncher ${wxWidgets_LIBRARIES} ${SDL2_LIBRARY})

# adapted from http://www.cmake.org/Wiki/CMake_FAQ#How_can_I_apply_resources_on_Mac_OS_X_automatically.3F
# copies necessary resources (and frameworks, if needed) to .app bundle
if(IS_APPLE)
  set(APP_RESOURCES_PATH ${CMAKE_CURRENT_BINARY_DIR}/$(CONFIGURATION)/${RESOURCES_PATH})
  set(APP_FRAMEWORKS_PATH ${CMAKE_CURRENT_BINARY_DIR}/$(CONFIGURATION)/wxlauncher.app/Contents/Frameworks)
  add_custom_command(TARGET wxlauncher POST_BUILD
    COMMAND rm -rf ${APP_RESOURCES_PATH}
    COMMAND rm -rf ${APP_FRAMEWORKS_PATH}
    COMMAND mkdir ${APP_RESOURCES_PATH}
    COMMAND mkdir ${APP_FRAMEWORKS_PATH}
    COMMAND cp ${PROJECT_SOURCE_DIR}/resources/* ${APP_RESOURCES_PATH}
    COMMAND cp ${helphtblocation} ${APP_RESOURCES_PATH}
    COMMAND cp ${PROJECT_SOURCE_DIR}/platform/macosx/wxlauncher.icns ${APP_RESOURCES_PATH})
  if(USING_SDL2_FRAMEWORK) # then copy the framework into the app
    # using -a because of https://github.com/scp-fs2open/fs2open.github.com/pull/955/commits/5bf9db07a83f42c49344596216a1d667edf31f56
    add_custom_command(TARGET wxlauncher POST_BUILD
      COMMAND cp -a ${SDL2_FRAMEWORK} ${APP_FRAMEWORKS_PATH})
  endif(USING_SDL2_FRAMEWORK)
endif(IS_APPLE)

# packaging
if(DEVELOPMENT_MODE)
  message( "Development mode is set. Building of installers is not allowed.  Set DEVELOPMENT_MODE=OFF to build installers.")
  configure_file(${CMAKE_SOURCE_DIR}/cmake/FailCPack.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CPackConfig.cmake COPYONLY)
  configure_file(${CMAKE_SOURCE_DIR}/cmake/FailCPack.cmake.in ${CMAKE_CURRENT_BINARY_DIR}/CPackSourceConfig.cmake COPYONLY)

  if(SDL2_DLL)
    ADD_CUSTOM_COMMAND(
        TARGET wxlauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${SDL2_DLL}"  "$<TARGET_FILE_DIR:wxlauncher>"
        COMMENT "copying '${SDL2_DLL}'..."
    )
  endif()
else()
  include(${LAUNCHER_CMAKE_SOURCE_DIR}/wxLauncherInstaller.cmake)
endif()
