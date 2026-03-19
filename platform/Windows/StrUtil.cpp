#include "StrUtil.h"

#include <QString>

namespace HomeCompa::Util
{

QString PathToString(const std::filesystem::path& path)
{
	return QString::fromStdWString(path);
}

std::filesystem::path StringToPath(const QString& string)
{
	return std::filesystem::path { string.toStdWString() };
}

}
