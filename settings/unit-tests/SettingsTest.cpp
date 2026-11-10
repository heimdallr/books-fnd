#include "settings/Settings.h"

#include <ranges>
#include <thread>

#include <gtest/gtest.h>

#include "mock/settings/MockSettingsObserver.h"
#include "settings/ISettings.h"

using namespace HomeCompa::Test;
using namespace HomeCompa;

namespace {

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
	EXPECT_EQ(
		settings->Get(
			"string",
			[]() {
				return QString("hello");
			}
		),
		"hello"
	);
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
	EXPECT_CALL(*observer, HandleValueChanged).WillOnce([](const QString& key, const QVariant& value) {
		EXPECT_EQ(key, "bool");
		EXPECT_EQ(value.toBool(), false);
	});
	settings->RegisterObserver(observer.get());
	EXPECT_FALSE(settings->Set("bool", true));
	EXPECT_TRUE(settings->Set("bool", false));
}

} // namespace
