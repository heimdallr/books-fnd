#include "zip.h"

#include <ranges>

#include <QBuffer>

#include <gtest/gtest.h>

using namespace HomeCompa;

namespace {

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
	}

	QBuffer buffer(&data);
	buffer.open(QIODevice::ReadOnly);
	Zip        zip(buffer);
	const auto unzippedFileNames = zip.GetFileNameList();
	EXPECT_EQ(unzippedFileNames.size(), files.size());
	for (const auto& [file, unzippedFileName] : std::views::zip(files, unzippedFileNames))
	{
		EXPECT_EQ(file.first, unzippedFileName);
		EXPECT_EQ(file.second, zip.Read(unzippedFileName)->GetStream().readAll());
	}
}

struct ZipTest : testing::TestWithParam<Zip::Format>
{
};

struct ZipEmptyTest : ZipTest
{
};

INSTANTIATE_TEST_SUITE_P(ZipEmptyTests, ZipEmptyTest, testing::Values(Zip::Format::Zip, Zip::Format::SevenZip));

TEST_P(ZipEmptyTest, Create)
{
	const auto format = GetParam();
	Process(format);
}

struct ZipSingleFileTest : ZipTest
{
};

INSTANTIATE_TEST_SUITE_P(ZipSingleFileTests, ZipSingleFileTest, testing::Values(Zip::Format::Zip, Zip::Format::SevenZip, Zip::Format::GZip));

TEST_P(ZipSingleFileTest, Create)
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
