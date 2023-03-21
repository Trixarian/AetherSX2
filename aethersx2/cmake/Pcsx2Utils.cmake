#-------------------------------------------------------------------------------
#                       detectOperatingSystem
#-------------------------------------------------------------------------------
# This function detects on which OS cmake is run and set a flag to control the
# build process. Supported OS: Linux, MacOSX, Windows
# 
# On linux, it also set a flag for specific distribution (ie Fedora)
#-------------------------------------------------------------------------------
function(detectOperatingSystem)
	if(WIN32)
		set(Windows TRUE PARENT_SCOPE)
	elseif(UNIX AND APPLE)
		# No easy way to filter out iOS.
		message(WARNING "OS X/iOS isn't supported, the build will most likely fail")
		set(MacOSX TRUE PARENT_SCOPE)
	elseif(UNIX)
		if(CMAKE_SYSTEM_NAME MATCHES "Linux")
			set(Linux TRUE PARENT_SCOPE)
			if (EXISTS /etc/os-release)
				# Read the file without CR character
				file(STRINGS /etc/os-release OS_RELEASE)
				if("${OS_RELEASE}" MATCHES "^.*ID=fedora.*$")
					set(Fedora TRUE PARENT_SCOPE)
					message(STATUS "Build Fedora specific")
				elseif("${OS_RELEASE}" MATCHES "^.*ID=.*suse.*$")
					set(openSUSE TRUE PARENT_SCOPE)
					message(STATUS "Build openSUSE specific")
				endif()
			endif()
		elseif(CMAKE_SYSTEM_NAME MATCHES "kFreeBSD")
			set(kFreeBSD TRUE PARENT_SCOPE)
		elseif(CMAKE_SYSTEM_NAME STREQUAL "GNU")
			set(GNU TRUE PARENT_SCOPE)
		endif()
	endif()
endfunction()

function(get_git_version_info)
	set(PCSX2_WC_TIME 0)
	set(PCSX2_GIT_REV "")
	set(PCSX2_GIT_TAG "")
	if (GIT_FOUND AND EXISTS ${PROJECT_SOURCE_DIR}/.git)
		EXECUTE_PROCESS(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${GIT_EXECUTABLE} show -s --format=%ci HEAD
			OUTPUT_VARIABLE PCSX2_WC_TIME
			OUTPUT_STRIP_TRAILING_WHITESPACE)
		# Output: "YYYY-MM-DD HH:MM:SS +HHMM" (last part is time zone, offset from UTC)
		string(REGEX REPLACE "[%:\\-]" "" PCSX2_WC_TIME "${PCSX2_WC_TIME}")
		string(REGEX REPLACE "([0-9]+) ([0-9]+).*" "\\1\\2" PCSX2_WC_TIME "${PCSX2_WC_TIME}")

		EXECUTE_PROCESS(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${GIT_EXECUTABLE} describe
			OUTPUT_VARIABLE PCSX2_GIT_REV
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET)

		EXECUTE_PROCESS(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR} COMMAND ${GIT_EXECUTABLE} tag --points-at HEAD
			OUTPUT_VARIABLE PCSX2_GIT_TAG
			OUTPUT_STRIP_TRAILING_WHITESPACE
			ERROR_QUIET)
	endif()
	if(PCSX2_GIT_REV)
		set(PCSX2_VERSION_LONG "${PCSX2_GIT_REV}")
		string(REGEX MATCH "[0-9]+\\.[0-9]+(\\.[0-9]+)?(-[a-z][a-z0-9]+)?" PCSX2_VERSION_SHORT "${PCSX2_VERSION_LONG}")
	else()
		set(PCSX2_VERSION_LONG "Unknown (git unavailable)")
		set(PCSX2_VERSION_SHORT "Unknown")
	endif()
	if ("${PCSX2_WC_TIME}" STREQUAL "")
		set(PCSX2_WC_TIME 0)
	endif()

	set(PCSX2_WC_TIME "${PCSX2_WC_TIME}" PARENT_SCOPE)
	set(PCSX2_GIT_REV "${PCSX2_GIT_REV}" PARENT_SCOPE)
	set(PCSX2_GIT_TAG "${PCSX2_GIT_TAG}" PARENT_SCOPE)
	set(PCSX2_VERSION_LONG "${PCSX2_VERSION_LONG}" PARENT_SCOPE)
	set(PCSX2_VERSION_SHORT "${PCSX2_VERSION_SHORT}" PARENT_SCOPE)
endfunction()

function(write_svnrev_h)
	if(PCSX2_GIT_TAG)
		file(WRITE ${CMAKE_BINARY_DIR}/common/include/svnrev.h "#define SVN_REV ${PCSX2_WC_TIME}ll \n#define GIT_TAG \"${PCSX2_GIT_TAG}\"\n#define GIT_TAGGED_COMMIT 1\n#define GIT_REV \"\"\n")
	else()
		file(WRITE ${CMAKE_BINARY_DIR}/common/include/svnrev.h "#define SVN_REV ${PCSX2_WC_TIME}ll \n#define GIT_TAG \"${PCSX2_GIT_TAG}\"\n#define GIT_TAGGED_COMMIT 0\n#define GIT_REV \"${PCSX2_GIT_REV}\"\n")
	endif()
endfunction()

function(check_compiler_version version_warn version_err)
	if(CMAKE_COMPILER_IS_GNUCXX)
		execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE GCC_VERSION)
		string(STRIP "${GCC_VERSION}" GCC_VERSION)
		if(GCC_VERSION VERSION_LESS ${version_err})
			message(FATAL_ERROR "PCSX2 doesn't support your old GCC ${GCC_VERSION}! Please upgrade it!

			The minimum supported version is ${version_err} but ${version_warn} is warmly recommended")
		else()
			if(GCC_VERSION VERSION_LESS ${version_warn})
				message(WARNING "PCSX2 will stop supporting GCC ${GCC_VERSION} in the near future. Please upgrade to at least GCC ${version_warn}.")
			endif()
		endif()

		set(GCC_VERSION "${GCC_VERSION}" PARENT_SCOPE)
	endif()
endfunction()

function(check_no_parenthesis_in_path)
	if ("${CMAKE_BINARY_DIR}" MATCHES "[()]" OR "${CMAKE_SOURCE_DIR}" MATCHES "[()]")
		message(FATAL_ERROR "Your path contains some parenthesis. Unfortunately Cmake doesn't support them correctly.\nPlease rename your directory to avoid '(' and ')' characters\n")
	endif()
endfunction()

# Makes an imported target if it doesn't exist.  Useful for when find scripts from older versions of cmake don't make the targets you need
function(make_imported_target_if_missing target lib)
	if(${lib}_FOUND AND NOT TARGET ${target})
		add_library(_${lib} INTERFACE)
		target_link_libraries(_${lib} INTERFACE "${${lib}_LIBRARIES}")
		target_include_directories(_${lib} INTERFACE "${${lib}_INCLUDE_DIRS}")
		add_library(${target} ALIAS _${lib})
	endif()
endfunction()

# like add_library(new ALIAS old) but avoids add_library cannot create ALIAS target "new" because target "old" is imported but not globally visible. on older cmake
function(alias_library new old)
	string(REPLACE "::" "" library_no_namespace ${old})
	add_library(_alias_${library_no_namespace} INTERFACE)
	target_link_libraries(_alias_${library_no_namespace} INTERFACE ${old})
	add_library(${new} ALIAS _alias_${library_no_namespace})
endfunction()

# Helper macro to generate resources on linux (based on glib)
macro(add_custom_glib_res out xml prefix)
	set(RESOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/res")
	set(RESOURCE_FILES "${ARGN}")
	# Note: trying to combine --generate-source and --generate-header doesn't work.
	# It outputs whichever one comes last into the file named by the first
	add_custom_command(
		OUTPUT ${out}.h
		COMMAND glib-compile-resources --sourcedir "${RESOURCE_DIR}" --generate-header
			--c-name ${prefix} "${RESOURCE_DIR}/${xml}" --target=${out}.h
		DEPENDS res/${xml} ${RESOURCE_FILES})

	add_custom_command(
		OUTPUT ${out}.cpp
		COMMAND glib-compile-resources --sourcedir "${RESOURCE_DIR}" --generate-source
			--c-name ${prefix} "${RESOURCE_DIR}/${xml}" --target=${out}.cpp
		DEPENDS res/${xml} ${RESOURCE_FILES})
endmacro()

function(source_groups_from_vcxproj_filters file)
	file(READ "${file}" filecontent)
	get_filename_component(parent "${file}" DIRECTORY)
	if (parent STREQUAL "")
		set(parent ".")
	endif()
	set(regex "<[^ ]+ Include=\"([^\"]+)\">[ \t\r\n]+<Filter>([^<]+)<\\/Filter>[ \t\r\n]+<\\/[^ >]+>")
	string(REGEX MATCHALL "${regex}" filterstrings "${filecontent}")
	foreach(filterstring IN LISTS filterstrings)
		string(REGEX REPLACE "${regex}" "\\1" path "${filterstring}")
		string(REGEX REPLACE "${regex}" "\\2" group "${filterstring}")
		source_group("${group}" FILES "${parent}/${path}")
	endforeach()
endfunction()
