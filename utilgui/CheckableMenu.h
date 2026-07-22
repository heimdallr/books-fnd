#pragma once

#include <vector>

#include <QString>

#include "export/utilgui.h"

class QMenu;
class QWidget;

namespace HomeCompa::Util
{

UTILGUI_EXPORT QMenu* CreateCheckableMenu(const std::vector<std::pair<QString, bool>>& values, std::function<void(int, bool)> callback, QWidget* parent);

} // namespace HomeCompa::Util
