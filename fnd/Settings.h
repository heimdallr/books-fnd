#pragma once

#include <memory>

#include "fnd/ISettings.h"

#include "export/fnd.h"

namespace HomeCompa::SettingsFactory {

class AbstractSettings : public ISettings
{
};

FND_EXPORT std::unique_ptr<AbstractSettings> CreateStub();
FND_EXPORT std::unique_ptr<AbstractSettings> Create(const QString& fileName);
FND_EXPORT std::unique_ptr<AbstractSettings> Create(const QString& organization, const QString& application);
FND_EXPORT std::unique_ptr<AbstractSettings> CreateMemory(const QString& organization, const QString& application);
FND_EXPORT std::unique_ptr<AbstractSettings> CreateDecorator(std::shared_ptr<ISettings> impl, std::unordered_map<QString, QVariant> replacement = {});
FND_EXPORT std::unique_ptr<AbstractSettings> CreateGroup(ISettings& impl, QString prefix);

} // namespace HomeCompa::SettingsFactory
