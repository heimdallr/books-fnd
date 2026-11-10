include_directories(${CMAKE_CURRENT_LIST_DIR}/github_api/include)

AddTarget(network	shared_lib
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}"
	EXCLUDE_SOURCES
		${CMAKE_CURRENT_LIST_DIR}/unit-tests
	PROJECT_GROUP Foundation/Network
	LINK_TARGETS
		logging
	LINK_LIBRARIES
		Qt${QT_MAJOR_VERSION}::Network
)

AddGTest(network
	PROJECT_GROUP UT/Foundation/Network
	SOURCE_DIRECTORY
		"${CMAKE_CURRENT_LIST_DIR}/unit-tests"
)
