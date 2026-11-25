#include "Settings.h"

#include <mutex>
#include <ranges>

#include <QFile>
#include <QSettings>

#include "fnd/ScopedCall.h"
#include "fnd/memory.h"
#include "fnd/observable.h"

#include "ISettingsObserver.h"

namespace HomeCompa::SettingsFactory {

namespace {

constexpr auto CUSTOM_FORMAT = "settingsInMemory";

bool ReadMemorySettings(QIODevice& /*device*/, QSettings::SettingsMap& /*map*/)
{
	return true;
}

bool WriteMemorySettings(QIODevice& /*device*/, const QSettings::SettingsMap& /*map*/)
{
	return true;
}

void Copy(ISettings& dst, const ISettings& src)
{
	const auto enumerate = [&](const QString& prefix, const auto& r) -> void {
		for (const auto& key : src.GetKeys(prefix))
		{
			const auto prefixed = QString(prefix).append(prefix.isEmpty() ? "" : "/").append(key);
			const auto value    = src.Get(prefixed);
			dst.Set(prefixed, value, false);
		}

		for (const auto& group : src.GetGroups(prefix))
			r(QString(prefix).append(prefix.isEmpty() ? "" : "/").append(group), r);
	};

	enumerate({}, enumerate);
}

class SettingsStub final : public AbstractSettings
{
private: // ISettings
	[[nodiscard]] QVariant Get(const QString& /*key*/, const QVariant& defaultValue) const override
	{
		return defaultValue;
	}

	[[nodiscard]] QVariant Get(const QString& /*key*/, const std::function<QVariant()>& defaultValueGetter) const override
	{
		return defaultValueGetter();
	}

	bool Set(const QString& /*key*/, const QVariant& /*value*/, bool /*sync*/) override
	{
		return true;
	}

	[[nodiscard]] bool HasKey(const QString& /*key*/) const override
	{
		return false;
	}

	[[nodiscard]] bool HasGroup(const QString& /*group*/) const override
	{
		return false;
	}

	[[nodiscard]] QStringList GetKeys(const QString& /*prefix*/) const override
	{
		return {};
	}

	[[nodiscard]] QStringList GetGroups(const QString& /*prefix*/) const override
	{
		return {};
	}

	std::unique_ptr<ISettings> CreateGroupSettings(QString prefix) override
	{
		return CreateGroup(*this, std::move(prefix));
	}

	void Remove(const QString& /*key*/) override
	{
	}

	void Save(const QString& /*path*/, const QString& /*prefix*/) const override
	{
	}

	void Load(const QString& /*path*/, const QString& /*prefix*/) override
	{
	}

	void RegisterObserver(ISettingsObserver*) override
	{
	}

	void UnregisterObserver(ISettingsObserver*) override
	{
	}
};

class SettingsDecorator final : public AbstractSettings
{
public:
	SettingsDecorator(std::shared_ptr<ISettings> impl, std::unordered_map<QString, QVariant> replacement)
		: m_impl { std::move(impl) }
		, m_replacement { std::move(replacement) }
	{
	}

private: // ISettings
	[[nodiscard]] QVariant Get(const QString& key, const QVariant& defaultValue) const override
	{
		if (const auto it = m_replacement.find(key); it != m_replacement.end())
			return it->second;

		return m_impl->Get(key, defaultValue);
	}

	QVariant Get(const QString& key, const std::function<QVariant()>& defaultValueGetter) const override
	{
		if (const auto it = m_replacement.find(key); it != m_replacement.end())
			return it->second;

		return m_impl->Get(key, defaultValueGetter);
	}

	bool Set(const QString& key, const QVariant& value, const bool sync) override
	{
		if (const auto it = m_replacement.find(key); it != m_replacement.end())
		{
			if (it->second == value)
				return false;

			it->second = value;
			return true;
		}

		return m_impl->Set(key, value, sync);
	}

	[[nodiscard]] bool HasKey(const QString& key) const override
	{
		return m_replacement.contains(key) || m_impl->HasKey(key);
	}

	[[nodiscard]] bool HasGroup(const QString& group) const override
	{
		return m_impl->HasGroup(group);
	}

	[[nodiscard]] QStringList GetKeys(const QString& prefix) const override
	{
		auto keys = m_impl->GetKeys(prefix) | std::ranges::to<std::unordered_set>();
		std::ranges::copy(m_replacement | std::views::keys, std::inserter(keys, keys.end()));
		return keys | std::ranges::to<QStringList>();
	}

	[[nodiscard]] QStringList GetGroups(const QString& prefix) const override
	{
		return m_impl->GetGroups(prefix);
	}

	std::unique_ptr<ISettings> CreateGroupSettings(QString prefix) override
	{
		return CreateGroup(*this, std::move(prefix));
	}

	void Remove(const QString& key) override
	{
		m_impl->Remove(key);
		m_replacement.erase(key);
	}

	void Save(const QString& path, const QString& prefix) const override
	{
		m_impl->Save(path, prefix);
	}

	void Load(const QString& path, const QString& prefix) override
	{
		m_impl->Load(path, prefix);
	}

	void RegisterObserver(ISettingsObserver* observer) override
	{
		m_impl->RegisterObserver(observer);
	}

	void UnregisterObserver(ISettingsObserver* observer) override
	{
		m_impl->UnregisterObserver(observer);
	}

private:
	PropagateConstPtr<ISettings, std::shared_ptr> m_impl;
	std::unordered_map<QString, QVariant>         m_replacement;
};

class SettingsGroup final : public AbstractSettings
{
public:
	SettingsGroup(ISettings& impl, QString prefix)
		: m_impl { impl }
		, m_prefix { std::move(prefix) }
	{
	}

private: // ISettings
	[[nodiscard]] QVariant Get(const QString& key, const QVariant& defaultValue) const override
	{
		return m_impl.Get(Key(key), defaultValue);
	}

	QVariant Get(const QString& key, const std::function<QVariant()>& defaultValueGetter) const override
	{
		return m_impl.Get(Key(key), defaultValueGetter);
	}

	bool Set(const QString& key, const QVariant& value, const bool sync) override
	{
		return m_impl.Set(Key(key), value, sync);
	}

	[[nodiscard]] bool HasKey(const QString& key) const override
	{
		return m_impl.HasKey(Key(key));
	}

	[[nodiscard]] bool HasGroup(const QString& group) const override
	{
		return m_prefix.isEmpty() ? m_impl.HasGroup(group) : m_impl.GetGroups(m_prefix).contains(group);
	}

	[[nodiscard]] QStringList GetKeys(const QString& prefix) const override
	{
		return m_impl.GetKeys(Key(prefix));
	}

	[[nodiscard]] QStringList GetGroups(const QString& prefix) const override
	{
		return m_impl.GetGroups(Key(prefix));
	}

	std::unique_ptr<ISettings> CreateGroupSettings(QString prefix) override
	{
		return CreateGroup(*this, std::move(prefix));
	}

	void Remove(const QString& key) override
	{
		m_impl.Remove(Key(key));
	}

	void Save(const QString& path, const QString& prefix) const override
	{
		m_impl.Save(path, Key(prefix));
	}

	void Load(const QString& path, const QString& prefix) override
	{
		m_impl.Load(path, Key(prefix));
	}

	void RegisterObserver(ISettingsObserver* observer) override
	{
		m_impl.RegisterObserver(observer);
	}

	void UnregisterObserver(ISettingsObserver* observer) override
	{
		m_impl.UnregisterObserver(observer);
	}

private:
	[[nodiscard]] QString Key(const QString& key) const
	{
		return QString(m_prefix).append(m_prefix.isEmpty() || key.isEmpty() ? "" : "/").append(key);
	}

private:
	ISettings& m_impl;
	QString    m_prefix;
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

	Settings(const QSettings::Format format, const QString& organization, const QString& application)
		: m_settings(format, QSettings::UserScope, organization, application)
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

	QVariant Get(const QString& key, const std::function<QVariant()>& defaultValueGetter) const override
	{
		std::lock_guard lock(m_mutex);
		if (auto value = m_settings.value(key); value.isValid())
			return value;
		return defaultValueGetter();
	}

	bool Set(const QString& key, const QVariant& value, const bool sync) override
	{
		if (Get(key) == value)
			return false;

		std::lock_guard lock(m_mutex);
		m_settings.setValue(key, value);
		Perform(&ISettingsObserver::HandleValueChanged, key, std::cref(value));

		if (sync)
			m_settings.sync();

		return true;
	}

	[[nodiscard]] bool HasKey(const QString& key) const override
	{
		std::lock_guard lock(m_mutex);
		return m_settings.contains(key);
	}

	[[nodiscard]] bool HasGroup(const QString& group) const override
	{
		return GetGroups({}).contains(group);
	}

	[[nodiscard]] QStringList GetKeys(const QString& prefix) const override
	{
		std::lock_guard lock(m_mutex);
		if (prefix.isEmpty())
			return m_settings.childKeys();

		const ScopedCall groupGuard(
			[&] {
				m_settings.beginGroup(prefix);
			},
			[this] {
				m_settings.endGroup();
			}
		);
		return m_settings.childKeys();
	}

	[[nodiscard]] QStringList GetGroups(const QString& prefix) const override
	{
		std::lock_guard lock(m_mutex);
		if (prefix.isEmpty())
			return m_settings.childGroups();

		const ScopedCall groupGuard(
			[&] {
				m_settings.beginGroup(prefix);
			},
			[this] {
				m_settings.endGroup();
			}
		);
		return m_settings.childGroups();
	}

	std::unique_ptr<ISettings> CreateGroupSettings(QString prefix) override
	{
		return CreateGroup(*this, std::move(prefix));
	}

	void Remove(const QString& key) override
	{
		std::lock_guard lock(m_mutex);
		m_settings.remove(key);
		m_settings.sync();
	}

	void Save(const QString& path, const QString& prefix) const override
	{
		QFile::remove(path);
		Settings dst(path);

		std::lock_guard lock(m_mutex);
		if (prefix.isEmpty())
			return Copy(dst, *this);

		const ScopedCall groupGuard(
			[&] {
				m_settings.beginGroup(prefix);
			},
			[this] {
				m_settings.endGroup();
			}
		);
		Copy(dst, *this);
	}

	void Load(const QString& path, const QString& prefix) override
	{
		assert(QFile::exists(path));
		const Settings src(path);

		std::lock_guard lock(m_mutex);
		if (prefix.isEmpty())
			return Copy(*this, src);

		const ScopedCall groupGuard(
			[&] {
				m_settings.beginGroup(prefix);
			},
			[this] {
				m_settings.endGroup();
			}
		);
		Copy(*this, src);
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
	mutable QSettings            m_settings;
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

std::unique_ptr<AbstractSettings> CreateMemory(const QString& organization, const QString& application)
{
	const auto format = QSettings::registerFormat(CUSTOM_FORMAT, &ReadMemorySettings, &WriteMemorySettings);
	return std::make_unique<Settings>(format, organization, application);
}

std::unique_ptr<AbstractSettings> CreateDecorator(std::shared_ptr<ISettings> impl, std::unordered_map<QString, QVariant> replacement)
{
	return std::make_unique<SettingsDecorator>(std::move(impl), std::move(replacement));
}

std::unique_ptr<AbstractSettings> CreateGroup(ISettings& impl, QString prefix)
{
	return std::make_unique<SettingsGroup>(impl, std::move(prefix));
}

} // namespace HomeCompa::SettingsFactory
