#include "util/StrUtil.h"

#include <gtest/gtest.h>

using namespace HomeCompa::Util;

namespace {

struct Item
{
	QString source;
	std::vector<QString> result;
};

struct UniqTitleTest : testing::TestWithParam<Item>
{
};

INSTANTIATE_TEST_SUITE_P(
	UniqTitleTests,
	UniqTitleTest,
	testing::Values(
		Item {
},
		Item { " ", {} },
		Item { "abc", { "abc" } },
		Item { "abc ", { "abc" } },
		Item { " abc", { "abc" } },
		Item { " abc ", { "abc" } },
		Item { " abc abd, aec .", { "abc", "abd", "aec" } },
		Item { "abc\tabd", { "abc", "abd" } },
		Item { "abc15", { "abc15" } },
		Item { "abc 15", { "abc", "15" } },
		Item { "42 abc 15", { "42", "abc", "15" } },
		Item { "ёйъ", { "еиь" } }
	)
);

TEST_P(UniqTitleTest, Test)
{
	const auto& [src, test] = GetParam();
	const auto values = UniqTitle(src);
	EXPECT_EQ(values, test);
}

TEST(AppendTitleTest, Test)
{
	QString str;
	AppendTitle(str, "a");
	EXPECT_EQ(str, "a");
	AppendTitle(str, "");
	EXPECT_EQ(str, "a");
	AppendTitle(str, "b");
	EXPECT_EQ(str, "a b");
	AppendTitle(str, "", ",");
	EXPECT_EQ(str, "a b");
	AppendTitle(str, "c", ",");
	EXPECT_EQ(str, "a b,c");
}

} // namespace
