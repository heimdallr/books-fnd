#pragma once

class QStringView;

namespace HomeCompa::Util
{

class XmlAttributes // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	virtual ~XmlAttributes()                                = default;
	virtual QStringView GetAttribute(QStringView key) const = 0;
	virtual size_t      GetCount() const                    = 0;
	virtual QStringView GetName(size_t index) const         = 0;
	virtual QStringView GetValue(size_t index) const        = 0;
};

}
