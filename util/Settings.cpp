#include "Settings.h"

#include <QFile>
#include <QSettings>

#include "fnd/observable.h"

#include "ISettingsObserver.h"

namespace HomeCompa::SettingsFactory
{

namespace
{

void Copy(ISettings& dst, const ISettings& src)
{
	const auto enumerate = [&](const auto& r) -> void {
		for (const auto& key : src.GetKeys())
		{
			const auto value = src.Get(key);
			dst.Set(key, value, false);
		}

		for (const auto& group : src.GetGroups())
		{
			const SettingsGroup dstGroup(dst, group), srcGroup(src, group);
			r(r);
		}
	};

	enumerate(enumerate);
}

class SettingsStub final : public AbstractSettings
{
private: // ISettings
	[[nodiscard]] QVariant Get(const QString& /*key*/, const QVariant& defaultValue) const override
	{
		return defaultValue;
	}

	void Set(const QString& /*key*/, const QVariant& /*value*/, bool /*sync*/) override
	{
	}

	[[nodiscard]] bool HasKey(const QString& /*key*/) const override
	{
		return false;
	}

	[[nodiscard]] bool HasGroup(const QString& /*group*/) const override
	{
		return false;
	}

	[[nodiscard]] QStringList GetKeys() const override
	{
		return {};
	}

	[[nodiscard]] QStringList GetGroups() const override
	{
		return {};
	}

	void Remove(const QString& /*key*/) override
	{
	}

	void Save(const QString& /*path*/) const override
	{
	}

	void Load(const QString& /*path*/) override
	{
	}

	std::recursive_mutex& BeginGroup(const QString&) const override
	{
		return m_mutex;
	}

	void EndGroup() const override
	{
	}

	void RegisterObserver(ISettingsObserver*) override
	{
	}

	void UnregisterObserver(ISettingsObserver*) override
	{
	}

private:
	mutable std::recursive_mutex m_mutex;
};

class Settings final
	: public AbstractSettings
	, Observable<ISettingsObserver>
{
public:
	explicit Settings(const QString& fileName)
		: m_settings(fileName, QSettings::Format::IniFormat)
	{
	}

	Settings(const QString& organization, const QString& application)
		: m_settings(QSettings::NativeFormat, QSettings::UserScope, organization, application)
	{
	}

private: // ISettings
	[[nodiscard]] QVariant Get(const QString& key, const QVariant& defaultValue = {}) const override
	{
		std::lock_guard lock(m_mutex);
		if (auto value = m_settings.value(key); value.isValid())
			return value;
		return defaultValue;
	}

	void Set(const QString& key, const QVariant& value, const bool sync) override
	{
		if (Get(key) == value)
			return;

		std::lock_guard lock(m_mutex);
		m_settings.setValue(key, value);
		if (!sync)
			return;

		m_settings.sync();
		Perform(&ISettingsObserver::HandleValueChanged, Key(key), std::cref(value));
	}

	[[nodiscard]] bool HasKey(const QString& key) const override
	{
		std::lock_guard lock(m_mutex);
		return m_settings.contains(key);
	}

	[[nodiscard]] bool HasGroup(const QString& group) const override
	{
		return GetGroups().contains(group);
	}

	[[nodiscard]] QStringList GetKeys() const override
	{
		std::lock_guard lock(m_mutex);
		return m_settings.childKeys();
	}

	[[nodiscard]] QStringList GetGroups() const override
	{
		std::lock_guard lock(m_mutex);
		return m_settings.childGroups();
	}

	void Remove(const QString& key) override
	{
		std::lock_guard lock(m_mutex);
		m_settings.remove(key);
		m_settings.sync();
	}

	void Save(const QString& path) const override
	{
		QFile::remove(path);
		Settings dst(path);
		Copy(dst, *this);
	}

	void Load(const QString& path) override
	{
		assert(QFile::exists(path));
		const Settings src(path);
		Copy(*this, src);
	}

	std::recursive_mutex& BeginGroup(const QString& group) const override
	{
		std::lock_guard lock(m_mutex);
		m_settings.beginGroup(group);
		m_group.push_back(Key(group));
		return m_mutex;
	}

	void EndGroup() const override
	{
		std::lock_guard lock(m_mutex);
		m_settings.endGroup();
		m_group.pop_back();
	}

	void RegisterObserver(ISettingsObserver* observer) override
	{
		Register(observer);
	}

	void UnregisterObserver(ISettingsObserver* observer) override
	{
		Unregister(observer);
	}

private:
	QString Key(const QString& key) const
	{
		return m_group.back() + (m_group.back().isEmpty() ? "" : "/") + key;
	}

private:
	mutable QSettings            m_settings;
	mutable std::vector<QString> m_group { 1 };
	mutable std::recursive_mutex m_mutex;
};

} // namespace

std::unique_ptr<AbstractSettings> CreateStub()
{
	return std::make_unique<SettingsStub>();
}

std::unique_ptr<AbstractSettings> Create(const QString& fileName)
{
	return std::make_unique<Settings>(fileName);
}

std::unique_ptr<AbstractSettings> Create(const QString& organization, const QString& application)
{
	return std::make_unique<Settings>(organization, application);
}

} // namespace HomeCompa::SettingsFactory
