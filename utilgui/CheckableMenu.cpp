#include "CheckableMenu.h"

#include <QMenu>
#include <QStringListModel>
#include <QTableView>
#include <QWidgetAction>

#include "MultiHeaderView.h"

using namespace HomeCompa;
using namespace HomeCompa::Util;

namespace
{

class Model final : public QStringListModel
{
public:
	explicit Model(const QStringList& strings, QObject* parent = nullptr)
		: QStringListModel(strings, parent)
		, m_checked(strings.size(), false)
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

		return m_checked[index.row()] ? Qt::Checked : Qt::Unchecked;
	}

	bool setData(const QModelIndex& index, const QVariant& value, const int role) override
	{
		if (role != Qt::CheckStateRole)
			return QStringListModel::setData(index, value, role);

		m_checked[index.row()] = value.value<Qt::CheckState>() == Qt::Checked;
		emit dataChanged(index, index, { Qt::CheckStateRole });
		return true;
	}

private:
	std::vector<bool> m_checked;
};

class TableView final : public QTableView
{
public:
	TableView(QAbstractItemModel* model, const int width, QWidget* parent = nullptr)
		: QTableView(parent)
	{
		setFont(parent->font());

		QTableView::setModel(model);

		const auto sectionHeight = verticalHeader()->sectionSize(0);
		setFixedHeight(sectionHeight * model->rowCount() + 4);
		setFixedWidth(width + 2 * sectionHeight);

		setShowGrid(false);
		horizontalHeader()->setVisible(false);
		horizontalHeader()->setStretchLastSection(true);
		verticalHeader()->setVisible(false);

		setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
		setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	}
};

} // namespace

namespace HomeCompa::Util
{

QMenu* CreateCheckableMenu(const std::vector<std::pair<QString, bool>>& values, std::function<void(int, bool)> callback, QWidget* parent)
{
	auto  font = parent->font();
	auto* menu = new QMenu(parent);
	menu->setFont(font);

	const auto          strings = values | std::views::keys | std::ranges::to<QStringList>();
	QAbstractItemModel* model   = new Model(strings, menu);
	for (auto&& [checked, row] : std::views::zip(values | std::views::values, std::views::iota(0)))
		model->setData(model->index(row, 0), checked ? Qt::Checked : Qt::Unchecked, Qt::CheckStateRole);

	QObject::connect(model, &QAbstractItemModel::dataChanged, [callback = std::move(callback)](const QModelIndex& topLeft, const QModelIndex& bottomRight, const QList<int>& roles = QList<int>()) {
		if (roles.contains(Qt::CheckStateRole))
			for (const auto& index : QItemSelection(topLeft, bottomRight).indexes())
				callback(index.row(), index.data(Qt::CheckStateRole).value<Qt::CheckState>() == Qt::Checked);
	});

	const QFontMetrics fontMetrics(font);
	const auto         maxWidth = std::accumulate(strings.cbegin(), strings.cend(), -1, [&](const int init, const QString& string) {
		return std::max(init, fontMetrics.boundingRect(string).width());
	});

	auto* view = new TableView(model, maxWidth, menu);

	auto* action = new QWidgetAction(menu);
	action->setFont(font);
	action->setDefaultWidget(view);

	menu->addAction(action);
	menu->setFixedSize(view->width() + 6, view->height() + 6);

	return menu;
}

} // namespace HomeCompa::Util
