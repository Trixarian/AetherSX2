### Select the build type
# Use Release/Devel/Debug      : -DCMAKE_BUILD_TYPE=Release|Devel|Debug
# Enable/disable the stripping : -DCMAKE_BUILD_STRIP=TRUE|FALSE
# generation .po based on src  : -DCMAKE_BUILD_PO=TRUE|FALSE

### GCC optimization options
# control C flags             : -DUSER_CMAKE_C_FLAGS="cflags"
# control C++ flags           : -DUSER_CMAKE_CXX_FLAGS="cxxflags"
# control link flags          : -DUSER_CMAKE_LD_FLAGS="ldflags"
#-------------------------------------------------------------------------------

# Extra preprocessor definitions that will be added to all pcsx2 builds
set(PCSX2_DEFS "")

#-------------------------------------------------------------------------------
# Misc option
#-------------------------------------------------------------------------------
option(DISABLE_BUILD_DATE "Disable including the binary compile date")
option(ENABLE_TESTS "Enables building the unit tests" ON)
option(USE_SYSTEM_YAML "Uses a system version of yaml, if found")
option(LTO_PCSX2_CORE "Enable LTO/IPO/LTCG on the subset of pcsx2 that benefits most from it but not anything else")

if(WIN32)
	set(DEFAULT_NATIVE_TOOLS ON)
else()
	set(DEFAULT_NATIVE_TOOLS OFF)
endif()
option(USE_NATIVE_TOOLS "Uses c++ tools instead of ones written in scripting languages.  OFF requires perl, ON may fail if cross compiling" ${DEFAULT_NATIVE_TOOLS})

if(DISABLE_BUILD_DATE OR openSUSE)
	message(STATUS "Disabling the inclusion of the binary compile date.")
	list(APPEND PCSX2_DEFS DISABLE_BUILD_DATE)
endif()

option(USE_VTUNE "Plug VTUNE to profile GS JIT.")

#-------------------------------------------------------------------------------
# Graphical option
#-------------------------------------------------------------------------------
option(BUILD_REPLAY_LOADERS "Build GS replayer to ease testing (developer option)")

#-------------------------------------------------------------------------------
# Path and lib option
#-------------------------------------------------------------------------------
option(PACKAGE_MODE "Use this option to ease packaging of PCSX2 (developer/distribution option)")
option(DISABLE_CHEATS_ZIP "Disable including the cheats_ws.zip file")
option(DISABLE_PCSX2_WRAPPER "Disable including the PCSX2-linux.sh file")
option(DISABLE_SETCAP "Do not set files capabilities")
option(XDG_STD "Use XDG standard path instead of the standard PCSX2 path")
option(PORTAUDIO_API "Build portaudio support on SPU2" ON)
option(CUBEB_API "Build Cubeb support on SPU2" ON)
option(SDL2_API "Use SDL2 on SPU2 and PAD Linux (wxWidget mustn't be built with SDL1.2 support" ON)
option(GTK2_API "Use GTK2 api (legacy)")

if(UNIX AND NOT APPLE AND NOT ANDROID)
	option(X11_API "Enable X11 support" ON)
	option(WAYLAND_API "Enable Wayland support" OFF)
endif()

if(PACKAGE_MODE)
	# Compile all source codes with those defines
	list(APPEND PCSX2_DEFS
		GAMEINDEX_DIR_COMPILATION=${CMAKE_INSTALL_FULL_DATADIR}/PCSX2
		DOC_DIR_COMPILATION=${CMAKE_INSTALL_FULL_DOCDIR})
endif()

if(APPLE)
	option(OSX_USE_DEFAULT_SEARCH_PATH "Don't prioritize system library paths" OFF)
	option(SKIP_POSTPROCESS_BUNDLE "Skip postprocessing bundle for redistributability" OFF)
endif()

#-------------------------------------------------------------------------------
# Compiler extra
#-------------------------------------------------------------------------------
option(USE_ASAN "Enable address sanitizer")

if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
	set(USE_CLANG TRUE)
	message(STATUS "Building with Clang/LLVM.")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
	set(USE_ICC TRUE)
	message(STATUS "Building with Intel's ICC.")
elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
	set(USE_GCC TRUE)
	message(STATUS "Building with GNU GCC")
elseif(MSVC)
	message(STATUS "Building with MSVC")
else()
	message(FATAL_ERROR "Unknown compiler: ${CMAKE_CXX_COMPILER_ID}")
endif()

#-------------------------------------------------------------------------------
# if no build type is set, use Devel as default
# Note without the CMAKE_BUILD_TYPE options the value is still defined to ""
# Ensure that the value set by the User is correct to avoid some bad behavior later
#-------------------------------------------------------------------------------
if(NOT CMAKE_BUILD_TYPE MATCHES "Debug|Devel|MinSizeRel|RelWithDebInfo|Release")
	set(CMAKE_BUILD_TYPE Devel)
	message(STATUS "BuildType set to ${CMAKE_BUILD_TYPE} by default")
endif()
# Add Devel build type
set(CMAKE_C_FLAGS_DEVEL "${CMAKE_C_FLAGS_RELWITHDEBINFO}"
	CACHE STRING "Flags used by the C compiler during development builds" FORCE)
set(CMAKE_CXX_FLAGS_DEVEL "${CMAKE_CXX_FLAGS_RELWITHDEBINFO}"
	CACHE STRING "Flags used by the C++ compiler during development builds" FORCE)
set(CMAKE_LINKER_FLAGS_DEVEL "${CMAKE_LINKER_FLAGS_RELWITHDEBINFO}"
	CACHE STRING "Flags used for linking binaries during development builds" FORCE)
set(CMAKE_SHARED_LINKER_FLAGS_DEVEL "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO}"
	CACHE STRING "Flags used for linking shared libraries during development builds" FORCE)
set(CMAKE_EXE_LINKER_FLAGS_DEVEL "${CMAKE_EXE_LINKER_FLAGS_RELWITHDEBINFO}"
	CACHE STRING "Flags used for linking executables during development builds" FORCE)
if(CMAKE_CONFIGURATION_TYPES)
	list(INSERT CMAKE_CONFIGURATION_TYPES 0 Devel)
endif()
mark_as_advanced(CMAKE_C_FLAGS_DEVEL CMAKE_CXX_FLAGS_DEVEL CMAKE_LINKER_FLAGS_DEVEL CMAKE_SHARED_LINKER_FLAGS_DEVEL CMAKE_EXE_LINKER_FLAGS_DEVEL)
# AVX2 doesn't play well with gdb
if(CMAKE_BUILD_TYPE MATCHES "Debug")
	SET(DISABLE_ADVANCE_SIMD ON)
endif()

# Initially strip was disabled on release build but it is not stackstrace friendly!
# It only cost several MB so disbable it by default
option(CMAKE_BUILD_STRIP "Srip binaries to save a couple of MB (developer option)")

if(NOT DEFINED CMAKE_BUILD_PO)
	if(CMAKE_BUILD_TYPE STREQUAL "Release")
		set(CMAKE_BUILD_PO TRUE)
		message(STATUS "Enable the building of po files by default in ${CMAKE_BUILD_TYPE} build !!!")
	else()
		set(CMAKE_BUILD_PO FALSE)
		message(STATUS "Disable the building of po files by default in ${CMAKE_BUILD_TYPE} build !!!")
	endif()
endif()

#-------------------------------------------------------------------------------
# Select the architecture
#-------------------------------------------------------------------------------
option(DISABLE_ADVANCE_SIMD "Disable advance use of SIMD (SSE2+ & AVX)" OFF)

# Print if we are cross compiling.
if(CMAKE_CROSSCOMPILING)
	message(STATUS "Cross compilation is enabled.")
else()
	message(STATUS "Cross compilation is disabled.")
endif()

# Architecture bitness detection
include(TargetArch)
target_architecture(PCSX2_TARGET_ARCHITECTURES)
if(${PCSX2_TARGET_ARCHITECTURES} MATCHES "x86_64" OR ${PCSX2_TARGET_ARCHITECTURES} MATCHES "i386" OR
   ${PCSX2_TARGET_ARCHITECTURES} MATCHES "aarch64")
	message(STATUS "Compiling a ${PCSX2_TARGET_ARCHITECTURES} build on a ${CMAKE_HOST_SYSTEM_PROCESSOR} host.")
else()
	message(FATAL_ERROR "Unsupported architecture: ${PCSX2_TARGET_ARCHITECTURES}")
endif()

if(${PCSX2_TARGET_ARCHITECTURES} MATCHES "i386")
	# * -fPIC option was removed for multiple reasons.
	#     - Code only supports the x86 architecture.
	#     - code uses the ebx register so it's not compliant with PIC.
	#     - Impacts the performance too much.
	#     - Only plugins. No package will link to them.
	set(CMAKE_POSITION_INDEPENDENT_CODE OFF)

	if(NOT DEFINED ARCH_FLAG)
		if (MSVC)
			set(ARCH_FLAG /arch:SSE2)
		else()
			if (DISABLE_ADVANCE_SIMD)
				if (USE_ICC)
					set(ARCH_FLAG "-msse2 -msse4.1")
				else()
					set(ARCH_FLAG "-msse -msse2 -msse4.1 -mfxsr -march=i686")
				endif()
			else()
				# AVX requires some fix of the ABI (mangling) (default 2)
				# Note: V6 requires GCC 4.7
				#set(ARCH_FLAG "-march=native -fabi-version=6")
				set(ARCH_FLAG "-mfxsr -march=native")
			endif()
		endif()
	endif()

	list(APPEND PCSX2_DEFS _ARCH_32=1 _M_X86=1 _M_X86_32=1)
	set(_ARCH_32 1)
	set(_M_X86 1)
	set(_M_X86_32 1)
elseif(${PCSX2_TARGET_ARCHITECTURES} MATCHES "x86_64")
	# x86_64 requires -fPIC
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)

	if(NOT DEFINED ARCH_FLAG AND NOT MSVC)
		if (DISABLE_ADVANCE_SIMD)
			if (USE_ICC)
				set(ARCH_FLAG "-msse2 -msse4.1")
			else()
				set(ARCH_FLAG "-msse -msse2 -msse4.1 -mfxsr")
			endif()
		else()
			#set(ARCH_FLAG "-march=native -fabi-version=6")
			set(ARCH_FLAG "-march=native")
		endif()
	endif()
	list(APPEND PCSX2_DEFS _ARCH_64=1 _M_X86=1 _M_X86_64=1 __M_X86_64=1)
	set(_ARCH_64 1)
	set(_M_X86 1)
	set(_M_X86_64 1)
elseif(${PCSX2_TARGET_ARCHITECTURES} MATCHES "aarch64")
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_definitions(-D_ARCH_64=1 -D_M_ARM64=1)
    add_definitions("-march=armv8-a+crc")
    set(_ARCH_64 1)
    set(_M_ARM64 1)
else()
	# All but i386 requires -fPIC
	set(CMAKE_POSITION_INDEPENDENT_CODE ON)

	message(FATAL_ERROR "Unsupported architecture: ${PCSX2_TARGET_ARCHITECTURES}")
endif()
string(REPLACE " " ";" ARCH_FLAG_LIST "${ARCH_FLAG}")
add_compile_options("${ARCH_FLAG_LIST}")

#-------------------------------------------------------------------------------
# Set some default compiler flags
#-------------------------------------------------------------------------------
option(USE_PGO_GENERATE "Enable PGO optimization (generate profile)")
option(USE_PGO_OPTIMIZE "Enable PGO optimization (use profile)")

# Note1: Builtin strcmp/memcmp was proved to be slower on Mesa than stdlib version.
# Note2: float operation SSE is impacted by the PCSX2 SSE configuration. In particular, flush to zero denormal.
if(MSVC)
	add_compile_options("$<$<COMPILE_LANGUAGE:CXX>:/Zc:externConstexpr>")
else()
	if(${PCSX2_TARGET_ARCHITECTURES} MATCHES "aarch64")
		add_compile_options(-pipe -fvisibility=hidden -pthread)
	else()
		add_compile_options(-pipe -fvisibility=hidden -pthread -fno-builtin-strcmp -fno-builtin-memcmp -mfpmath=sse -fno-operator-names)
	endif()
endif()

if(WIN32)
	add_compile_definitions(
		$<$<CONFIG:Debug>:_ITERATOR_DEBUG_LEVEL=2>
		$<$<CONFIG:Devel>:_ITERATOR_DEBUG_LEVEL=1>
		$<$<CONFIG:RelWithDebInfo>:_ITERATOR_DEBUG_LEVEL=0>
		$<$<CONFIG:MinSizeRel>:_ITERATOR_DEBUG_LEVEL=0>
		$<$<CONFIG:Release>:_ITERATOR_DEBUG_LEVEL=0>
	)
	list(APPEND PCSX2_DEFS TIXML_USE_STL _SCL_SECURE_NO_WARNINGS _UNICODE UNICODE)
endif()

if(USE_VTUNE)
	list(APPEND PCSX2_DEFS ENABLE_VTUNE)
endif()

if(X11_API)
	list(APPEND PCSX2_DEFS X11_API)
endif()

if(WAYLAND_API)
	list(APPEND PCSX2_DEFS WAYLAND_API)
endif()

# -Wno-attributes: "always_inline function might not be inlinable" <= real spam (thousand of warnings!!!)
# -Wno-missing-field-initializers: standard allow to init only the begin of struct/array in static init. Just a silly warning.
# Note: future GCC (aka GCC 5.1.1) has less false positive so warning could maybe put back
# -Wno-unused-function: warn for function not used in release build
# -Wno-unused-value: lots of warning for this kind of statements "0 && ...". There are used to disable some parts of code in release/dev build.
# -Wno-overloaded-virtual: Gives a fair number of warnings under clang over in the wxwidget gui section of the code.
# -Wno-deprecated-declarations: The USB plugins dialogs are written in straight gtk 2, which gives a million deprecated warnings. Suppress them until we can deal with them.
# -Wno-format*: Yeah, these need to be taken care of, but...
# -Wno-stringop-truncation: Who comes up with these compiler warnings, anyways?
# -Wno-stringop-overflow: Probably the same people as this one...

if (MSVC)
	set(DEFAULT_WARNINGS)
else()
	set(DEFAULT_WARNINGS -Wall -Wextra -Wno-attributes -Wno-unused-function -Wno-unused-parameter -Wno-missing-field-initializers -Wno-deprecated-declarations -Wno-format -Wno-format-security -Wno-overloaded-virtual)
	if (NOT USE_ICC)
		list(APPEND DEFAULT_WARNINGS -Wno-unused-value)
	endif()
endif()

if (USE_CLANG)
	list(APPEND DEFAULT_WARNINGS -Wno-overloaded-virtual)
endif()

if (USE_GCC)
	list(APPEND DEFAULT_WARNINGS -Wno-stringop-truncation -Wno-stringop-overflow)
endif()


# -Wstrict-aliasing=n: to fix one day aliasing issue. n=1/2/3
if (USE_ICC)
	set(AGGRESSIVE_WARNING -Wstrict-aliasing)
elseif(NOT MSVC)
	set(AGGRESSIVE_WARNING -Wstrict-aliasing -Wstrict-overflow=1)
endif()

if (USE_CLANG)
	# -Wno-deprecated-register: glib issue...
	list(APPEND DEFAULT_WARNINGS -Wno-deprecated-register -Wno-c++14-extensions)
endif()

if (USE_PGO_GENERATE OR USE_PGO_OPTIMIZE)
	add_compile_options("-fprofile-dir=${CMAKE_SOURCE_DIR}/profile")
endif()

if (USE_PGO_GENERATE)
	add_compile_options(-fprofile-generate)
endif()

if(USE_PGO_OPTIMIZE)
	add_compile_options(-fprofile-use)
endif()

list(APPEND PCSX2_DEFS
	"$<$<CONFIG:Debug>:PCSX2_DEVBUILD;PCSX2_DEBUG;_DEBUG>"
	"$<$<CONFIG:Devel>:PCSX2_DEVBUILD;_DEVEL>"
	"$<$<CONFIG:Release>:NDEBUG>"
)

if (USE_ASAN)
	add_compile_options(-fsanitize=address)
	list(APPEND PCSX2_DEFS ASAN_WORKAROUND)
endif()

if(USE_CLANG AND TIMETRACE)
	add_compile_options(-ftime-trace)
endif()

set(PCSX2_WARNINGS ${DEFAULT_WARNINGS} ${AGGRESSIVE_WARNING})

if(CMAKE_BUILD_STRIP)
	add_link_options(-s)
endif()

if(QT_BUILD OR ANDROID)
	# We want the core PCSX2 library.
	set(PCSX2_CORE TRUE)
endif()

# Default symbol visibility to hidden, that way we don't go through the PLT for intra-library calls.
if(ANDROID)
  set(CMAKE_C_VISIBILITY_PRESET hidden)
  set(CMAKE_CXX_VISIBILITY_PRESET hidden)
endif()

# Enable special stuff for CI builds
if("$ENV{CI}" STREQUAL "true")
	list(APPEND PCSX2_DEFS PCSX2_CI)
endif()

#-------------------------------------------------------------------------------
# MacOS-specific things
#-------------------------------------------------------------------------------

set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)

if (APPLE AND ${CMAKE_OSX_DEPLOYMENT_TARGET} VERSION_LESS 10.14 AND NOT ${CMAKE_CXX_COMPILER_VERSION} VERSION_LESS 9)
	# Older versions of the macOS stdlib don't have operator new(size_t, align_val_t)
	# Disable use of them with this flag
	# Not great, but also no worse that what we were getting before we turned on C++17
	add_compile_options(-fno-aligned-allocation)
endif()

# CMake defaults the suffix for modules to .so on macOS but wx tells us that the
# extension is .dylib (so that's what we search for)
if(APPLE)
	set(CMAKE_SHARED_MODULE_SUFFIX ".dylib")
endif()

if(CMAKE_SYSTEM_NAME MATCHES "Darwin")
	if(NOT OSX_USE_DEFAULT_SEARCH_PATH)
		# Hack up the path to prioritize the path to built-in OS libraries to
		# increase the chance of not depending on a bunch of copies of them
		# installed by MacPorts, Fink, Homebrew, etc, and ending up copying
		# them into the bundle.  Since we depend on libraries which are not
		# part of OS X (wx, etc.), however, don't remove the default path
		# entirely.  This is still kinda evil, since it defeats the user's
		# path settings...
		# See http://www.cmake.org/cmake/help/v3.0/command/find_program.html
		list(APPEND CMAKE_PREFIX_PATH "/usr")
	endif()

	add_link_options(-Wl,-dead_strip,-dead_strip_dylibs)
endif()
