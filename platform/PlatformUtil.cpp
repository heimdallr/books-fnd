#include "PlatformUtil.h"

#include <QString>

namespace HomeCompa::Platform
{

PLATFORM_EXPORT QString GetPlatformName() noexcept
{
	static constexpr const char* NAMES[] {
#define PLATFORM_TYPE_ITEM(NAME) #NAME,
		PLATFORM_TYPE_ITEMS_X_MACRO
#undef PLATFORM_TYPE_ITEM
	};

	return NAMES[static_cast<size_t>(GetPlatformType())];
}

} // namespace HomeCompa::Platform
