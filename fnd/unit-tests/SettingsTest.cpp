#include "fnd/Settings.h"

#include <ranges>
#include <thread>

#include <QTemporaryDir>

#include <gtest/gtest.h>

#include "fnd/ISettings.h"

#include "mock/util/MockSettings.h"
#include "mock/util/MockSettingsObserver.h"

using namespace HomeCompa::Test;
using namespace HomeCompa;

namespace {

TEST(SettingsStub, Test)
{
	const auto settings = SettingsFactory::CreateStub();
	EXPECT_EQ(settings->Get("key", 42), 42);
	EXPECT_EQ(
		settings->Get(
			"key",
			[]() {
				return QVariant::fromValue(42);
			}
		),
		42
	);
	EXPECT_TRUE(settings->Set("key", 42));
	EXPECT_FALSE(settings->HasKey("key"));
	EXPECT_FALSE(settings->HasGroup("group"));
	EXPECT_TRUE(settings->GetKeys().isEmpty());
	EXPECT_TRUE(settings->GetGroups().isEmpty());
	EXPECT_TRUE(!!settings->CreateGroupSettings("prefix"));
	EXPECT_NO_THROW(settings->Remove("key"));
	EXPECT_NO_THROW(settings->Save({}, {}));
	EXPECT_NO_THROW(settings->Load({}, {}));
	EXPECT_NO_THROW(settings->RegisterObserver(nullptr));
	EXPECT_NO_THROW(settings->UnregisterObserver(nullptr));
}

TEST(SettingsFile, Test)
{
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const auto settings = SettingsFactory::Create(dir.filePath("settings.ini"));
	EXPECT_EQ(settings->Get("key", 42), 42);
	EXPECT_EQ(
		settings->Get(
			"key",
			[]() {
				return QVariant::fromValue(42);
			}
		),
		42
	);
	EXPECT_FALSE(settings->HasKey("key"));
	EXPECT_TRUE(settings->Set("key", 42));
	EXPECT_TRUE(settings->HasKey("key"));
	EXPECT_EQ(settings->Get("key").toInt(), 42);
	EXPECT_TRUE(settings->Set("group/key", 42));
	EXPECT_TRUE(settings->HasGroup("group"));
	EXPECT_NO_THROW(settings->Remove("key"));
	EXPECT_FALSE(settings->HasKey("key"));
}

struct SettingsGroupTest : testing::Test
{
	testing::StrictMock<MockSettings> impl;
	const std::unique_ptr<ISettings>  settings { SettingsFactory::CreateGroup(impl, "prefix") };
};

TEST_F(SettingsGroupTest, Get)
{
	EXPECT_CALL(impl, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce([](const QString& key, const QVariant& defaultValue) {
		EXPECT_EQ(key, "prefix/key1");
		EXPECT_EQ(defaultValue.toInt(), 42);
		return QVariant { 17 };
	});
	EXPECT_EQ(settings->Get("key1", 42), 17);

	EXPECT_CALL(impl, Get(testing::_, testing::Matcher<const std::function<QVariant()>&>(testing::_))).WillOnce([](const QString& key, const auto& defaultValue) {
		EXPECT_EQ(key, "prefix/key2");
		return defaultValue();
	});
	EXPECT_EQ(
		settings
			->Get(
				"key2",
				[]() {
					return QVariant { QString { "hello" } };
				}
			)
			.toString(),
		"hello"
	);
}

TEST_F(SettingsGroupTest, Set)
{
	EXPECT_CALL(impl, Set(testing::_, testing::_, testing::_)).WillOnce([](const QString& key, const QVariant& value, const bool sync) {
		EXPECT_STREQ(key.toStdString().data(), "prefix/key1");
		EXPECT_DOUBLE_EQ(value.toDouble(), 42.0);
		EXPECT_TRUE(sync);
		return true;
	});
	EXPECT_TRUE(settings->Set("key1", 42.0));

	EXPECT_CALL(impl, Set(testing::_, testing::_, testing::_)).WillOnce([](const QString& key, const QVariant& value, const bool sync) {
		EXPECT_STREQ(key.toStdString().data(), "prefix/key2");
		EXPECT_EQ(value.toString(), "hello");
		EXPECT_FALSE(sync);
		return false;
	});
	EXPECT_FALSE(settings->Set("key2", QString("hello"), false));
}

TEST_F(SettingsGroupTest, HasKey)
{
	EXPECT_CALL(impl, HasKey)
		.WillOnce([](const QString& key) {
			EXPECT_EQ(key, "prefix/key1");
			return true;
		})
		.WillOnce([](const QString& key) {
			EXPECT_EQ(key, "prefix/key2");
			return false;
		});
	EXPECT_TRUE(settings->HasKey("key1"));
	EXPECT_FALSE(settings->HasKey("key2"));
}

TEST_F(SettingsGroupTest, HasGroup)
{
	EXPECT_CALL(impl, GetGroups).Times(2).WillRepeatedly([](const QString& group) {
		EXPECT_EQ(group, "prefix");
		return QStringList { "group", "group1" };
	});
	EXPECT_TRUE(settings->HasGroup("group1"));
	EXPECT_FALSE(settings->HasGroup("group2"));
}

TEST_F(SettingsGroupTest, GetKeys)
{
	QStringList result { "key1", "key2" };
	EXPECT_CALL(impl, GetKeys)
		.WillOnce([&](const QString& prefix) {
			EXPECT_EQ(prefix, "prefix");
			return result;
		})
		.WillOnce([&](const QString& prefix) {
			EXPECT_EQ(prefix, "prefix/group");
			return QStringList {};
		});
	EXPECT_EQ(settings->GetKeys(), result);
	EXPECT_TRUE(settings->GetKeys("group").isEmpty());
}

TEST_F(SettingsGroupTest, GetGroups)
{
	QStringList result { "group1", "group2" };
	EXPECT_CALL(impl, GetGroups)
		.WillOnce([&](const QString& prefix) {
			EXPECT_EQ(prefix, "prefix");
			return result;
		})
		.WillOnce([&](const QString& prefix) {
			EXPECT_EQ(prefix, "prefix/group");
			return QStringList {};
		});
	EXPECT_EQ(settings->GetGroups(), result);
	EXPECT_TRUE(settings->GetGroups("group").isEmpty());
}

TEST_F(SettingsGroupTest, CreateGroupSettings)
{
	const auto subSettings = settings->CreateGroupSettings("subPrefix");
	EXPECT_TRUE(!!subSettings);

	EXPECT_CALL(impl, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce([](const QString& key, const QVariant& defaultValue) {
		EXPECT_STREQ(key.toStdString().data(), "prefix/subPrefix/key");
		EXPECT_FALSE(defaultValue.isValid());
		return QVariant { 42 };
	});
	EXPECT_EQ(subSettings->Get("key").toInt(), 42);
}

TEST_F(SettingsGroupTest, Save)
{
	bool called = false;
	EXPECT_CALL(impl, Save).WillOnce([&](const QString& path, const QString& prefix) {
		called = true;
		EXPECT_EQ(path, "path");
		EXPECT_EQ(prefix, "prefix/subPrefix");
	});
	EXPECT_NO_THROW(settings->Save("path", "subPrefix"));
	EXPECT_TRUE(called);
}

TEST_F(SettingsGroupTest, Load)
{
	bool called = false;
	EXPECT_CALL(impl, Load).WillOnce([&](const QString& path, const QString& prefix) {
		called = true;
		EXPECT_EQ(path, "path");
		EXPECT_EQ(prefix, "prefix/subPrefix");
	});
	EXPECT_NO_THROW(settings->Load("path", "subPrefix"));
	EXPECT_TRUE(called);
}

TEST_F(SettingsGroupTest, RegisterObserver)
{
	MockSettingsObserver observer;
	bool                 called = false;
	EXPECT_CALL(impl, RegisterObserver).WillOnce([&](ISettingsObserver* o) {
		called = true;
		EXPECT_EQ(o, &observer);
	});
	EXPECT_NO_THROW(settings->RegisterObserver(&observer));
	EXPECT_TRUE(called);
}

TEST_F(SettingsGroupTest, UnregisterObserver)
{
	MockSettingsObserver observer;
	bool                 called = false;
	EXPECT_CALL(impl, UnregisterObserver).WillOnce([&](ISettingsObserver* o) {
		called = true;
		EXPECT_EQ(o, &observer);
	});
	EXPECT_NO_THROW(settings->UnregisterObserver(&observer));
	EXPECT_TRUE(called);
}

struct SettingsDecoratorTest : testing::Test
{
	std::shared_ptr<testing::StrictMock<MockSettings>> impl     = std::make_shared<testing::StrictMock<MockSettings>>();
	const std::shared_ptr<ISettings>                   settings = SettingsFactory::CreateDecorator(
		impl,
		{
			{    "int",                  QVariant::fromValue(42) },
			{ "string", QVariant::fromValue(QString { "hello" }) },
	}
	);
};

TEST_F(SettingsDecoratorTest, Get)
{
	EXPECT_EQ(settings->Get("int").toInt(), 42);
	EXPECT_CALL(*impl, Get(QString { "key" }, QVariant {})).WillOnce([](const auto& key, const auto& defaultValue) {
		EXPECT_EQ(key, "key");
		EXPECT_FALSE(defaultValue.isValid());
		return QString { "rock" };
	});
	EXPECT_EQ(settings->Get("key").toString(), "rock");

	bool       called = false;
	const auto def    = [&]() {
		called = true;
		return QVariant::fromValue(42);
	};
	EXPECT_EQ(settings->Get("string", def).toString(), "hello");
	EXPECT_FALSE(called);

	EXPECT_CALL(*impl, Get(QString { "val" }, testing::Matcher<const std::function<QVariant()>&>(testing::_))).WillOnce([](const QString& key, const std::function<QVariant()>& defaultValueGetter) {
		EXPECT_EQ(key, "val");
		return defaultValueGetter();
	});
	EXPECT_EQ(settings->Get("val", def).toInt(), 42);
	EXPECT_TRUE(called);
}

TEST_F(SettingsDecoratorTest, Set)
{
	EXPECT_FALSE(settings->Set("int", 42));
	EXPECT_TRUE(settings->Set("string", QString { "bye" }));
	EXPECT_EQ(settings->Get("string").toString(), "bye");

	EXPECT_CALL(*impl, Set(QString { "double" }, QVariant { 42.0 }, true)).WillOnce([](const QString& key, const QVariant& value, const bool sync) {
		EXPECT_STREQ(key.toStdString().data(), "double");
		EXPECT_DOUBLE_EQ(value.toDouble(), 42.0);
		EXPECT_TRUE(sync);
		return true;
	});
	EXPECT_TRUE(settings->Set("double", 42.0));
}

TEST_F(SettingsDecoratorTest, HasKey)
{
	EXPECT_TRUE(settings->HasKey("int"));
	EXPECT_CALL(*impl, HasKey).WillOnce(testing::Return(true)).WillOnce(testing::Return(false));
	EXPECT_TRUE(settings->HasKey("key1"));
	EXPECT_FALSE(settings->HasKey("key2"));
}

TEST_F(SettingsDecoratorTest, HasGroup)
{
	EXPECT_CALL(*impl, HasGroup).WillOnce(testing::Return(false)).WillOnce(testing::Return(true));
	EXPECT_FALSE(settings->HasGroup("group1"));
	EXPECT_TRUE(settings->HasGroup("group2"));
}

TEST_F(SettingsDecoratorTest, GetKeys)
{
	EXPECT_CALL(*impl, GetKeys).WillOnce([](const auto& prefix) {
		EXPECT_EQ(prefix, "prefix");
		return QStringList { "1", "2", "3" };
	});
	const auto keys = settings->GetKeys("prefix");
	EXPECT_TRUE(std::ranges::all_of(QStringList { "1", "2", "3", "int", "string" }, [&keys](const auto& item) {
		return keys.contains(item);
	}));
}

TEST_F(SettingsDecoratorTest, GetGroups)
{
	EXPECT_CALL(*impl, GetGroups).WillOnce([](const auto& prefix) {
		EXPECT_EQ(prefix, "prefix");
		return QStringList { "a", "b", "w" };
	});
	const auto groups = settings->GetGroups("prefix");
	EXPECT_TRUE(std::ranges::all_of(QStringList { "a", "b", "w" }, [&groups](const auto& item) {
		return groups.contains(item);
	}));
}

TEST_F(SettingsDecoratorTest, CreateGroupSettings)
{
	EXPECT_TRUE(!!settings->CreateGroupSettings("group"));
}

TEST_F(SettingsDecoratorTest, Remove)
{
	bool called = false;
	EXPECT_CALL(*impl, Remove).WillOnce([&](const auto& key) {
		EXPECT_EQ(key, "int");
		called = true;
	});
	EXPECT_NO_THROW(settings->Remove("int"));
	EXPECT_CALL(*impl, HasKey).WillOnce(testing::Return(false));
	EXPECT_FALSE(settings->HasKey("int"));
	EXPECT_TRUE(called);
}

TEST_F(SettingsDecoratorTest, Save)
{
	bool called = false;
	EXPECT_CALL(*impl, Save).WillOnce([&](const QString& path, const QString& prefix) {
		EXPECT_EQ(path, "path");
		EXPECT_EQ(prefix, "prefix");
		called = true;
	});
	EXPECT_NO_THROW(settings->Save("path", "prefix"));
	EXPECT_TRUE(called);
}

TEST_F(SettingsDecoratorTest, Load)
{
	bool called = false;
	EXPECT_CALL(*impl, Load).WillOnce([&](const QString& path, const QString& prefix) {
		EXPECT_EQ(path, "path");
		EXPECT_EQ(prefix, "prefix");
		called = true;
	});
	EXPECT_NO_THROW(settings->Load("path", "prefix"));
	EXPECT_TRUE(called);
}

TEST_F(SettingsDecoratorTest, RegisterObserver)
{
	testing::StrictMock<MockSettingsObserver> observer;
	bool                                      called = false;
	EXPECT_CALL(*impl, RegisterObserver).WillOnce([&](ISettingsObserver* o) {
		EXPECT_EQ(o, static_cast<ISettingsObserver*>(&observer));
		called = true;
	});
	EXPECT_NO_THROW(settings->RegisterObserver(&observer));
	EXPECT_TRUE(called);
}

TEST_F(SettingsDecoratorTest, UnregisterObserver)
{
	testing::StrictMock<MockSettingsObserver> observer;
	bool                                      called = false;
	EXPECT_CALL(*impl, UnregisterObserver).WillOnce([&](ISettingsObserver* o) {
		EXPECT_EQ(o, static_cast<ISettingsObserver*>(&observer));
		called = true;
	});
	EXPECT_NO_THROW(settings->UnregisterObserver(&observer));
	EXPECT_TRUE(called);
}

struct SettingsTest : testing::Test
{
	std::unique_ptr<ISettings>            settings { SettingsFactory::CreateMemory("HomeCompa", "TestSettings") };
	std::unique_ptr<MockSettingsObserver> observer { std::make_unique<testing::StrictMock<MockSettingsObserver>>() };

	void Fill() const
	{
		EXPECT_TRUE(settings->Set("bool", true));
		EXPECT_TRUE(settings->Set("int", 42));
		EXPECT_TRUE(settings->Set("double", -17.0));
		EXPECT_TRUE(settings->Set("string", QString("hello")));
		EXPECT_TRUE(settings->Set("first/1", true));
		EXPECT_TRUE(settings->Set("first/2", false));
		EXPECT_TRUE(settings->Set("second/key/test", false));
	}
};

TEST_F(SettingsTest, Create)
{
	EXPECT_TRUE(!!settings);
}

TEST_F(SettingsTest, GetDefault)
{
	EXPECT_TRUE(settings->Get("bool", true));
	EXPECT_FALSE(settings->Get("bool", false));
	EXPECT_EQ(settings->Get("int", 42), 42);
	EXPECT_DOUBLE_EQ(settings->Get("double", -17.0), -17.0);
	EXPECT_EQ(settings->Get("string", QString("hello")), "hello");

	const auto def = []() {
		return QString("hello");
	};

	EXPECT_EQ(settings->Get("string", def), "hello");
	EXPECT_TRUE(settings->Set("string", "hi"));
	EXPECT_EQ(settings->Get("string", def), "hi");
}

TEST_F(SettingsTest, GetEnum)
{
	enum class MyEnum
	{
		Value    = 42,
		NotValue = 13,
	};
	Fill();
	EXPECT_EQ(settings->Get("int", MyEnum::NotValue), MyEnum::Value);
	EXPECT_EQ(settings->Get("int1", MyEnum::NotValue), MyEnum::NotValue);
}

TEST_F(SettingsTest, Set)
{
	Fill();
	EXPECT_FALSE(settings->Set("bool", true));
	EXPECT_TRUE(settings->Get("bool").toBool());

	EXPECT_TRUE(settings->Set("bool", false));
	EXPECT_FALSE(settings->Set("bool", false));
	EXPECT_FALSE(settings->Get("bool").toBool());

	EXPECT_FALSE(settings->Set("int", 42));
	EXPECT_EQ(settings->Get("int").toInt(), 42);

	EXPECT_FALSE(settings->Set("double", -17.0));
	EXPECT_DOUBLE_EQ(settings->Get("double").toDouble(), -17.0);

	EXPECT_FALSE(settings->Set("string", QString("hello")));
	EXPECT_EQ(settings->Get("string").toString(), "hello");

	EXPECT_TRUE(settings->Set("first/bool", true));
	EXPECT_TRUE(settings->Get("first/bool").toBool());
	EXPECT_FALSE(settings->Get("second/bool").toBool());
}

TEST_F(SettingsTest, Keys)
{
	Fill();

	EXPECT_TRUE(settings->HasKey("bool"));
	EXPECT_TRUE(settings->HasKey("int"));
	EXPECT_TRUE(settings->HasKey("double"));
	EXPECT_TRUE(settings->HasKey("string"));
	EXPECT_TRUE(settings->HasKey("first/1"));
	EXPECT_TRUE(settings->HasKey("first/2"));
	EXPECT_TRUE(settings->HasKey("second/key/test"));
	EXPECT_FALSE(settings->HasKey("first"));
	EXPECT_FALSE(settings->HasKey("second"));
	EXPECT_FALSE(settings->HasKey("second/key"));

	EXPECT_TRUE(std::ranges::equal(settings->GetKeys() | std::views::common | std::ranges::to<std::set>(), std::set<QString> { "bool", "int", "double", "string" }));
	EXPECT_TRUE(std::ranges::equal(settings->GetKeys("first") | std::views::common | std::ranges::to<std::set>(), std::set<QString> { "1", "2" }));
	EXPECT_TRUE(settings->GetKeys("second").isEmpty());
	EXPECT_EQ(settings->GetKeys("second/key"), QStringList { "test" });
}

TEST_F(SettingsTest, Groups)
{
	Fill();

	EXPECT_FALSE(settings->HasGroup("bool"));
	EXPECT_FALSE(settings->HasGroup("int"));
	EXPECT_FALSE(settings->HasGroup("double"));
	EXPECT_FALSE(settings->HasGroup("string"));
	EXPECT_FALSE(settings->HasGroup("first/1"));
	EXPECT_FALSE(settings->HasGroup("first/2"));
	EXPECT_FALSE(settings->HasGroup("second/key/test"));
	EXPECT_TRUE(settings->HasGroup("first"));
	EXPECT_TRUE(settings->HasGroup("second"));
	EXPECT_FALSE(settings->HasGroup("second/key"));

	EXPECT_TRUE(std::ranges::equal(settings->GetGroups() | std::views::common | std::ranges::to<std::set>(), std::set<QString> { "first", "second" }));
	EXPECT_TRUE(settings->GetGroups("kiss").isEmpty());
	EXPECT_TRUE(settings->GetGroups("first").isEmpty());
	EXPECT_EQ(settings->GetGroups("second"), QStringList { "key" });
	EXPECT_TRUE(settings->GetGroups("second/key").isEmpty());
}

TEST_F(SettingsTest, Remove)
{
	Fill();

	EXPECT_TRUE(settings->HasKey("bool"));
	EXPECT_NO_THROW(settings->Remove("bool"));
	EXPECT_FALSE(settings->HasKey("bool"));

	EXPECT_TRUE(settings->HasGroup("first"));
	EXPECT_NO_THROW(settings->Remove("first"));
	EXPECT_FALSE(settings->HasGroup("first"));

	EXPECT_TRUE(settings->HasKey("second/key/test"));
	EXPECT_NO_THROW(settings->Remove("second"));
	EXPECT_FALSE(settings->HasKey("second/key/test"));
}

TEST_F(SettingsTest, GroupSettingsConst)
{
	Fill();
	ASSERT_TRUE(settings->Get("first/1").toBool());

	const std::unique_ptr<const ISettings> settingsConst = std::move(settings);
	const auto                             group         = settingsConst->CreateGroupSettings("first");
	EXPECT_TRUE(group->Get("1").toBool());
	EXPECT_FALSE(group->Get("2").toBool());
	EXPECT_TRUE(std::ranges::equal(group->GetKeys() | std::views::common | std::ranges::to<std::set>(), std::set<QString> { "1", "2" }));
	EXPECT_TRUE(group->GetGroups().isEmpty());
}

TEST_F(SettingsTest, GroupSettings)
{
	Fill();
	ASSERT_TRUE(settings->Get("first/1").toBool());

	{
		const auto group = settings->CreateGroupSettings("first");
		EXPECT_TRUE(group->Get("1").toBool());
		EXPECT_FALSE(group->Get("2").toBool());
		EXPECT_TRUE(group->Set("1", false));
		EXPECT_FALSE(group->Set("2", false));
		EXPECT_FALSE(settings->Get("first/1").toBool());
		EXPECT_TRUE(std::ranges::equal(group->GetKeys() | std::views::common | std::ranges::to<std::set>(), std::set<QString> { "1", "2" }));
		EXPECT_TRUE(group->GetGroups().isEmpty());
		group->Remove("1");
		EXPECT_FALSE(group->HasKey("1"));
		EXPECT_FALSE(settings->HasKey("first/1"));
	}
	{
		const auto group = settings->CreateGroupSettings("second");
		EXPECT_TRUE(group->HasGroup("key"));
		EXPECT_TRUE(group->HasKey("key/test"));
	}
}

TEST_F(SettingsTest, GroupSettings2)
{
	Fill();
	ASSERT_FALSE(settings->Get("second/key/test").toBool());
	ASSERT_TRUE(settings->Set("second/key/test", true));
	ASSERT_TRUE(settings->Get("second/key/test").toBool());

	const auto group = settings->CreateGroupSettings("second");
	EXPECT_EQ(group->GetGroups(), QStringList { "key" });

	const auto group2 = group->CreateGroupSettings("key");
	EXPECT_TRUE(group2->Get("test").toBool());
	EXPECT_TRUE(group2->Set("test", false));
	EXPECT_FALSE(settings->Get("second/key/test").toBool());
	EXPECT_FALSE(group->Get("key/test").toBool());
}

TEST_F(SettingsTest, GroupSettings3)
{
	Fill();
	const auto group = settings->CreateGroupSettings("first");
	EXPECT_EQ(
		group
			->Get(
				"42",
				[]() {
					return QVariant(QString("rock!"));
				}
			)
			.toString(),
		"rock!"
	);
}

TEST_F(SettingsTest, GroupSettings4)
{
	Fill();
	const auto group = settings->CreateGroupSettings("");
	EXPECT_TRUE(group->HasGroup("first"));
}

TEST_F(SettingsTest, GroupSettingsMultiThreading)
{
	Fill();
	std::atomic_int              value;
	const unsigned               threadCount = std::thread::hardware_concurrency() * 2;
	static constexpr const char* groups[]    = { "first", "first/tag", "second" };

	const auto work = [&](const std::stop_token stop, const unsigned n) {
		while (!stop.stop_requested())
		{
			const auto group = settings->CreateGroupSettings(groups[value % std::size(groups)]);
			group->Set("value", n & 1 ? value.fetch_add(1) : value.fetch_sub(1));
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
	};

	std::vector<std::jthread> threads;
	threads.reserve(threadCount);
	for (unsigned i = 0; i < threadCount; ++i)
		threads.emplace_back(work, i);

	std::this_thread::sleep_for(std::chrono::seconds(5));
	threads.clear();
}

TEST_F(SettingsTest, Observer)
{
	Fill();

	bool called = false;
	EXPECT_CALL(*observer, HandleValueChanged).WillOnce([&](const QString& key, const QVariant& value) {
		called = true;
		EXPECT_EQ(key, "bool");
		EXPECT_EQ(value.toBool(), false);
	});
	settings->RegisterObserver(observer.get());
	EXPECT_FALSE(settings->Set("bool", true));
	EXPECT_FALSE(called);

	EXPECT_TRUE(settings->Set("bool", false));
	EXPECT_TRUE(called);

	settings->UnregisterObserver(observer.get());
	called = false;
	EXPECT_FALSE(settings->Set("bool", false));
	EXPECT_FALSE(called);

	EXPECT_TRUE(settings->Set("bool", true));
	EXPECT_FALSE(called);
}

TEST_F(SettingsTest, LoadFail)
{
	EXPECT_DEBUG_DEATH(settings->Load("bad_file_name:"), "");
}

TEST_F(SettingsTest, SaveLoad)
{
	Fill();
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const auto fileName = dir.filePath("settings.ini");
	ASSERT_NO_THROW(settings->Save(fileName));

	const auto copy = SettingsFactory::CreateMemory("HomeCompa", "TestSettings1");
	ASSERT_NO_THROW(copy->Load(fileName));

	EXPECT_TRUE(copy->Get("bool").toBool());
	EXPECT_EQ(copy->Get("int").toInt(), 42);
	EXPECT_DOUBLE_EQ(copy->Get("double").toDouble(), -17.0);
	EXPECT_EQ(copy->Get("string").toString(), "hello");
	EXPECT_TRUE(copy->Get("first/1").toBool());
	EXPECT_FALSE(copy->Get("first/2").toBool());
	EXPECT_FALSE(copy->Get("second/key/test").toBool());
}

TEST_F(SettingsTest, SaveLoadWithPrefix)
{
	Fill();
	QTemporaryDir dir;
	ASSERT_TRUE(dir.isValid());

	const auto fileName = dir.filePath("settings.ini");
	ASSERT_NO_THROW(settings->Save(fileName, "first"));

	const auto copy = SettingsFactory::CreateMemory("HomeCompa", "TestSettings1");
	ASSERT_NO_THROW(copy->Load(fileName, "prefix"));

	EXPECT_FALSE(copy->Get("1").toBool());
	EXPECT_FALSE(copy->Get("2").toBool());

	EXPECT_TRUE(copy->Get("prefix/1").toBool());
	EXPECT_FALSE(copy->Get("prefix/2").toBool());
}

} // namespace
