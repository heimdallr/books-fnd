#include "util/ColorUtil.h"

#include <QColor>
#include <QString>
#include <QVariant>
#include <QGuiApplication>

#include <gtest/gtest.h>

#include "mock/settings/MockSettings.h"

using namespace HomeCompa::Test;
using namespace HomeCompa::Util;

namespace {

constexpr auto KEY = "key";

struct ColorUtil_GetColor : testing::Test
{
	const testing::StrictMock<MockSettings> settings;
};

TEST_F(ColorUtil_GetColor, NoKey)
{
	EXPECT_CALL(settings, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce(testing::Return(QVariant {}));
	const auto color = GetColor(settings, KEY);
	EXPECT_FALSE(color.has_value());
}

TEST_F(ColorUtil_GetColor, BadValue)
{
	EXPECT_CALL(settings, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce(testing::Return(QVariant { "Bad value" }));
	const auto color = GetColor(settings, KEY);
	EXPECT_FALSE(color.has_value());
}

TEST_F(ColorUtil_GetColor, Transparent)
{
	EXPECT_CALL(settings, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce(testing::Return(QVariant { 0xFF000000 }));
	const auto color = GetColor(settings, KEY);
	EXPECT_TRUE(color.has_value());
	EXPECT_TRUE(color->isValid());
	EXPECT_EQ(*color, QColorConstants::Transparent);
}

struct ColorUtil_GetColorTest
    : ColorUtil_GetColor
    , testing::WithParamInterface<std::tuple<uint32_t, int, int, int, int>>
{
};

INSTANTIATE_TEST_SUITE_P(
	ColorUtil_GetColorTests,
	ColorUtil_GetColorTest,
	testing::Values(std::make_tuple(0x42, 0x42, 0, 0, 0), std::make_tuple(0x4200, 0, 0x42, 0, 0), std::make_tuple(0x420000, 0, 0, 0x42, 0), std::make_tuple(0x04030201, 1, 2, 3, 4))
);

TEST_P(ColorUtil_GetColorTest, Get)
{
	const auto [value, r, g, b, a] = GetParam();
	EXPECT_CALL(settings, Get(testing::_, testing::Matcher<const QVariant&>(testing::_))).WillOnce(testing::Return(QVariant { value }));
	const auto color = GetColor(settings, KEY);
	EXPECT_TRUE(color.has_value());
	QColor test(r, g, b, 255 - a);
	EXPECT_EQ(*color, test);
}

struct ColorUtil_ToString : testing::TestWithParam<std::pair<QColor, QString>>
{
};

INSTANTIATE_TEST_SUITE_P(
	ColorUtil_ToStrings,
	ColorUtil_ToString,
	testing::Values(
		std::make_pair(QColor(Qt::black), QString { "#000000" }),
		std::make_pair(QColor(Qt::red), QString { "#FF0000" }),
		std::make_pair(QColor(Qt::green), QString { "#00FF00" }),
		std::make_pair(QColor(Qt::blue), QString { "#0000FF" })
	)
);

TEST_P(ColorUtil_ToString, ToString)
{
	const auto& [color, test] = GetParam();
	const auto value          = ToString(color);
	EXPECT_EQ(value.toUpper(), test);
}


TEST(ColorUtil_ToString2, ToString)
{
	const auto palette = QGuiApplication::palette();
	for (int role = QPalette::WindowText; role < QPalette::NColorRoles; ++role)
	{
		const auto value1 = ToString(palette, static_cast<QPalette::ColorRole>(role));
		const auto value2 = ToString(palette.color(static_cast<QPalette::ColorRole>(role)));
		EXPECT_EQ(value1.toUpper(), value2.toUpper());
	}
}

} // namespace
