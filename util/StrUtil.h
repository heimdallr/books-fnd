#pragma once

#include <QHash>
#include <QString>

#include "export/util.h"

class QString;

namespace HomeCompa::Util {

UTIL_EXPORT std::vector<QString> UniqTitle(QString value);
UTIL_EXPORT QString&             Normalize(QString& value);
UTIL_EXPORT void                 AppendTitle(QString& title, const QString& str, const QString& delimiter = " ");

template <typename T>
T ToInteger(const QStringView value, T defaultValue = 0)
{
	if (value.isEmpty())
		return defaultValue;

	bool       ok     = false;
	const auto result = value.toLongLong(&ok);
	return ok ? static_cast<T>(result) : defaultValue;
}

} // namespace HomeCompa::Util
