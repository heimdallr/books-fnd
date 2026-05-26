#pragma once

#include "export/utilgui.h"

namespace HomeCompa
{

class ISettings;

}

class QFont;

namespace HomeCompa::Util
{

UTILGUI_EXPORT void Serialize(const QFont& font, ISettings& settings);
UTILGUI_EXPORT void Deserialize(QFont& font, const ISettings& settings);

}
