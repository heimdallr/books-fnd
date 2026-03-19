#include "archive.h"

#include <fstream>
#include <ranges>
#include <thread>

#include "interface/types.h"

#include "bit7z/bit7zlibrary.hpp"
#include "bit7z/bitarchivereader.hpp"
#include "bit7z/bitformat.hpp"
#include "bit7z/bitpropvariant.hpp"
#include "zip/interface/error.h"
#include "zip/interface/file.h"
#include "zip/interface/format.h"
#include "zip/interface/zip.h"

#include "FileItem.h"
#include "log.h"
#include "reader.h"

namespace HomeCompa::ZipDetails::SevenZip
{

namespace
{

#ifdef _WIN32
	#define fromBit7zString fromStdWString
#else
	#define fromBit7zString fromStdString
#endif

std::optional<qint64> GetTime(const bit7z::BitInputArchive& archive, const uint32_t index, const bit7z::BitProperty property)
{
	static constexpr auto NANO100_IN_SECOND         = 10000000;
	static constexpr auto SECONDS_FROM_1601_TO_1970 = 11644473600LL;

	if (const auto prop = archive.itemProperty(index, property); !prop.isEmpty())
		if (auto fileTime = prop.getFileTime(); fileTime.dwHighDateTime != 0 || fileTime.dwLowDateTime != 0)
			return ((static_cast<qint64>(fileTime.dwHighDateTime) << 32) | fileTime.dwLowDateTime) / NANO100_IN_SECOND - SECONDS_FROM_1601_TO_1970;

	return std::nullopt;
}

FileStorage CreateFileList(const bit7z::BitInputArchive& archive)
{
	FileStorage result;

	const auto numItems = archive.itemsCount();

	result.files.reserve(numItems);

	for (std::remove_const_t<decltype(numItems)> i = 0; i < numItems; ++i)
	{
		const auto size = archive.itemProperty(i, bit7z::BitProperty::Size).getUInt64();

		auto time = [&]() -> QDateTime {
			auto fileTime = GetTime(archive, i, bit7z::BitProperty::CTime);
			if (!fileTime)
			{
				fileTime = GetTime(archive, i, bit7z::BitProperty::MTime);
				if (!fileTime)
				{
					fileTime = GetTime(archive, i, bit7z::BitProperty::ATime);
					if (!fileTime)
						return {};
				}
			}

			return QDateTime::fromSecsSinceEpoch(*fileTime);
		}();

		auto fileName = QDir::fromNativeSeparators(QString::fromBit7zString(archive.itemProperty(i, bit7z::BitProperty::Path).getString()));
		if (const auto it = result.index.try_emplace(fileName, result.files.size()); it.second)
			result.files.emplace_back(i, std::move(fileName), size, std::move(time), archive.isItemFolder(i));
		else
			PLOGW << "something strange: " << fileName << " duplicates";
	}

	return result;
}

class Reader : virtual public IZip
{
public:
	explicit Reader(std::shared_ptr<ProgressCallback> progress)
		: m_progress(std::move(progress))
	{
	}

private: // IZip
	void SetProperty(const PropertyId id, QVariant value) override
	{
		m_properties[id] = std::move(value);
	}

	QStringList GetFileNameList() const override
	{
		return m_files.files | std::views::filter([](const auto& item) {
				   return !item.isDir;
			   })
		     | std::views::transform([](const auto& item) {
				   return item.name;
			   })
		     | std::ranges::to<QStringList>();
	}

	std::unique_ptr<IFile> Read(const QString& filename) const override
	{
		return File::Read(*m_archive, m_files.GetFile(filename));
	}

	bool Write(std::shared_ptr<IZipFileProvider> /*zipFileProvider*/) override
	{
		assert(false && "Cannot write with reader");
		return false;
	}

	bool Remove(const std::vector<QString>& /*fileNames*/) override
	{
		assert(false && "Cannot remove with reader");
		return false;
	}

	size_t GetFileSize(const QString& filename) const override
	{
		return m_files.GetFile(filename).size;
	}

	const QDateTime& GetFileTime(const QString& filename) const override
	{
		return m_files.GetFile(filename).time;
	}

protected:
	bit7z::Bit7zLibrary                      m_lib;
	std::unique_ptr<bit7z::BitArchiveReader> m_archive;
	FileStorage                              m_files;
	std::shared_ptr<ProgressCallback>        m_progress;
	std::unordered_map<PropertyId, QVariant> m_properties {
		{ PropertyId::CompressionLevel,               QVariant::fromValue(CompressionLevel::Ultra) },
		{     PropertyId::ThreadsCount, static_cast<uint32_t>(std::thread::hardware_concurrency()) },
	};
};

class ReaderFile : public Reader
{
public:
	ReaderFile(QString filename, std::shared_ptr<ProgressCallback> progress)
		: Reader(std::move(progress))
		, m_filename { std::move(filename) }
		, m_stream(m_filename.toStdString(), std::ios::binary)
	{
		m_archive = std::make_unique<bit7z::BitArchiveReader>(m_lib, m_stream);
		m_files   = CreateFileList(*m_archive);
	}

protected:
	const QString m_filename;
	std::ifstream m_stream;
};

class ReaderStream : public Reader
{
public:
	ReaderStream(QIODevice& stream, std::shared_ptr<ProgressCallback> progress)
		: Reader(std::move(progress))
	{
		if (stream.isReadable())
		{
			m_bytes.resize(stream.size());
			stream.read(reinterpret_cast<char*>(m_bytes.data()), static_cast<qint64>(m_bytes.size()));
		}
		m_archive = std::make_unique<bit7z::BitArchiveReader>(m_lib, m_bytes);
		m_files   = CreateFileList(*m_archive);
	}

protected:
	std::vector<bit7z::byte_t> m_bytes;
};

class WriterFile final : public ReaderFile
{
public:
	WriterFile(QString filename, const Format format, std::shared_ptr<ProgressCallback> progress, const bool /*appendMode*/)
		: ReaderFile(std::move(filename), std::move(progress))
		, m_format { format }
		, m_ioDevice { std::make_unique<QFile>(m_filename) }
	{
	}

private: // IZip
	bool Write(std::shared_ptr<IZipFileProvider> /*zipFileProvider*/) override
	{
		//		if (!m_archive->archive)
		//			SetArchiveProperties(*m_outArchive, GetInOutFormat(m_format), m_properties);
		//		return File::Write(m_files, *m_outArchive, *m_ioDevice, std::move(zipFileProvider), *m_progress);
		return true;
	}

	bool Remove(const std::vector<QString>& /*fileNames*/) override
	{
		//		const auto n      = m_files.files.size();
		//		const auto result = File::Remove(m_files, *m_outArchive, *m_ioDevice, fileNames, *m_progress);
		//		PLOGI << m_filename << ". Files removed: " << n - m_files.files.size() << " out of " << n;
		//		return result;
		return true;
	}

private:
	const Format               m_format;
	std::unique_ptr<QIODevice> m_ioDevice;
	//	CMyComPtr<IOutArchive>     m_outArchive;
};

class WriterStream final : public ReaderStream
{
public:
	WriterStream(QIODevice& stream, const Format format, std::shared_ptr<ProgressCallback> progress, const bool /*appendMode*/)
		: ReaderStream(stream, std::move(progress))
		, m_format { format }
		, m_ioDevice { stream }
	{
	}

private: // IZip
	bool Write(std::shared_ptr<IZipFileProvider> /*zipFileProvider*/) override
	{
		//		if (!m_archive->archive)
		//			SetArchiveProperties(*m_outArchive, GetInOutFormat(m_format), m_properties);
		//		return File::Write(m_files, *m_outArchive, m_ioDevice, std::move(zipFileProvider), *m_progress);
		return true;
	}

	bool Remove(const std::vector<QString>& /*fileNames*/) override
	{
		//		const auto n      = m_files.files.size();
		//		const auto result = File::Remove(m_files, *m_outArchive, m_ioDevice, fileNames, *m_progress);
		//		PLOGI << "files removed: " << n - m_files.files.size() << " out of " << n;
		//		return result;
		return true;
	}

private:
	const Format m_format;
	QIODevice&   m_ioDevice;
	//	CMyComPtr<IOutArchive> m_outArchive;
};

} // namespace

std::unique_ptr<IZip> Archive::CreateReader(const QString& filename, std::shared_ptr<ProgressCallback> progress)
{
	return std::make_unique<ReaderFile>(filename, std::move(progress));
}

std::unique_ptr<IZip> Archive::CreateReaderStream(QIODevice& stream, std::shared_ptr<ProgressCallback> progress)
{
	return std::make_unique<ReaderStream>(stream, std::move(progress));
}

std::unique_ptr<IZip> Archive::CreateWriter(const QString& filename, const Format format, std::shared_ptr<ProgressCallback> progress, bool appendMode)
{
	return std::make_unique<WriterFile>(filename, format, std::move(progress), appendMode);
}

std::unique_ptr<IZip> Archive::CreateWriterStream(QIODevice& stream, Format format, std::shared_ptr<ProgressCallback> progress, bool appendMode)
{
	return std::make_unique<WriterStream>(stream, format, std::move(progress), appendMode);
}

bool Archive::IsArchive(const QString& /*filename*/)
{
	//	return bit7z::detect_format_from_extension(filename.toStdWString()) != bit7z::BitFormat::Auto;
	return false;
}

QStringList Archive::GetTypes()
{
	return QStringList {} << "7z"
	                      << "zip"
	                      << "rar"
	                      << "bzip2"
	                      << "bz2"
	                      << "tbz2"
	                      << "tbz"
	                      << "gz"
	                      << "gzip"
	                      << "tgz"
	                      << "tar"
	                      << "ova"
	                      << "wim"
	                      << "swm"
	                      << "xz"
	                      << "txz"
	                      << "zipx"
	                      << "jar"
	                      << "xpi"
	                      << "odt"
	                      << "ods"
	                      << "odp"
	                      << "docx"
	                      << "xlsx"
	                      << "pptx"
	                      << "epub"
	                      << "001"
	                      << "ar"
	                      << "deb"
	                      << "apm"
	                      << "arj"
	                      << "cab"
	                      << "chm"
	                      << "chi"
	                      << "msi"
	                      << "doc"
	                      << "xls"
	                      << "ppt"
	                      << "msg"
	                      << "obj"
	                      << "cpio"
	                      << "cramfs"
	                      << "dmg"
	                      << "dll"
	                      << "exe"
	                      << "dylib"
	                      << "ext"
	                      << "ext2"
	                      << "ext3"
	                      << "ext4"
	                      << "fat"
	                      << "flv"
	                      << "gpt"
	                      << "hfs"
	                      << "hfsx"
	                      << "hxs"
	                      << "ihex"
	                      << "lzh"
	                      << "lha"
	                      << "lzma"
	                      << "lzma86"
	                      << "mbr"
	                      << "mslz"
	                      << "mub"
	                      << "nsis"
	                      << "ntfs"
	                      << "pmd"
	                      << "ppmd"
	                      << "qcow"
	                      << "qcow2"
	                      << "qcow2c"
	                      << "rpm"
	                      << "squashfs"
	                      << "swf"
	                      << "te"
	                      << "udf"
	                      << "scap"
	                      << "uefif"
	                      << "vmdk"
	                      << "vdi"
	                      << "vhd"
	                      << "xar"
	                      << "pkg"
	                      << "z"
	                      << "taz";
}

} // namespace HomeCompa::ZipDetails::SevenZip
