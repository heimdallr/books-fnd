#pragma once

#include <set>

#include <QObject>

#include "export/utilgui.h"

class QAbstractScrollArea;

namespace HomeCompa::Util
{

class UTILGUI_EXPORT ItemViewToolTipper final : public QObject
{
public:
	explicit ItemViewToolTipper(QObject* parent = nullptr);
	void SetScrollArea(const QAbstractScrollArea* area);
	void SetShowForceColumns(std::set<int> columns);

private: // QObject
	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	std::set<int> m_showForceColumns;
};

}
