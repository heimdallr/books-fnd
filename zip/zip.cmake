include_directories(${CMAKE_CURRENT_LIST_DIR})

AddTarget(zip	shared_lib
	PROJECT_GROUP Foundation/zip
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
    INCLUDE_DIRECTORIES
        "${EXT_ROOT}/bit7z/include/bit7z"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Gui
		Shlwapi.lib
		7zip::7zip
        bit7z
	LINK_TARGETS
		logging
)

string(TOUPPER ${CMAKE_BUILD_TYPE} CBTUP)
file(COPY "${7zip_BIN_DIRS_${CBTUP}}/7z.dll" DESTINATION ${CMAKE_BINARY_DIR}/bin)
install(FILES "${7zip_BIN_DIRS_${CBTUP}}/7z.dll" DESTINATION .)
