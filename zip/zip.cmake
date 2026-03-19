include_directories(${CMAKE_CURRENT_LIST_DIR})

AddTarget(zip	shared_lib
	PROJECT_GROUP Foundation/zip
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
    INCLUDE_DIRECTORIES
        "${EXT_ROOT}/bit7z/include"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Gui
        bit7z
	LINK_TARGETS
		logging
)

set(7zip_BIN_FILENAME)
if (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Windows)
	set(7zip_BIN_FILENAME 7z.dll)
elseif (${CMAKE_HOST_SYSTEM_NAME} STREQUAL Linux)
	set(7zip_BIN_FILENAME 7z.so)
else()
	message(FATAL_ERROR "Unsupported host system: ${CMAKE_HOST_SYSTEM_NAME}")
endif()

file(COPY "${7zip_BIN_DIR}/${7zip_BIN_FILENAME}" DESTINATION ${CMAKE_BINARY_DIR}/bin)
install(FILES "${7zip_BIN_DIR}/${7zip_BIN_FILENAME}" DESTINATION .)
