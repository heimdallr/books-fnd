#pragma once

#include <functional>
#include <memory>

#include "export/utilgui.h"

class QTimer;

namespace HomeCompa::Util
{

UTILGUI_EXPORT std::unique_ptr<QTimer> CreateUiTimer(std::function<void()> f);

}
