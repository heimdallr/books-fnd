#include <QString>

#include <gtest/gtest.h>

#include "icu/icu.h"

namespace {

struct Test
{
	const char* id;
	QString     source;
	QString     test;
};

struct TransliterateTest : testing::TestWithParam<Test>
{
};

INSTANTIATE_TEST_SUITE_P(
	TransliterateTests,
	TransliterateTest,
	testing::Values(
		Test { "Any-Latin", "a", "a" },
		Test { "Any-Latin", "abba", "abba" },
		Test { "ru-ru_Latn/BGN", "мама", "mama" },
		Test { "ru-ru_Latn/BGN; Latin-ASCII", "съешь ещё этих мягких французских булок да выпей чаю", R"(s"yesh' yeshche etikh myagkikh frantsuzskikh bulok da vypey chayu)" },
		Test { "Arab-Latin; Latin-ASCII", "السلام", "alslam" },
		Test { "Hebr-Latin; Latin-ASCII", "שלום", "slwm" },
		Test { "Han-Latin; Latin-ASCII", "你好", "ni hao" },
		Test { "Any-Latin; Latin-ASCII", "ありがとう", "arigatou" }
	)
);

TEST_P(TransliterateTest, Transliterate)
{
	const auto& [id, str, test] = GetParam();

	auto src = str.toStdU32String();
	src.push_back(0);
	std::u32string dst;
	EXPECT_TRUE(ICU_Transliterate(id, &src, &dst));

	const auto transliterated = QString::fromStdU32String(dst);
	EXPECT_EQ(transliterated, test);
}

} // namespace
