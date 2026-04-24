#include "ImageRestore.h"

#include <QBuffer>
#include <QDir>
#include <QFileInfo>
#include <QPixmap>

#include "fnd/EnumBitmask.h"
#include "fnd/IsOneOf.h"
#include "fnd/try.h"

#include "interface/constants/SettingsConstant.h"

#include "xml/SaxParser.h"
#include "xml/XmlAttributes.h"
#include "xml/XmlWriter.h"

#include "BookUtil.h"
#include "Constant.h"
#include "ISettings.h"
#include "ImageUtil.h"
#include "zip.h"

#include "config/version.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

enum class ImageProcessing
{
	None             = 0,
	RemoveCovers     = 1 << 0,
	RemoveImages     = 1 << 1,
	GrayscaleCovers  = 1 << 2,
	GrayscaleImages  = 1 << 3,
	ConvertToJpegPng = 1 << 4,
};

}

ENABLE_BITMASK_OPERATORS(ImageProcessing);

namespace
{

using Covers = std::unordered_map<QString, std::pair<bool, QByteArray>>;

constexpr auto BINARY       = "binary";
constexpr auto ID           = "id";
constexpr auto CONTENT_TYPE = "content-type";

void ConvertToGrayscale(QImage& image)
{
	if (image.pixelFormat().alphaUsage() == QPixelFormat::AlphaUsage::IgnoresAlpha)
		return image.convertTo(QImage::Format::Format_Grayscale8);

	QImage alpha(image.width(), image.height(), QImage::Format::Format_Grayscale8);

	for (auto h = 0, H = image.height(); h < H; ++h)
	{
		auto* alphaBytes = alpha.scanLine(h);
		for (auto w = 0, W = image.width(); w < W; ++w, ++alphaBytes)
			*alphaBytes = static_cast<uchar>(qAlpha(image.pixel(w, h)));
	}

	image.convertTo(QImage::Format::Format_Grayscale8);
	image.setAlphaChannel(alpha);
}

class BinaryParser final : public SaxParser
{
	static constexpr auto COVERPAGE_IMAGE = "FictionBook/description/title-info/coverpage/image";

public:
	BinaryParser(QIODevice& input, Covers& covers, const ImageProcessing imageProcessing)
		: SaxParser { input }
		, m_imageProcessing { imageProcessing }
		, m_covers { covers }
	{
		Parse();
		input.seek(0);
	}

private:
	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override
	{
		if (name == BINARY)
			return (m_id = attributes.GetAttribute(ID)), true;

		if (path == COVERPAGE_IMAGE)
		{
			for (size_t i = 0, sz = attributes.GetCount(); i < sz; ++i)
			{
				auto attributeName  = attributes.GetName(i);
				auto attributeValue = attributes.GetValue(i);
				if (attributeName.endsWith(":href"))
				{
					if (const auto it = std::ranges::find_if(
							attributeValue,
							[](const auto ch) {
								return ch != '#';
							}
						);
					    it != attributeValue.end())
						m_coverId = attributeValue.last(std::distance(it, attributeValue.end())).trimmed();
					break;
				}
			}
			return true;
		}

		return true;
	}

	bool OnCharacters(const QString&, const QString& value) override
	{
		if (m_id.isEmpty())
			return true;

		auto       bytes   = QByteArray::fromBase64(value.toUtf8());
		const auto isCover = m_id == m_coverId;
		if ((isCover && !(m_imageProcessing & ImageProcessing::RemoveCovers)) || (!isCover && !(m_imageProcessing & ImageProcessing::RemoveImages)))
			m_covers.emplace(std::move(m_id), std::make_pair(isCover, std::move(bytes)));
		m_id = {};

		return true;
	}

private:
	const ImageProcessing m_imageProcessing;

	Covers& m_covers;
	QString m_coverId;
	QString m_id;
};

class SaxPrinter final : public SaxParser
{
	static constexpr auto FICTION_BOOK       = "FictionBook";
	static constexpr auto DESCRIPTION        = "FictionBook/description";
	static constexpr auto DOCUMENT_INFO      = "FictionBook/description/document-info";
	static constexpr auto PROGRAM_USED       = "FictionBook/description/document-info/program-used";
	static constexpr auto TITLE_INFO         = "FictionBook/description/title-info";
	static constexpr auto AUTHOR             = "FictionBook/description/title-info/author";
	static constexpr auto AUTHOR_FIRST_NAME  = "first-name";
	static constexpr auto AUTHOR_LAST_NAME   = "last-name";
	static constexpr auto AUTHOR_MIDDLE_NAME = "middle-name";
	static constexpr auto BOOK_TITLE         = "FictionBook/description/title-info/book-title";
	static constexpr auto SEQUENCE           = "FictionBook/description/title-info/sequence";

public:
	SaxPrinter(QIODevice& input, QIODevice& output, Covers covers, std::unique_ptr<const ExtractedBook> metadataReplacement, const ImageProcessing imageProcessing)
		: SaxParser { input }
		, m_metadataReplacement { std::move(metadataReplacement) }
		, m_imageProcessing { imageProcessing }
		, m_writer { output }
		, m_covers { std::move(covers) }
	{
		Parse();
		assert(m_hasProgramUsed);
	}

	bool HasError() const noexcept
	{
		return m_hasError;
	}

private: // Util::SaxParser
	bool OnProcessingInstruction(const QString& target, const QString& data) override
	{
		return m_writer.WriteProcessingInstruction(target, data), true;
	}

	bool OnStartElement(const QString& name, const QString& path, const XmlAttributes& attributes) override
	{
		if (m_specialNode || (m_metadataReplacement && IsOneOf(path, AUTHOR, BOOK_TITLE, SEQUENCE)))
			return m_specialNode = true;

		if (name == BINARY && !m_covers.empty())
			return WriteImage(attributes.GetAttribute(ID)), true;

		m_writer.WriteStartElement(name, attributes);

		if (path == TITLE_INFO && m_metadataReplacement)
			for (const auto invoker : {
					 &SaxPrinter::WriteAuthor,
					 &SaxPrinter::WriteTitle,
					 &SaxPrinter::WriteSeries,
				 })
				std::invoke(invoker, this);

		return true;
	}

	bool OnEndElement(const QString& name, const QString& path) override
	{
		if (m_specialNode)
		{
			if (name == BINARY)
				m_specialNode = false;

			if (IsOneOf(path, AUTHOR, BOOK_TITLE, SEQUENCE))
				m_specialNode = false;
			return true;
		}

		if (path == DOCUMENT_INFO && !m_hasProgramUsed)
		{
			m_writer.WriteStartElement("program-used").WriteCharacters(QString("%1 %2").arg(PRODUCT_ID, PRODUCT_VERSION)).WriteEndElement();
			m_hasProgramUsed = true;
		}

		if (path == DESCRIPTION)
		{
			if (!m_hasProgramUsed)
			{
				m_writer.WriteStartElement("document-info").WriteStartElement("program-used").WriteCharacters(QString("%1 %2").arg(PRODUCT_ID, PRODUCT_VERSION)).WriteEndElement().WriteEndElement();
				m_hasProgramUsed = true;
			}
		}

		if (path == FICTION_BOOK)
			WriteImages();

		return m_writer.WriteEndElement(), true;
	}

	bool OnCharacters(const QString& path, const QString& value) override
	{
		if (m_specialNode)
			return true;

		if (path != PROGRAM_USED)
			return m_writer.WriteCharacters(value), true;

		m_hasProgramUsed = true;
		return m_writer.WriteCharacters(QString("%1, %2 %3").arg(value, PRODUCT_ID, PRODUCT_VERSION)), true;
	}

	bool OnWarning(const size_t line, const size_t column, const QString& text) override
	{
		PLOGW << line << ":" << column << " " << text;
		return true;
	}

	bool OnError(const size_t line, const size_t column, const QString& text) override
	{
		m_hasError = true;
		PLOGE << line << ":" << column << " " << text;
		return false;
	}

	bool OnFatalError(const size_t line, const size_t column, const QString& text) override
	{
		return OnError(line, column, text);
	}

private:
	void WriteAuthor()
	{
		assert(m_metadataReplacement);
		const auto node = m_writer.Guard("author");
		node->WriteStartElement(AUTHOR_FIRST_NAME).WriteCharacters(m_metadataReplacement->authorFull.firstName).WriteEndElement();
		node->WriteStartElement(AUTHOR_MIDDLE_NAME).WriteCharacters(m_metadataReplacement->authorFull.middleName).WriteEndElement();
		node->WriteStartElement(AUTHOR_LAST_NAME).WriteCharacters(m_metadataReplacement->authorFull.lastName).WriteEndElement();
	}

	void WriteTitle()
	{
		assert(m_metadataReplacement);
		m_writer.WriteStartElement("book-title").WriteCharacters(m_metadataReplacement->title).WriteEndElement();
	}

	void WriteSeries()
	{
		assert(m_metadataReplacement);
		if (m_metadataReplacement->series.isEmpty())
			return;

		const auto node = m_writer.Guard("sequence");
		node->WriteAttribute("name", m_metadataReplacement->series);
		if (m_metadataReplacement->seqNumber > 0)
			node->WriteAttribute("number", QString::number(m_metadataReplacement->seqNumber));
	}

	void WriteImage(const QString& imageFileName)
	{
		m_specialNode = true;

		const auto [isCover, body] = [&]() -> std::pair<bool, QByteArray> {
			try
			{
				const auto it = m_covers.find(imageFileName);
				if (it == m_covers.end())
					return {};

				auto result = std::move(it->second);
				m_covers.erase(it);

				return result;
			}
			catch (const std::exception& ex)
			{
				PLOGE << ex.what();
			}

			return {};
		}();

		if (body.isNull())
		{
			PLOGW << imageFileName << " not found";
			return;
		}

		WriteImage(imageFileName, isCover, body);
	}

	void WriteImages()
	{
		for (const auto& [name, body] : m_covers)
			WriteImage(name, body.first, body.second);

		m_covers.clear();
	}

	void WriteImage(const QString& name, const bool isCover, const QByteArray& body)
	{
		if (body.isEmpty())
			return;

		auto image = Decode(body).toImage();
		if (image.isNull())
			return;

		if ((isCover && !!(m_imageProcessing & ImageProcessing::GrayscaleCovers)) || (!isCover && !!(m_imageProcessing & ImageProcessing::GrayscaleImages)))
			ConvertToGrayscale(image);

		if (const auto [bytes, mediaType] = Encode(image); !bytes.isEmpty())
			m_writer.WriteStartElement(BINARY).WriteAttribute(ID, name).WriteAttribute(CONTENT_TYPE, mediaType).WriteCharacters(QString::fromUtf8(bytes.toBase64())).WriteEndElement();
	}

private:
	std::unique_ptr<const ExtractedBook> m_metadataReplacement;
	const ImageProcessing                m_imageProcessing;
	XmlWriter                            m_writer;
	Covers                               m_covers;
	bool                                 m_hasError { false };
	bool                                 m_hasProgramUsed { false };
	bool                                 m_specialNode { false };
};

QByteArray PrepareToExportImpl(QIODevice& stream, Covers covers, std::unique_ptr<const ExtractedBook> metadataReplacement, const ImageProcessing imageProcessing)
{
	QByteArray byteArray;
	QBuffer    buffer(&byteArray);
	buffer.open(QIODevice::WriteOnly);
	const SaxPrinter saxPrinter(stream, buffer, std::move(covers), std::move(metadataReplacement), imageProcessing);
	return saxPrinter.HasError() ? QByteArray {} : byteArray;
}

QByteArray PrepareToExportImpl(QIODevice& stream, const QString& folder, const QString& fileName, const std::shared_ptr<const ISettings>& settings, std::unique_ptr<const ExtractedBook> metadataReplacement)
{
	Covers covers;
	ExtractBookImages(
		folder,
		fileName,
		[&covers](QString name, const bool isCover, QByteArray body) {
			covers.try_emplace(std::move(name), std::make_pair(isCover, std::move(body)));
			return false;
		},
		settings
	);

	const auto imageProcessing = ImageProcessing::None | (settings->Get(Export::REMOVE_COVER_KEY, false) ? ImageProcessing::RemoveCovers : ImageProcessing::None)
	                           | (settings->Get(Export::REMOVE_IMAGES_KEY, false) ? ImageProcessing::RemoveImages : ImageProcessing::None)
	                           | (settings->Get(Export::GRAYSCALE_COVER_KEY, false) ? ImageProcessing::GrayscaleCovers : ImageProcessing::None)
	                           | (settings->Get(Export::GRAYSCALE_IMAGES_KEY, false) ? ImageProcessing::GrayscaleImages : ImageProcessing::None)
	                           | (settings->Get(Flibrary::Constant::Settings::EXPORT_CONVERT_IMAGES_KEY, false) ? ImageProcessing::ConvertToJpegPng : ImageProcessing::None);

	if (imageProcessing != ImageProcessing::None || !!metadataReplacement)
		BinaryParser(stream, covers, imageProcessing);

	if (covers.empty() && !metadataReplacement)
		return stream.readAll();

	if (auto byteArray = PrepareToExportImpl(stream, std::move(covers), std::move(metadataReplacement), imageProcessing); !byteArray.isEmpty())
		return byteArray;

	stream.seek(0);
	return stream.readAll();
}

bool ParseCover(const QString& folder, const QString& fileName, const ExtractBookImagesCallback& callback, bool& stop)
{
	if (!QFile::exists(folder))
		return false;

	const Zip  zip(folder);
	const auto fileList = zip.GetFileNameList();
	const auto file     = QFileInfo(fileName).completeBaseName();
	if (!fileList.contains(file))
		return false;

	const auto stream = zip.Read(file);
	auto       body   = stream->GetStream().readAll();
	stop              = callback(Global::COVER, true, std::move(body));
	return true;
}

void ParseImages(const QString& folder, const QString& fileName, const ExtractBookImagesCallback& callback)
{
	if (!QFile::exists(folder))
		return;

	const Zip  zip(folder);
	auto       fileList   = zip.GetFileNameList();
	const auto filePrefix = QString("%1/").arg(QFileInfo(fileName).completeBaseName());
	if (const auto [begin, end] = std::ranges::remove_if(
			fileList,
			[&](const auto& item) {
				return !item.startsWith(filePrefix) /*|| item == filePrefix*/;
			}
		);
	    begin != end)
		fileList.erase(begin, end);

	for (const auto& file : fileList)
	{
		auto body = zip.Read(file)->GetStream().readAll();
		if (!body.isEmpty() && callback(file.split('/').back(), false, std::move(body)))
			return;
	}
}

constexpr const char* EXTENSIONS[] { "zip", "7z" };

void ExtractBookImagesCoverImpl(const QFileInfo& fileInfo, const QString& fileName, const ExtractBookImagesCallback& callback, const std::shared_ptr<const ISettings>& settings)
{
	if (settings && settings->Get(Export::REMOVE_COVER_KEY, false))
		return;

	bool stop = false;
	for (const auto* ext : EXTENSIONS)
	{
		TRY("parse cover", [&] {
			ParseCover(QString("%1/%2/%3.%4").arg(fileInfo.dir().path(), Global::COVERS, fileInfo.completeBaseName(), ext), fileName, callback, stop);
			return true;
		});
	}
}

void ExtractBookImagesImagesImpl(const QFileInfo& fileInfo, const QString& fileName, const ExtractBookImagesCallback& callback, const std::shared_ptr<const ISettings>& settings)
{
	if (settings && settings->Get(Export::REMOVE_IMAGES_KEY, false))
		return;

	for (const auto* ext : EXTENSIONS)
	{
		TRY("parse cover", [&] {
			ParseImages(QString("%1/%2/%3.%4").arg(fileInfo.dir().path(), Global::IMAGES, fileInfo.completeBaseName(), ext), fileName, callback);
			return true;
		});
	}
}

} // namespace

namespace HomeCompa::Util
{

QByteArray PrepareToExport(QIODevice& input, const QString& folder, const QString& fileName, const std::shared_ptr<const ISettings>& settings, std::unique_ptr<const ExtractedBook> metadataReplacement)
{
	return PrepareToExportImpl(input, folder, fileName, settings, std::move(metadataReplacement));
}

void ExtractBookImages(const QString& folder, const QString& fileName, const ExtractBookImagesCallback& callback, const std::shared_ptr<const ISettings>& settings)
{
	const QFileInfo fileInfo(folder);
	ExtractBookImagesCoverImpl(fileInfo, fileName, callback, settings);
	ExtractBookImagesImagesImpl(fileInfo, fileName, callback, settings);
}

} // namespace HomeCompa::Util
