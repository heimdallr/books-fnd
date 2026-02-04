#include "ImageUtil.h"

#include <QBuffer>
#include <QImage>
#include <QPixmap>

#include "jxl/jxl.h"

namespace HomeCompa::Util
{

namespace
{

using Decoder = QPixmap (*)(const QByteArray&);
using Recoder = std::pair<QByteArray, const char*> (*)(const QByteArray& bytes, const char* type);

QPixmap QtDecoder(const QByteArray& data)
{
	if (QPixmap pixmap; pixmap.loadFromData(data))
		return pixmap;

	return {};
}

QPixmap JxlDecoder(const QByteArray& data)
{
	auto image = JXL::Decode(data);
	return QPixmap::fromImage(std::move(image));
}

std::pair<QByteArray, const char*> QtRecoder(const QByteArray& data, const char* type)
{
	return std::make_pair(data, type);
}

std::pair<QByteArray, const char*> JxlRecoder(const QByteArray& data, const char*)
{
	std::pair<QByteArray, const char*> result;
	auto                               image = JXL::Decode(data);
	QByteArray                         bytes;
	{
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::WriteOnly);
		const auto hasAlpha = image.pixelFormat().alphaUsage() == QPixelFormat::UsesAlpha;
		image.save(&buffer, hasAlpha ? PNG : JPEG);
		result.second = hasAlpha ? IMAGE_PNG : IMAGE_JPEG;
	}
	result.first = std::move(bytes);
	return result;
}

struct ImageFormatDescription
{
	const char* mediaType;
	Decoder     decoder;
	Recoder     recoder;
};

constexpr ImageFormatDescription DEFAULT_DESCRIPTION { IMAGE_JPEG, &QtDecoder, &QtRecoder };

constexpr std::pair<const char*, ImageFormatDescription> SIGNATURES[] {
	{ "\xFF\xD8\xFF\xE0", { IMAGE_JPEG, &QtDecoder, &QtRecoder } },
	{ "\x89\x50\x4E\x47",  { IMAGE_PNG, &QtDecoder, &QtRecoder } },
	{		 "\xFF\x0A",  { nullptr, &JxlDecoder, &JxlRecoder } },
};

} // namespace

QPixmap Decode(const QByteArray& bytes)
{
	assert(!bytes.isEmpty());
	const auto it      = std::ranges::find_if(SIGNATURES, [&](const auto& item) {
        return bytes.startsWith(item.first);
    });
	const auto decoder = it != std::end(SIGNATURES) ? it->second.decoder : &QtDecoder;
	return decoder(bytes);
}

std::pair<QByteArray, const char*> Recode(const QByteArray& bytes)
{
	assert(!bytes.isEmpty());
	const auto  it          = std::ranges::find_if(SIGNATURES, [&](const auto& item) {
        return bytes.startsWith(item.first);
    });
	const auto& description = it != std::end(SIGNATURES) ? it->second : DEFAULT_DESCRIPTION;
	return description.recoder(bytes, description.mediaType);
}

QImage HasAlpha(const QImage& image, const char* data)
{
	if (memcmp(data, "\xFF\xD8\xFF\xE0", 4) == 0)
		return image.convertToFormat(QImage::Format_RGB888);

	if (!image.hasAlphaChannel())
		return image.convertToFormat(QImage::Format_RGB888);

	auto argb = image.convertToFormat(QImage::Format_RGBA8888);
	for (int i = 0, h = argb.height(), w = argb.width(); i < h; ++i)
	{
		const auto* pixels = reinterpret_cast<const QRgb*>(argb.constScanLine(i));
		if (std::any_of(pixels, pixels + static_cast<size_t>(w), [](const QRgb pixel) {
				return qAlpha(pixel) < UCHAR_MAX;
			}))
		{
			return argb;
		}
	}

	return image.convertToFormat(QImage::Format_RGB888);
}

} // namespace HomeCompa::Util
