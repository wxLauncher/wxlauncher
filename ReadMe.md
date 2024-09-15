# wxLauncher: Multi-platform launcher for Freespace 2

Introduction
============
[wxLauncher] aims to give one unified answer to members of
the Freespace 2 Open community looking for an easy way
to control `fs2_open` on various platforms and to those
looking for an easy way to find and get updates to their
Freespace 2 MODs and TCs.

[wxLauncher]: https://github.com/wxLauncher/wxlauncher

Compatibility
=============
wxLauncher is built for the 3.6.9+ versions of `fs2_open`.
While older versions might function, we don't officially
support them. Use them at your own risk.

Obtaining wxLauncher
====================
Precompiled binaries can be found on the project
[releases page](https://github.com/wxLauncher/wxlauncher/releases)

Find the build for your system and you're set.

Building from source
====================
The wxLauncher requires the CMake build system for building.
CMake is a cross-platform meta-build system; it generates the
files that allow a platform's native build system to build the
launcher.  CMake supports VS2005 and newer, as well as
Xcode, KDevelop, and of course autotools.

### Requirements shortlist

  * All platforms:
      * CMake 2.8
      * wxWidgets 2.8.10+ or 3.0.2
      * Python 2.7 or Python 3.4+
      * markdown for Python
  * Windows
      * Windows SDK or Platform SDK
      * Nullsoft Scriptable Install System (NSIS)
  * Linux/macOS/FreeBSD
      * SDL 2

### Optional components
  * All platforms:
      * OpenAL or OpenALSoft

### Detailed requirements

wxLauncher is built using CMake.  Only version 2.8 or later has
been tested (and the CMake file enforces this).  CMake can be
downloaded in binary form, from the [CMake Home Page] or if you
run Linux, from your distro's package repository.
[Cmake Home Page]: http://cmake.org

wxLauncher is a [wxWidgets]-based application. It can
only be built with wxWidgets version 2.8.10 or higher.
Version 0.10.0 has been tested with:
  - 2.8.12 (no STL)
  - 2.8.12 (STL)
  - 3.0.2 (no STL)
  - 3.0.2 (STL)
[wxWidgets]: http://www.wxwidgets.org/

[Python] 2.7 or higher is required to build this project.
This project also assumes that the python executable is
in your PATH.  Check your operating system's documentation
for information on how to add Python to your PATH.
[Python]: http://www.python.org/

Markdown in Python is required in order to build the
integrated help system.
  * [Markdown in Python homepage](https://pypi.python.org/pypi/Markdown)
  * On `pip` enabled installs, try: `pip install markdown` but for Python
3, use `pip3` instead of `pip`.
  * On Debian-based systems, try: `apt-get install python-markdown`

The OpenAL Software Development Kit is an optional component
needed to build this program.  It can be downloaded from the
[OpenAL homepage]. OpenAL support requires the
preprocessor symbol `USE_OPENAL=1`.  This symbol is set to 1 by
default by CMake.  You can pass `-DUSE_OPENAL=0` to CMake to
disable building with OpenAL support.  See the compiler specific
instructions for getting your compiler ready to build
with OpenAL. Note that macOS ships with OpenAL pre-installed.
wxLauncher should also work with the [OpenALSoft] library.
[OpenAL homepage]: http://connect.creativelabs.com/openal/default.aspx
[OpenALSoft]: http://kcat.strangesoft.net/openal.html

The Microsoft Windows SDK (formerly the Platform SDK) is
required to build both this application and wxWidgets when
 on Windows.  Note that only the Windows SDK for Windows
Vista and Windows 7 have been tested with this application.

The Nullsoft Scriptable Install System (NSIS) is required on
Windows to build the installer. The latest version at the time
of writing is 2.46. Be sure to either install NSIS before
running CMake or re-run CMake after installing NSIS. NSIS can
be found on the [NSIS homepage](http://nsis.sourceforge.net/).

The offical wxLauncher releases for Windows are built with
Microsoft's Visual Studio 2008 Express edition. Other
Visual Studio versions should work, but are not routinely
tested. [Visual Studio Express] editions can be downloaded
and used for developing wxLauncher for free.
[Visual Studio Express]: http://www.microsoft.com/express/download/

wxLauncher's source can be explored from the project's
[source page](https://github.com/wxLauncher/wxlauncher/)

To get the source, you'll need Git:
  * [Command Line Git](http://git-scm.com/)
  * Graphical frontend: [TortioseGit](http://tortiosgit.org)
  * Distro package manager. Sometimes called `git-scm`
  * The Homebrew Project for macOS

Once Git is installed, you can get a copy of the source
by running the following command in a folder of your choice:
	`git clone https://code.google.com/p/wxlauncher/`

Building - Windows
==================
Run CMake in your favourite way (GUI, or on the commandline
ccmake (uses curses) or cmake).

CMake QT Gui
------------
Assuming the GUI, select the CMakeLists.txt in the main
wxLauncher source directory and set your output directory
to where you want the native build tool to be placed,
somewhere without spaces. Click configure until the Generate
button Enables.  The lines that are highligted red are new
variables that CMake has found.
- Set wxWdigets_ROOT_DIR to the root directory of your wxWidgets
source directory if it remains NOTFOUND.
- Set PYTHON_EXECUTABLE to the python that you want to use. It may not show
up. If it doesn't show up, it means that cmake found python automatically,
but if cmake selects the wrong version of python, please see "CMake selects the
wrong python".
- Check DEVELOPMENT_MODE if you are going to be debugging wxLauncher. Make sure
that it is unchecked if you plan on distributing the code as with this checked,
the launcher will only run on the dev machine. This option changes where the
launcher looks for files to display such as the default interface.
- Check USE_JOYSTICK, USE_OPENAL, and/or USE_SPEECH if you want those options compiled in.
- If USE_OPENAL is checked, OPENAL_INCLUDE_DIR will appear. Set it to the include
folder in the OpenAL SDK folder.

Building - Linux (has been tested on Debian Jessie)
=========================
Command line
-----------
- sudo apt-get install build-essential libopenal-dev libwxgtk3.0-dev
libwxgtk3.0-0-dbg python-markdown git cmake libsdl2-dbg libsdl2-dev
- Download the wxLauncher source
- cd <source directory>
- mkdir build
- cd build
To prevent error messages like "The launcher is expecting (/home/foo/bar) to
contain the resource images." from occurring, either type:
- cmake -DUSE_OPENAL=1 -DDEVELOPMENT_MODE=1 -DCMAKE_INSTALL_PREFIX=/usr/local ../
- make
Or instead type:
- cmake -DUSE_OPENAL=1 -DCMAKE_INSTALL_PREFIX=/usr/local ../
- make
- make install (with root privileges)

CMake selects the wrong python
------------------------------
If CMake selects the wrong version of python, you
can set the version yourself by setting PYTHON_EXECUTABLE when you run CMake by
adding "-DPYTHON_EXECUTABLE=</path/to/python2>" to your command line, for
example "-DPYTHON_EXECUTABLE=/usr/bin/python2".

If you run into this problem, please post in this ticket
<http://code.google.com/p/wxlauncher/issues/detail?id=99> with your distro,
OS version, and whether you're running 32- or 64-bit.

Building - macOS (tested on 10.11, El Capitan)
=============================================
- Get Xcode from the Mac App Store (used 7.3.1)
- Get Python 3 from python.org if you don't have it. (used 3.5.2)
- Get Markdown for Python (link is provided above). (used 2.6.6)
Use `pip3` to install Markdown rather than `pip`. By default this is in
/usr/local/bin/pip3 . You will need root privileges.
- Get Git, making sure that you select the version of
  Git for your version of macOS.
- Clone the wxLauncher repository.
- Get CMake 3 (used 3.6.1).
- Get the latest stable version of wxWidgets 3 (used 3.0.2). Once you've
downloaded and extracted the source tarball, do these things, assuming 3.0.2:
    * cd wxWidgets-3.0.2/
    * Either "mkdir build-debug" or "mkdir build-release" (your choice)
    * cd <TheBuildFolderYouJustMade>
    * Type the following to configure, adjusting according to the notes
    that follow:
    * ../configure --enable-stl --enable-unicode --enable-debug
    --disable-shared --with-macosx-version-min=10.9 CC=clang CXX=clang++
    CXXFLAGS="-stdlib=libc++ -std=c++11"
    OBJCXXFLAGS="-stdlib=libc++ -std=c++11" LDFLAGS=-stdlib=libc++
        - If you want a release build rather than a debug build, leave out
       the '--enable-debug'
    * make
- Install SDL2: you can either (1) download, build, and install it
yourself (such as with Homebrew) or (2) simply download the
[Frameworks.tgz tarbal](https://github.com/scp-fs2open/fs2open.github.com/blob/master/lib/mac/Frameworks.tgz)
from the FreeSpace 2 Open source tree

Extract the SDL2.framework and copy it to your
/Users/<YourUsername>/Library/Frameworks folder. Create the Frameworks
folder if it does not exist.
- Run CMake either by using cmake at the command line or by using the
CMake.app GUI, selecting Xcode as your generator.
    - A few notes on configuring the CMake variables:
        * Set wxWidgets_CONFIG_EXECUTABLE to point to the version of
        wxWidgets you built. wxWidgets_CONFIG_EXECUTABLE is located at
        /yourWxWidgetsBuildFolder/wx-config
        * Set PYTHON_EXECUTABLE to point to Python 3, by default in
        /usr/local/bin/python3
        * If you are not using SDL2.framework, uncheck
        USING_SDL2_FRAMEWORK (or on command line,
        add -DUSING_SDL2_FRAMEWORK=0)
    * cd <wxLauncherSourceFolder>
    * mkdir build
    * cd build
    * /path/to/CMake.app/Contents/bin/cmake -G Xcode
    -DwxWidgets_CONFIG_EXECUTABLE=/path/to/wxWidgets/Build/Folder/wx-config
    -DPYTHON_EXECUTABLE=/path/to/python3 -DUSE_OPENAL=1 ../
- Once you have your Xcode project set up, build the "ALL_BUILD" target to
build wxlauncher.app in /wxLauncherBuildFolder/SelectedBuildConfig/ , or
type "xcodebuild -configuration <SelectedBuildConfig>" at the terminal in
your wxLauncher build folder. Make sure that the build configuration you
choose (Debug or Release) matches the build configuration you used when
you built wxWidgets.

Important known issues on macOS:
- After startup or after a FS2 Open binary is (re-)selected, checkboxes on
the advanced settings page may not appear until after a moment or after
the user interacts with the advanced settings page, such as by clicking on
the flag list.

Building - FreeBSD (tested on TrueOS Desktop, 22 Feb 2017)
=============================================
- Download the wxLauncher source
- Ensure the following packages are installed:
- cmake, openal-soft, SDL2, wx28-gtk2, python, python2, python27, py27-markdown
- cd <source directory>
- mkdir build
- cd build
- A typical fully usable build configuration will look something like:
- cmake -DCMAKE_INSTALL_PREFIX=/usr/local -DUSE_OPENAL=1 -DUSE_JOYSTICK=1
- -DwxWidgets_CONFIG_EXECUTABLE=/usr/local/bin/wxgtk2u-2.8-config ../
- make
- make install (with root privileges if necessary)

Notes
-----
Using wx30-gtk2 currently yields a compilation error (11 March 2017)
TrueOS desktop comes with python2/python27, but no binary 'python'.
Installing the python package seems to just create that symlink.
Joystick support has not been tested.  If you have a joystick working
with FreeBSD please report test results.

License
=======
See License.txt.
