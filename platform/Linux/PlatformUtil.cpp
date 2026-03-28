#include "PlatformUtil.h"

#include <QWidget>

namespace HomeCompa::Platform
{

bool IsRegisteredExtension(const QString& /*extension*/)
{
	return true;
}

void SetKeyboardLayout(const QString& /*locale*/)
{
	assert(false);
}

bool IsAppAddedToAutostart(const QString& /*key*/)
{
	assert(false);
}

void AddToAutostart(const QString& /*key*/, const QString& /*path*/)
{
	assert(false);
}

void RemoveFromAutostart(const QString& /*key*/)
{
	assert(false);
}

void SetGeometry(QWidget& widget, const QRect& rect)
{
    widget.resize(rect.size());
}

} // namespace HomeCompa::Util
