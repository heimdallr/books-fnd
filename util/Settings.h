#pragma once

#include <memory>

#include "ISettings.h"

#include "export/util.h"

namespace HomeCompa::SettingsFactory
{

class AbstractSettings : virtual public ISettings
{
};

UTIL_EXPORT std::unique_ptr<AbstractSettings> CreateStub();
UTIL_EXPORT std::unique_ptr<AbstractSettings> Create(const QString& fileName);
UTIL_EXPORT std::unique_ptr<AbstractSettings> Create(const QString& organization, const QString& application);

} // namespace HomeCompa::SettingsFactory
