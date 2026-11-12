#include "zip.h"

#include "src/ext/zip/7z/bit7z/include/bit7z/bit7zlibrary.hpp"
#include <ranges>

#include <QBuffer>

#include <gtest/gtest.h>

using namespace HomeCompa;

namespace {

TEST(Bit7z, Create)
{
    const auto test = []{
        bit7z::Bit7zLibrary lib;
        lib.setLargePageMode();
    };
    EXPECT_NO_THROW(test());
}

void Process(const Zip::Format format, const std::vector<std::pair<QString, QByteArray>>& files = {})
{
	QByteArray data;
    {
		QBuffer buffer(&data);
        buffer.open(QIODevice::WriteOnly);
        Zip        zip(buffer, format);
        const auto zipFiles = Zip::CreateZipFileController();
        for (const auto& [name, body] : files)
			zipFiles->AddFile(name, body);
        ASSERT_NO_THROW(zip.Write(*zipFiles));
        std::cerr << 8 << std::endl;
    }
    std::cerr << 9 << std::endl;

	QBuffer buffer(&data);
    std::cerr << 10 << std::endl;
    buffer.open(QIODevice::ReadOnly);
    std::cerr << 11 << std::endl;
    Zip        zip(buffer);
    std::cerr << 12 << std::endl;
    const auto unzippedFileNames = zip.GetFileNameList();
    std::cerr << 13 << std::endl;
    EXPECT_EQ(unzippedFileNames.size(), files.size());
    std::cerr << 14 << std::endl;
    for (const auto& [file, unzippedFileName] : std::views::zip(files, unzippedFileNames))
	{
        std::cerr << 15 << std::endl;
        EXPECT_EQ(file.first, unzippedFileName);
        std::cerr << 16 << std::endl;
        EXPECT_EQ(file.second, zip.Read(unzippedFileName)->GetStream().readAll());
        std::cerr << 17 << std::endl;
    }
    std::cerr << 18 << std::endl;
}

struct ZipTest : testing::TestWithParam<Zip::Format>
{
};

struct ZipCreateTest : ZipTest
{
};

INSTANTIATE_TEST_SUITE_P(ZipEmptyTests, ZipCreateTest, testing::Values(Zip::Format::Zip, Zip::Format::SevenZip, Zip::Format::BZip2, Zip::Format::GZip, Zip::Format::Tar, Zip::Format::Xz));

struct ZipEmptyTest : ZipTest
{
};

TEST_P(ZipCreateTest, JustCreate)
{
    const auto format = GetParam();
    QBuffer buffer;
    buffer.open(QIODevice::WriteOnly);
    const auto test = [&]{
        [[maybe_unused]] const Zip zip(buffer, format);
    };
    EXPECT_NO_THROW(test());
}

INSTANTIATE_TEST_SUITE_P(ZipEmptyTests, ZipEmptyTest, testing::Values(Zip::Format::Zip, Zip::Format::SevenZip));

TEST_P(ZipEmptyTest, ReadWrite)
{
	const auto format = GetParam();
	Process(format);
}

struct ZipSingleFileTest : ZipTest
{
};

INSTANTIATE_TEST_SUITE_P(ZipSingleFileTests, ZipSingleFileTest, testing::Values(Zip::Format::Zip, Zip::Format::SevenZip, Zip::Format::GZip));

TEST_P(ZipSingleFileTest, ReadWrite)
{
	const auto format = GetParam();
	Process(
		format,
		{
			{ "12345qwert6", "data.dat" }
    }
	);
}

} // namespace
