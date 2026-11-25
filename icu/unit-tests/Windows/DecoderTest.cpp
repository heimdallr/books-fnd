#include <gtest/gtest.h>

#include "icu/icu.h"

using namespace HomeCompa::ICU;

namespace {

struct Test
{
	const char*      id;
	std::string_view source;
	std::string      test;
};

struct DecoderTest : testing::TestWithParam<Test>
{
};

INSTANTIATE_TEST_SUITE_P(
	DecoderTests,
	DecoderTest,
	testing::Values(
		Test { "UTF-8", "", "" },
		Test { "ISO-8859-1", "Hello 123!", "Hello 123!" },
		Test { "windows-1251", "\xCF\xF0\xE8\xE2\xE5\xF2", "Привет" },
		Test { "KOI8-R", "\xF0\xD2\xC9\xD7\xC5\xD4", "Привет" },
		Test { "windows-1256", "\xE3\xD1\xCD\xC8\xC7", "مرحبا" },
		Test { "windows-1255", "\xF9\xEC\xE5\xED", "שלום" },
		Test { "ISO-8859-1", "\xC4\xD6\xDC", "ÄÖÜ" },
		Test { "Shift_JIS", "\x82\xB1\x82\xF1\x82\xC9\x82\xBF\x82\xCD", "こんにちは" },
		Test { "GB18030", "\xC4\xE3\xBA\xC3", "你好" },
		Test { "EUC-KR", "\xBE\xC8\xB3\xE7", "안녕" },
		Test { "UTF-16LE", "\x1F\x04\x40\x04", "Пр" },
		Test { "UTF-16BE", "\x04\x1F\x04\x40", "Пр" },
		Test { "UTF-8", "\xF0\x9F\x98\x80", "😀" },
		Test { "ISO-2022-JP", "\x1B$B$3$s$K$A$O\x1B(B", "こんにちは" }
	)
);

TEST_P(DecoderTest, Transliterate)
{
	const auto& [id, str, test] = GetParam();

	const auto decoder = IDecoder::Create(id);
	ASSERT_TRUE(!!decoder);
	const auto decoded = decoder->Decode(str);
	EXPECT_EQ(decoded, test);
}

} // namespace
