AddTarget(flidjvu		shared_lib
	PROJECT_GROUP		Foundation
	SOURCE_DIRECTORY	"${CMAKE_CURRENT_LIST_DIR}"
	EXCLUDE_SOURCES		${CMAKE_CURRENT_LIST_DIR}/unit-tests
	INCLUDE_DIRECTORIES     "${djvulibre_INCLUDE_DIR}"
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Gui
	LINK_TARGETS
		logging
		platform
)


AddGTest(flidjvu
	PROJECT_GROUP UT/Foundation
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}/unit-tests"
)

CopyAndInstallDjVuLibre()
