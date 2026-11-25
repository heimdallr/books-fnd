#include <QCoreApplication>
#include <QString>

#include <gtest/gtest.h>

#include "icu/icu.h"

using namespace HomeCompa::ICU;

int main(int argc, char* argv[])
{
	[[maybe_unused]] const QCoreApplication app(argc, argv);
	testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

namespace {

TEST(ICU, Main)
{
	SUCCEED();
}

TEST(ICU, TransliterateFail)
{
	std::u32string src { U"fail" };
	std::u32string dst;
	EXPECT_FALSE(ICU_Transliterate("fake stub id", &src, &dst));
}


TEST(ICU, CreateDecoderFail)
{
	EXPECT_THROW(IDecoder::Create("fake stub encoding"), std::runtime_error);
}

TEST(ICU, DecodeFail)
{
	const auto decoder = IDecoder::Create("UTF-8", true);
	ASSERT_TRUE(!!decoder);
	EXPECT_THROW(decoder->Decode("\x80"), std::runtime_error);
	const auto decoded = decoder->Decode("Hello 123!");
	EXPECT_EQ(decoded, "Hello 123!");
}

} // namespace
