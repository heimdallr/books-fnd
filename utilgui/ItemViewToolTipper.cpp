#include "ItemViewToolTipper.h"

#include <QAbstractItemView>
#include <QHelpEvent>
#include <QToolTip>

#include "Constant.h"

using namespace HomeCompa::Util;

ItemViewToolTipper::ItemViewToolTipper(QObject* parent)
	: QObject(parent)
{
}

bool ItemViewToolTipper::eventFilter(QObject* obj, QEvent* event)
{
	if (event->type() != QEvent::ToolTip)
		return false;

	auto* view = qobject_cast<QAbstractItemView*>(obj->parent());
	if (!(view && view->model()))
		return false;

	const auto* helpEvent = static_cast<const QHelpEvent*>(event); // NOLINT(cppcoreguidelines-pro-type-static-cast-downcast)

	const auto pos   = helpEvent->pos();
	const auto index = view->indexAt(pos);
	if (!index.isValid())
		return false;

	const auto itemTooltip = index.data(Qt::ToolTipRole).toString();
	if (itemTooltip.isEmpty())
		return false;

	const auto itemText             = index.data().toString();
	const auto authorAnnotationMode = itemText == Global::INFO;

	auto font = view->font();
	font.setPointSizeF(font.pointSizeF() * (authorAnnotationMode ? 0.8 : 1.2));

	if (!authorAnnotationMode && !m_showForceColumns.contains(index.column()))
	{
		const int itemTextWidth = QFontMetrics(font).horizontalAdvance(itemText);

		const auto rect      = view->visualRect(index);
		auto       rectWidth = rect.width();

		if (index.flags() & Qt::ItemIsUserCheckable)
			rectWidth -= rect.height();

		if (itemTextWidth <= rectWidth)
			return true;
	}

	QToolTip::setFont(font);
	QToolTip::showText(helpEvent->globalPos(), itemTooltip, view);

	return true;
}

void ItemViewToolTipper::SetScrollArea(const QAbstractScrollArea* area)
{
	area->viewport()->installEventFilter(this);
}

void ItemViewToolTipper::SetShowForceColumns(std::set<int> columns)
{
	m_showForceColumns = std::move(columns);
}
