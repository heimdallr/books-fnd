#include "flihash.h"

#include <QCryptographicHash>

#include <ranges>
#include <set>

#include <QDir>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonObject>

#include "fnd/FindPair.h"

#include "Constant.h"
#include "hashfb2.h"
#include "hashxml.h"
#include "zip.h"

using namespace HomeCompa::Util;
using namespace HomeCompa;

namespace
{

constexpr auto KEY_FOLDER    = "folder";
constexpr auto KEY_FILE      = "file";
constexpr auto KEY_ID        = "id";
constexpr auto KEY_HASH      = "hash";
constexpr auto KEY_PHASH     = "phash";
constexpr auto KEY_COVER     = "cover";
constexpr auto KEY_IMAGES    = "images";
constexpr auto KEY_HISTOGRAM = "histogram";
constexpr auto KEY_WORD      = "word";
constexpr auto KEY_COUNT     = "count";

BookHashItem GetHash_7z(const QString& path, const QString& file)
{
	QCryptographicHash md5 { QCryptographicHash::Md5 };
	auto               bookHashItem = BookHashItemProvider(path).Get(file);
	ParseFb2Hash(bookHashItem, md5);
	return bookHashItem;
}

BookHashItem GetHash_xml(const QString& path, const QString& file)
{
	return ParseXmlHash(path, file);
}

std::unique_ptr<Zip> GetZip(const QFileInfo& fileInfo, const char* type)
{
	const auto zipPath = fileInfo.dir().absoluteFilePath(QString("%1/%2.zip").arg(type, fileInfo.completeBaseName()));
	return QFile::exists(zipPath) ? std::make_unique<Zip>(zipPath) : std::unique_ptr<Zip> {};
}

} // namespace

struct BookHashItemProvider::Impl
{
	Zip             zip;
	const QFileInfo fileInfo;

	std::unique_ptr<Zip> coversZip { GetZip(fileInfo, Global::COVERS) };
	std::unique_ptr<Zip> imagesZip { GetZip(fileInfo, Global::IMAGES) };

	std::set<QString> covers { (coversZip ? coversZip->GetFileNameList() : QStringList {}) | std::ranges::to<std::set<QString>>() };
	std::set<QString> images { (imagesZip ? imagesZip->GetFileNameList() : QStringList {}) | std::ranges::to<std::set<QString>>() };

	explicit Impl(const QString& path)
		: zip(path)
		, fileInfo(path)
	{
	}
};

BookHashItemProvider::BookHashItemProvider(const QString& path)
	: m_impl(path)
{
}

BookHashItemProvider::~BookHashItemProvider() = default;

QStringList BookHashItemProvider::GetFiles() const
{
	return m_impl->zip.GetFileNameList();
}

BookHashItem BookHashItemProvider::Get(const QString& file) const
{
	BookHashItem bookHashItem { .folder = m_impl->fileInfo.fileName(), .file = file, .body = m_impl->zip.Read(file)->GetStream().readAll() };

	const auto baseName = QFileInfo(file).completeBaseName();
	if (m_impl->coversZip && m_impl->covers.contains(baseName))
		bookHashItem.cover = { QString {}, m_impl->coversZip->Read(baseName)->GetStream().readAll() };

	if (m_impl->imagesZip)
		std::ranges::transform(
			std::ranges::equal_range(
				m_impl->images,
				baseName + "/",
				{},
				[n = baseName.length() + 1](const QString& item) {
					return QStringView { item.begin(), std::next(item.begin(), n) };
				}
			),
			std::back_inserter(bookHashItem.images),
			[&](const QString& item) {
				return ImageHashItem { item.split("/").back(), m_impl->imagesZip->Read(item)->GetStream().readAll() };
			}
		);

	return bookHashItem;
}

namespace HomeCompa::Util
{

BookHashItem GetHash(const QString& path, const QString& file)
{
	static constexpr std::pair<const char*, BookHashItem (*)(const QString&, const QString&)> parsers[] {
#define ITEM(NAME) { #NAME, &GetHash_##NAME }
		ITEM(xml),
#undef ITEM
	};

	auto bookHashItem = FindSecond(parsers, QFileInfo(path).suffix().toLower().toStdString().data(), &GetHash_7z, PszComparer {})(path, file);
	bookHashItem.body.clear();

	return bookHashItem;
}

std::ostream& operator<<(std::ostream& stream, const BookHashItem& bookHashItem)
{
	auto result = QStringList() << QString("%1/%2, %3").arg(bookHashItem.folder, bookHashItem.file, bookHashItem.parseResult.title) << QString("full hash: %1").arg(bookHashItem.parseResult.id)
	                            << QString("top-10 hash: %1").arg(bookHashItem.parseResult.hashText);
	std::ranges::transform(bookHashItem.parseResult.hashValues, std::back_inserter(result), [](const auto& item) {
		return QString("%1: %2").arg(item.first).arg(item.second);
	});
	result << QString("cover%1").arg(
		bookHashItem.cover.hash.isEmpty() ? QString { " not found" } : QString(" hash: %1, pHash: %2").arg(bookHashItem.cover.hash).arg(bookHashItem.cover.pHash, 16, 16, QChar { '0' })
	);
	std::ranges::transform(bookHashItem.images, std::back_inserter(result), [](const auto& item) {
		return QString("image %1 hash: %2, pHash: %3").arg(item.file, item.hash).arg(item.pHash, 16, 16, QChar { '0' });
	});
	return stream << result.join('\n').toStdString();
}

QByteArray Serialize(const BookHashItem& bookHashItem)
{
	QJsonArray images;
	for (const auto& image : bookHashItem.images)
		images.append(
			QJsonObject {
				{    KEY_ID,                   image.file },
				{  KEY_HASH,                   image.hash },
				{ KEY_PHASH, QString::number(image.pHash) },
        }
		);

	QJsonArray histogram;
	for (const auto& [count, word] : bookHashItem.parseResult.hashValues)
		histogram.append(
			QJsonObject {
				{ KEY_COUNT, static_cast<int>(count) },
				{  KEY_WORD,                    word },
        }
		);
	QJsonObject obj {
		{    KEY_FOLDER,               bookHashItem.folder },
        {      KEY_FILE,                 bookHashItem.file },
        {        KEY_ID,       bookHashItem.parseResult.id },
        {      KEY_HASH, bookHashItem.parseResult.hashText },
        { KEY_HISTOGRAM,              std::move(histogram) },
	};
	if (!bookHashItem.cover.hash.isEmpty())
		obj.insert(
			KEY_COVER,
			QJsonObject {
				{  KEY_HASH,                   bookHashItem.cover.hash },
				{ KEY_PHASH, QString::number(bookHashItem.cover.pHash) },
        }
		);
	if (!images.isEmpty())
		obj.insert(KEY_IMAGES, std::move(images));

	return QJsonDocument(obj).toJson(QJsonDocument::Compact);
}

} // namespace HomeCompa::Util
