#pragma once

#include <QObject>

#include "export/utilgui.h"

class QAbstractScrollArea;

namespace HomeCompa::Util
{

class UTILGUI_EXPORT ItemViewToolTipper final : public QObject
{
public:
	explicit ItemViewToolTipper(QObject* parent = nullptr);
	void SetScrollArea(QAbstractScrollArea* area);

private: // QObject
	bool eventFilter(QObject* obj, QEvent* event) override;
};

}
