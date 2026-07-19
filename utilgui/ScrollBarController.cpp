#include "ScrollBarController.h"

#include <QApplication>
#include <QDateTime>
#include <QEvent>
#include <QListView>
#include <QMenu>
#include <QMouseEvent>
#include <QScrollBar>
#include <QStringListModel>
#include <QStyle>
#include <QTimer>
#include <QWidgetAction>

#include "fnd/memory.h"

#include "Constant.h"

using namespace HomeCompa;
using namespace HomeCompa::Util;

namespace
{

constexpr auto MENU_ITEM_ENABLED_TEMPLATE = "ui/ScrollBarContextMenu/Items/%1";

constexpr auto CONTEXT = "ScrollBarController";
constexpr auto OPTIONS = QT_TRANSLATE_NOOP("ScrollBarController", "Options...");

QString Tr(const char* str)
{
	return QCoreApplication::translate(CONTEXT, str);
}

class Model final : public QStringListModel
{
public:
	Model(std::shared_ptr<ISettings> settings, const QStringList& strings, QObject* parent = nullptr)
		: QStringListModel(strings, parent)
		, m_settings { std::move(settings) }
	{
	}

private: // QAbstractItemModel
	Qt::ItemFlags flags(const QModelIndex& index) const override
	{
		return QStringListModel::flags(index) | Qt::ItemIsUserCheckable;
	}

	QVariant data(const QModelIndex& index, const int role) const override
	{
		if (role != Qt::CheckStateRole)
			return QStringListModel::data(index, role);

		return QVariant::fromValue(m_settings->Get(QString(MENU_ITEM_ENABLED_TEMPLATE).arg(index.row()), true) ? Qt::Checked : Qt::Unchecked);
	}

	bool setData(const QModelIndex& index, const QVariant& value, const int role) override
	{
		if (role != Qt::CheckStateRole)
			return QStringListModel::setData(index, value, role);

		m_settings->Set(QString(MENU_ITEM_ENABLED_TEMPLATE).arg(index.row()), value.value<Qt::CheckState>() == Qt::Checked);
		emit dataChanged(index, index, { Qt::CheckStateRole });
		return true;
	}

private:
	PropagateConstPtr<ISettings, std::shared_ptr> m_settings;
};

class ScrollBar final : public QScrollBar
{
public:
	ScrollBar(std::shared_ptr<ISettings> settings, const Qt::Orientation orientation, QWidget* parent = nullptr)
		: QScrollBar(orientation, parent)
		, m_settings { std::move(settings) }
	{
		connect(this, &QAbstractSlider::actionTriggered, this, &ScrollBar::OnScrollBarAction);
	}

private: // QWidget
	void contextMenuEvent(QContextMenuEvent* event) override
	{
		QMenu* menu = createStandardContextMenu(event->pos());
		if (!menu)
			return;

		menu->addSeparator();
		menu->addAction(Tr(OPTIONS), [this, strings = [this, menu] {
							QStringList result;
							for (qsizetype i = 0, j = 0, sz = menu->actions().count(); i < sz; ++i)
							{
								auto* action = menu->actions().at(i);
								if (const auto text = action->text(); !text.isEmpty())
								{
									action->setVisible(m_settings->Get(QString(MENU_ITEM_ENABLED_TEMPLATE).arg(j++), true));
									result << text;
								}
							}
							return result;
						}()] {
			auto  font = parentWidget()->font();
			QMenu optionsMenu(this);
			optionsMenu.setFont(font);

			Model     model(m_settings, strings);
			QListView view;
			view.setFont(font);
			view.setModel(&model);
			view.setFixedHeight(view.sizeHintForRow(0) * model.rowCount() + 4);

			QFontMetrics fontMetrics(font);
			auto         max = fontMetrics.boundingRect(strings.front()).width();
			for (const auto& string : strings | std::views::drop(1))
				max = std::max(max, fontMetrics.boundingRect(string).width());

			view.setFixedWidth(max + view.sizeHintForRow(0));

			auto* action = new QWidgetAction(&optionsMenu);
			action->setFont(font);
			action->setDefaultWidget(&view);
			optionsMenu.addAction(action);

			optionsMenu.setFixedSize(view.width() + 6, view.height() + 6);
			optionsMenu.exec(QCursor::pos());
		});

		menu->setAttribute(Qt::WA_DeleteOnClose);
		menu->setFont(parentWidget()->font());
		menu->popup(event->globalPos());
	}

private:
	void OnScrollBarAction(const int action)
	{
		switch (action)
		{
			case SliderSingleStepSub:
				return OnArrowButtonClicked(true);
			case SliderSingleStepAdd:
				return OnArrowButtonClicked(false);
			default:
				break;
		}
	}

	void OnArrowButtonClicked(const bool up)
	{
		const auto apply = [this] {
			if (QGuiApplication::keyboardModifiers() & Qt::ControlModifier)
				return true;

			auto clickTime = QDateTime::currentDateTime();
			if (m_clickTime.msecsTo(clickTime) < 400)
				return true;

			m_clickTime = std::move(clickTime);
			return false;
		}();

		if (apply)
			setValue(up ? minimum() : maximum());
	}

private:
	std::shared_ptr<ISettings> m_settings;
	QDateTime                  m_clickTime { QDateTime::currentDateTime() };
};

} // namespace

ScrollBarController::ScrollBarController(std::shared_ptr<ISettings> settings, QObject* parent)
	: QObject(parent)
	, m_settings { std::move(settings) }
	, m_timerV { CreateTimer(*m_settings, &ScrollBarController::OnTimeoutV) }
	, m_timerH { CreateTimer(*m_settings, &ScrollBarController::OnTimeoutH) }
{
}

void ScrollBarController::SetScrollArea(QAbstractScrollArea* area)
{
	m_area = area;

	m_area->setHorizontalScrollBar(new ScrollBar(m_settings, Qt::Horizontal, m_area));
	m_area->setVerticalScrollBar(new ScrollBar(m_settings, Qt::Vertical, m_area));

	m_area->setMouseTracking(m_timerV);
	m_area->viewport()->installEventFilter(this);

	if (m_timerV)
		return;

	m_area->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
	m_area->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
}

bool ScrollBarController::eventFilter(QObject* /*obj*/, QEvent* event)
{
	if (!m_timerV)
		return false;

	if (event->type() == QEvent::Enter)
	{
		m_timerV->stop();
		m_timerH->stop();
		return false;
	}

	if (event->type() == QEvent::Leave)
	{
		m_timerV->start();
		m_timerH->start();
		return false;
	}

	if (event->type() == QEvent::MouseMove)
	{
		OnTimeoutV();
		OnTimeoutH();
		return false;
	}

	return false;
}

QTimer* ScrollBarController::CreateTimer(const ISettings& settings, void (ScrollBarController::*f)() const)
{
	if (!settings.Get(Preferences::PREFER_HIDE_SCROLLBARS_KEY, true))
		return nullptr;

	auto* timer = new QTimer(this);
	timer->setSingleShot(false);
	timer->setInterval(std::chrono::milliseconds(200));
	connect(timer, &QTimer::timeout, this, f);
	return timer;
}

void ScrollBarController::OnTimeoutV() const
{
	if (!m_area)
		return;

	auto&       area     = *m_area;
	const auto& viewport = *area.viewport();

	const auto threshold   = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	const auto x           = viewport.geometry().width() - threshold - (area.verticalScrollBar()->isVisible() ? 0 : threshold);
	const auto topLeft     = viewport.mapToGlobal(QPoint { x, std::numeric_limits<int>::min() / 100 });
	const auto bottomRight = viewport.mapToGlobal(QPoint { x + 5 * threshold / 2, std::numeric_limits<int>::max() / 100 });

	if (const auto pos = QCursor::pos(); QRect(topLeft, bottomRight).contains(pos))
		return area.setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	area.setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_timerV->stop();
}

void ScrollBarController::OnTimeoutH() const
{
	if (!m_area)
		return;

	auto&       area     = *m_area;
	const auto& viewport = *area.viewport();

	const auto threshold   = QApplication::style()->pixelMetric(QStyle::PM_ScrollBarExtent);
	const auto y           = viewport.geometry().height() - threshold - (area.horizontalScrollBar()->isVisible() ? 0 : threshold);
	const auto topLeft     = viewport.mapToGlobal(QPoint { std::numeric_limits<int>::min() / 100, y });
	const auto bottomRight = viewport.mapToGlobal(QPoint { std::numeric_limits<int>::max() / 100, y + 5 * threshold / 2 });

	if (const auto pos = QCursor::pos(); QRect(topLeft, bottomRight).contains(pos))
		return area.setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);

	area.setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	m_timerH->stop();
}
