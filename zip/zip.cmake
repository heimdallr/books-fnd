include_directories(${CMAKE_CURRENT_LIST_DIR})

set(SEVENZIP_SRC_DIR "${EXT_ROOT}/zip/7z/7-zip/CPP/7zip/Bundles/Format7zF")
set(SEVENZIP_BUILD_DIR "${CMAKE_BINARY_DIR}/7z-build")

if(WIN32)
	set(7zip_BIN_FILENAME 7z.dll)
	set(7zip_BIN_PATH ${CMAKE_BINARY_DIR}/bin)
    set(SEVENZIP_OUTPUT "${SEVENZIP_BUILD_DIR}/${7zip_BIN_FILENAME}")
    file(TO_NATIVE_PATH "${SEVENZIP_BUILD_DIR}" SEVENZIP_BUILD_DIR_NATIVE)
	install(FILES "${SEVENZIP_BUILD_DIR}/${7zip_BIN_FILENAME}" DESTINATION .)

    add_custom_command(
        OUTPUT "${SEVENZIP_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SEVENZIP_BUILD_DIR}"
        COMMAND nmake /NOLOGO PLATFORM=x64 NEW_COMPILER=1 "O=${SEVENZIP_BUILD_DIR_NATIVE}"
        WORKING_DIRECTORY "${SEVENZIP_SRC_DIR}"
        COMMENT "Building 7z.dll with nmake..."
        VERBATIM
    )
elseif(UNIX)
	set(7zip_BIN_FILENAME 7z.so)
	set(7zip_BIN_PATH ${CMAKE_BINARY_DIR}/lib)
    set(SEVENZIP_OUTPUT "${SEVENZIP_BUILD_DIR}/${7zip_BIN_FILENAME}")
	install(FILES "${SEVENZIP_BUILD_DIR}/${7zip_BIN_FILENAME}" DESTINATION lib)
    if(APPLE)
    	if(CMAKE_OSX_ARCHITECTURES)
	        set(TARGET_ARCH "${CMAKE_OSX_ARCHITECTURES}")
    	else()
        	set(TARGET_ARCH "${CMAKE_SYSTEM_PROCESSOR}")
	    endif()

	    if(TARGET_ARCH STREQUAL "arm64")
    	    set(SEVENZIP_MAK "../../cmpl_mac_arm64.mak")
	    elseif(TARGET_ARCH STREQUAL "x86_64")
    	    set(SEVENZIP_MAK "../../cmpl_mac_x64.mak")
	    else()
    	    message(FATAL_ERROR "Unsupported macOS architecture for 7-Zip: ${TARGET_ARCH}")
	    endif()
    else()
        set(SEVENZIP_MAK "../../cmpl_gcc.mak")
    endif()

    include(ProcessorCount)
    ProcessorCount(NPROC)
    if(NPROC EQUAL 0)
        set(NPROC 4)
    endif()

    add_custom_command(
        OUTPUT "${SEVENZIP_OUTPUT}"
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${SEVENZIP_BUILD_DIR}"
        COMMAND make -j${NPROC} -f "${SEVENZIP_MAK}" "ODIR=${SEVENZIP_BUILD_DIR}"
        WORKING_DIRECTORY "${SEVENZIP_SRC_DIR}"
        COMMENT "Building 7z.so with make..."
        VERBATIM
    )
else()
	message(FATAL_ERROR "Unsupported host system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

set(SEVENZIP_FINAL_FILE ${7zip_BIN_PATH}/${7zip_BIN_FILENAME})

add_custom_command(
    OUTPUT "${SEVENZIP_FINAL_FILE}"
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different "${SEVENZIP_OUTPUT}" "${SEVENZIP_FINAL_FILE}"
    DEPENDS "${SEVENZIP_OUTPUT}"
    COMMENT "Copying ${7zip_BIN_FILENAME} to ${7zip_BIN_PATH}..."
    VERBATIM
)

add_custom_target(7z ALL DEPENDS "${SEVENZIP_FINAL_FILE}")
if(MSVC)
	set_property(TARGET 7z PROPERTY FOLDER Foundation/Zip)
endif()


AddTarget(zip	shared_lib
	PROJECT_GROUP Foundation/Zip
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	EXCLUDE_SOURCES
		${CMAKE_CURRENT_LIST_DIR}/unit-tests
    INCLUDE_DIRECTORIES
        "${EXT_ROOT}/zip/bit7z/include"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
        bit7z
	LINK_TARGETS
		fnd
		logging
		platform
	DEPENDENCIES
		7z
)

AddGTest(zip
	PROJECT_GROUP UT/Foundation/Zip
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}/unit-tests"
        LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
        LINK_TARGETS
		zip
)
