#include "PlatformUtil.h"

#include <Windows.h>

#include <QDir>
#include <QSettings>
#include <QWidget>

#include "fnd/FindPair.h"

namespace HomeCompa::Platform
{

namespace
{

constexpr std::pair<const char*, const wchar_t*> LOCALES[] {
	{ "en", L"00000409" },
	{ "ru", L"00000419" },
	{ "uk", L"00000422" },
};

std::unique_ptr<QSettings> GetStartupSettings()
{
	return std::make_unique<QSettings>(QSettings::NativeFormat, QSettings::UserScope, "Microsoft", "Windows");
}

QString GetStartupKey(const QString& key)
{
	static constexpr auto STARTUP_KEY_TEMPLATE = "CurrentVersion/Run/%1";
	return QString(STARTUP_KEY_TEMPLATE).arg(key);
}

}

PlatformType GetPlatformType() noexcept
{
	return PlatformType::Windows;
}

bool IsRegisteredExtension(const QString& extension)
{
	QSettings  m("HKEY_CLASSES_ROOT", QSettings::NativeFormat);
	const auto fileType = m.value(QString(".%1/.").arg(extension)).toString();
	if (fileType.isEmpty())
	{
		m.beginGroup(QString(".%1/OpenWithProgids").arg(extension));
		return !m.allKeys().isEmpty();
	}

	const auto command = m.value(QString("%1/shell/open/command/.").arg(fileType)).toString();
	return !command.isEmpty();
}

void SetKeyboardLayout(const QString& locale)
{
	const auto*                  keyboardLayoutId = FindSecond(LOCALES, locale.toStdString().data(), PszComparer {});
	[[maybe_unused]] const auto* klId             = LoadKeyboardLayout(keyboardLayoutId, KLF_ACTIVATE);
	assert(klId);
}

bool IsAppAddedToAutostart(const QString& key)
{
	return GetStartupSettings()->contains(GetStartupKey(key));
}

void AddToAutostart(const QString& key, const QString& path)
{
	GetStartupSettings()->setValue(GetStartupKey(key), QDir::toNativeSeparators(path));
}

void RemoveFromAutostart(const QString& key)
{
	GetStartupSettings()->remove(GetStartupKey(key));
}

void SetGeometry(QWidget& widget, const QRect& rect)
{
	widget.setGeometry(rect);
}

QString SetDefaultInstallerSuffix()
{
	return "exe";
}

} // namespace HomeCompa::Platform
