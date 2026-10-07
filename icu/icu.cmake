AddTarget(flicu			shared_lib
	PROJECT_GROUP		Foundation
	SOURCE_DIRECTORY	"${CMAKE_CURRENT_LIST_DIR}"
	LINK_LIBRARIES
		ICU::dt
		ICU::io
		ICU::uc
		Qt${QT_MAJOR_VERSION}::Core
	LINK_TARGETS
		logging
)
