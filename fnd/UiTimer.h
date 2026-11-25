#pragma once

#include <functional>
#include <memory>

#include "export/fnd.h"

class QTimer;

namespace HomeCompa::Util {

FND_EXPORT std::unique_ptr<QTimer> CreateUiTimer(std::function<void()> f);

}
