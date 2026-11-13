#include "zip.h"

#include <ranges>

#include <QBuffer>
#include <QFile>

#include <gtest/gtest.h>

using namespace HomeCompa;

namespace {

struct DecodeTest : testing::TestWithParam<QString>
{
};

INSTANTIATE_TEST_SUITE_P(DecodeTests, DecodeTest, testing::Values("archive.7z", "archive.gz", "archive.tar", "archive.wim", "archive.zip"));

TEST_P(DecodeTest, Read)
{
	const auto input = [] {
		QFile src(":/src/input.txt");
		return src.open(QIODevice::ReadOnly) ? src.readAll() : QByteArray {};
	}();
	ASSERT_FALSE(input.isEmpty());

	const auto test = [&] {
		const auto file = GetParam();
		QFile      stream(QString(":/dst/%1").arg(file));
		std::ignore = stream.open(QIODevice::ReadOnly);
		const Zip  zip(stream);
		const auto files = zip.GetFileNameList();
		ASSERT_EQ(files.size(), 1);
		const auto file1 = files.front().toStdString();
		const auto fileStr = file1.data();
		ASSERT_STREQ(fileStr, "input.txt");
		EXPECT_EQ(zip.Read(files.front())->GetStream().readAll(), input);
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
	QBuffer    buffer;
	buffer.open(QIODevice::WriteOnly);
	const auto test = [&] {
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
