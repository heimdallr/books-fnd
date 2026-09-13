#pragma once

#include "export/util.h"

namespace HomeCompa::Util {

#define INSTALLER_MODE_ITEMS_X_MACRO                                                                                                                                                                           \
	INSTALLER_MODE_ITEM(exe, "exe", true, false)                                                                                                                                                               \
	INSTALLER_MODE_ITEM(wix, "msi", true, false)                                                                                                                                                               \
	INSTALLER_MODE_ITEM(dmg, "dmg", true, false)                                                                                                                                                               \
	INSTALLER_MODE_ITEM(portable, "7z", false, true)                                                                                                                                                           \
	INSTALLER_MODE_ITEM(deb, "deb", false, false)                                                                                                                                                              \
	INSTALLER_MODE_ITEM(txz, "xz", false, true)

enum class InstallerType
{
#define INSTALLER_MODE_ITEM(NAME, _1, _2, _3) NAME,
	INSTALLER_MODE_ITEMS_X_MACRO
#undef INSTALLER_MODE_ITEM
};

struct InstallerDescription
{
	InstallerType type;
	const char*   name;
	const char*   ext;
	bool          installable;
	bool          portable;
};

UTIL_EXPORT const InstallerDescription& GetInstallerDescription();

} // namespace HomeCompa::Util
