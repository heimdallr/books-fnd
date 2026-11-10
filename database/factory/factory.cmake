AddTarget(dbfactory	shared_lib
	PROJECT_GROUP Foundation/Database
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	EXCLUDE_SOURCES
		${CMAKE_CURRENT_LIST_DIR}/unit-tests
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Core
	LINK_TARGETS
		dbsqlite
		logging
)

AddGTest(dbfactory
	PROJECT_GROUP UT/Foundation/Database
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}/unit-tests"
)
