AddTarget(flidjvu		shared_lib
	PROJECT_GROUP		Foundation
	SOURCE_DIRECTORY	"${CMAKE_CURRENT_LIST_DIR}"
	LINK_LIBRARIES
		djvulibre::djvulibre
		Qt${QT_MAJOR_VERSION}::Core
		Qt${QT_MAJOR_VERSION}::Gui
	LINK_TARGETS
		logging
		platform
)

CopyAndInstallDjVuLibre()
