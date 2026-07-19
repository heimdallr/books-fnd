#pragma once

#include <QObject>
#include <QPointer>

#include "settings/ISettings.h"

#include "export/utilgui.h"

class QAbstractScrollArea;
class QTimer;

namespace HomeCompa::Util
{

class UTILGUI_EXPORT ScrollBarController final : public QObject
{
public:
	explicit ScrollBarController(std::shared_ptr<ISettings> settings, QObject* parent = nullptr);
	void SetScrollArea(QAbstractScrollArea* area);

private: // QObject
	bool eventFilter(QObject* obj, QEvent* event) override;

private:
	QTimer* CreateTimer(const ISettings& settings, void (ScrollBarController::*)() const);
	void    OnTimeoutV() const;
	void    OnTimeoutH() const;

private:
	std::shared_ptr<ISettings>    m_settings;
	QTimer*                       m_timerV;
	QTimer*                       m_timerH;
	QPointer<QAbstractScrollArea> m_area;
};

} // namespace HomeCompa::Util
