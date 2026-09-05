#include "SaxParser.h"

#include <QIODevice>
#include <QStringList>

#include <xercesc/parsers/SAXParser.hpp>
#include <xercesc/sax/HandlerBase.hpp>
#include <xercesc/sax/InputSource.hpp>
#include <xercesc/util/BinInputStream.hpp>

#include "Initializer.h"
#include "XmlAttributes.h"
#include "log.h"

using namespace HomeCompa;
using namespace Util;
namespace xercesc = xercesc_3_3;

namespace
{

QStringView ToStringView(const XMLCh* const value)
{
	return QStringView { value, static_cast<qsizetype>(std::char_traits<char16_t>::length(value)) };
}

class XmlAttributesImpl final : public XmlAttributes
{
public:
	void SetAttributeList(const xercesc::AttributeList& attributes) noexcept
	{
		m_attributes = &attributes;
	}

private: // SaxParser::Attributes
	QString GetAttribute(const QString& key) const override
	{
		if (const auto value = m_attributes->getValue(key.toStdU16String().data()))
			return QString::fromStdU16String(value);
		return {};
	}

	size_t GetCount() const override
	{
		return m_attributes->getLength();
	}

	QString GetName(const size_t index) const override
	{
		assert(index < GetCount());
		return QString::fromStdU16String(m_attributes->getName(index));
	}

	QString GetValue(const size_t index) const override
	{
		assert(index < GetCount());
		return QString::fromStdU16String(m_attributes->getValue(index));
	}

private:
	const xercesc::AttributeList* m_attributes { nullptr };
};

class XmlStack
{
public:
	void Push(const QStringView tag) //-V801
	{
		m_data.push_back(tag.toString());
		m_key.reset();
	}

	void Pop(const QStringView tag) //-V801
	{
		assert(!m_data.isEmpty());
		if (tag != m_data.back())
			return;

		m_data.pop_back();
		m_key.reset();
	}

	const QString& ToString() const
	{
		if (!m_key)
			m_key = m_data.join('/');

		return *m_key;
	}

private:
	mutable std::optional<QString> m_key;
	QStringList                    m_data;
};

class BinInputStream final : public xercesc_3_3::BinInputStream
{
public:
	explicit BinInputStream(QIODevice& source)
		: m_source(source)
	{
	}

	void SetStopped(const bool value) noexcept
	{
		m_stopped = value;
	}

	bool IsStopped() const noexcept
	{
		return m_stopped;
	}

private: // xercesc::BinInputStream
	XMLFilePos curPos() const override
	{
		return m_source.pos();
	}

	const XMLCh* getContentType() const override
	{
		return nullptr;
	}

	XMLSize_t readBytes(XMLByte* const toFill, const XMLSize_t maxToRead) override
	{
		return m_stopped ? 0 : m_source.read(reinterpret_cast<char*>(toFill), static_cast<int64_t>(maxToRead));
	}

private:
	QIODevice& m_source;
	bool       m_stopped { false };
};

class InputSource final : public xercesc::InputSource
{
public:
	InputSource(QIODevice& source)
		: m_binInputStream(new BinInputStream(source))
	{
	}

	void SetStopped(const bool value) const noexcept
	{
		m_binInputStream->SetStopped(value);
	}

	bool IsStopped() const noexcept
	{
		return m_binInputStream->IsStopped();
	}

private: // xercesc::InputSource
	xercesc::BinInputStream* makeStream() const override
	{
		return m_binInputStream;
	}

private:
	BinInputStream* m_binInputStream;
};

class IDeclHandler
{
public:
	virtual ~IDeclHandler() = default;

	virtual void XMLDecl(const XMLCh* const versionStr, const XMLCh* const encodingStr, const XMLCh* const standaloneStr, const XMLCh* const actualEncodingStr) = 0;
};

class SaxHandler final
	: public xercesc::HandlerBase
	, public IDeclHandler
{
public:
	SaxHandler(SaxParser& parser, InputSource& inputSource)
		: m_parser(parser)
		, m_inputSource(inputSource)
	{
	}

private: // xercesc::DocumentHandler
	void processingInstruction(const XMLCh* const target, const XMLCh* const data) override
	{
		if (m_inputSource.IsStopped())
			return;

		if (!m_parser.OnProcessingInstruction(ToStringView(target), ToStringView(data)))
			m_inputSource.SetStopped(true);
	}

	void startElement(const XMLCh* const name, xercesc::AttributeList& args) override
	{
		if (m_inputSource.IsStopped())
			return;

		m_stack.Push(name);
		const auto& key = m_stack.ToString();
		m_attributes.SetAttributeList(args);
		if (!m_parser.OnStartElement(ToStringView(name), key, m_attributes))
			m_inputSource.SetStopped(true);
	}

	void endElement(const XMLCh* const name) override
	{
		if (m_inputSource.IsStopped())
			return;

		if (const auto& key = m_stack.ToString(); !m_parser.OnEndElement(ToStringView(name), key))
			m_inputSource.SetStopped(true);

		m_stack.Pop(name);
	}

	void characters(const XMLCh* const chars, const XMLSize_t length) override
	{
		if (m_inputSource.IsStopped() || length == 0)
			return;

		if (const auto& key = m_stack.ToString(); !m_parser.OnCharacters(key, QStringView { chars, chars + length }))
			m_inputSource.SetStopped(true);
	}

private: // xercesc::ErrorHandler
	void warning(const xercesc::SAXParseException& exc) override
	{
		if (m_inputSource.IsStopped())
			return;

		if (!m_parser.OnWarning(exc.getLineNumber(), exc.getColumnNumber(), QString::fromStdU16String(exc.getMessage())))
			m_inputSource.SetStopped(true);
	}

	void error(const xercesc::SAXParseException& exc) override
	{
		if (m_inputSource.IsStopped())
			return;

		if (!m_parser.OnError(exc.getLineNumber(), exc.getColumnNumber(), QString::fromStdU16String(exc.getMessage())))
			m_inputSource.SetStopped(true);
	}

	void fatalError(const xercesc::SAXParseException& exc) override
	{
		if (m_inputSource.IsStopped())
			return;

		if (!m_parser.OnFatalError(exc.getLineNumber(), exc.getColumnNumber(), QString::fromStdU16String(exc.getMessage())))
			m_inputSource.SetStopped(true);
	}

private: // IDeclHandler
	void XMLDecl(const XMLCh* const versionStr, const XMLCh* const encodingStr, const XMLCh* const standaloneStr, const XMLCh* const actualEncodingStr) override
	{
		if (m_inputSource.IsStopped())
			return;

		if (!m_parser.OnXMLDecl(ToStringView(versionStr), ToStringView(encodingStr), ToStringView(standaloneStr), ToStringView(actualEncodingStr)))
			m_inputSource.SetStopped(true);
	}

private:
	XmlStack          m_stack;
	XmlAttributesImpl m_attributes {};

	SaxParser&   m_parser;
	InputSource& m_inputSource;
};

class SAXParserImpl : public xercesc::SAXParser
{
public:
	void SetDeclHandler(IDeclHandler* const declHandler) noexcept
	{
		m_declHandler = declHandler;
	}

private: // xercesc::SAXParser
	void XMLDecl(const XMLCh* const versionStr, const XMLCh* const encodingStr, const XMLCh* const standaloneStr, const XMLCh* const actualEncodingStr) override
	{
		assert(m_declHandler);
		m_declHandler->XMLDecl(versionStr, encodingStr, standaloneStr, actualEncodingStr);
	}

private:
	IDeclHandler* m_declHandler { nullptr };
};

} // namespace

class SaxParser::Impl
{
public:
	Impl(SaxParser& self, QIODevice& stream)
		: m_self(self)
		, m_inputSource(stream)
	{
		m_saxParser.setValidationScheme(xercesc::SAXParser::Val_Never);
		m_saxParser.setDoNamespaces(false);
		m_saxParser.setDoSchema(false);
		m_saxParser.setHandleMultipleImports(true);
		m_saxParser.setValidationSchemaFullChecking(false);
	}

	void Parse()
	{
		SaxHandler handler(m_self, m_inputSource);
		m_saxParser.setDocumentHandler(&handler);
		m_saxParser.setErrorHandler(&handler);
		m_saxParser.SetDeclHandler(&handler);

		m_saxParser.parse(m_inputSource);
	}

private:
	XMLPlatformInitializer m_initializer;
	SAXParserImpl          m_saxParser;
	SaxParser&             m_self;
	InputSource            m_inputSource;
};

SaxParser::SaxParser(QIODevice& stream)
	: m_impl(*this, stream)
{
}

SaxParser::~SaxParser() = default;

void SaxParser::Parse()
{
	m_impl->Parse();
}

bool SaxParser::IsLastItemProcessed() const noexcept
{
	return m_processed;
}

bool SaxParser::OnProcessingInstruction(QStringView /*target*/, QStringView /*data*/)
{
	return true;
}

bool SaxParser::OnXMLDecl(QStringView /*versionStr*/, QStringView /*encodingStr*/, QStringView /*standaloneStr*/, QStringView /*actualEncodingStr*/)
{
	return true;
}

bool SaxParser::OnStartElement(QStringView /*name*/, QStringView /*path*/, const XmlAttributes& /*attributes*/)
{
	return true;
}

bool SaxParser::OnEndElement(QStringView /*name*/, QStringView /*path*/)
{
	return true;
}

bool SaxParser::OnCharacters(QStringView /*path*/, QStringView /*value*/)
{
	return true;
}

bool SaxParser::OnWarning(const size_t line, const size_t column, const QString& text)
{
	PLOGW << line << ":" << column << " " << text;
	return true;
}

bool SaxParser::OnError(const size_t line, const size_t column, const QString& text)
{
	PLOGE << line << ":" << column << " " << text;
	return false;
}

bool SaxParser::OnFatalError(const size_t line, const size_t column, const QString& text)
{
	PLOGF << line << ":" << column << " " << text;
	return false;
}
