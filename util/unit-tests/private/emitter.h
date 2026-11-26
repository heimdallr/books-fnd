#pragma once

#include <QObject>

namespace HomeCompa::Util::Test {

struct Emitter : QObject
{
	Q_OBJECT
signals:
	void doSignal() const;
	void doSignal(int) const;
	void doSignal(double) const;
	void doSignal(QString) const;
};

}
