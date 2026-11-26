#pragma once

#include <QObject>

#include <gmock/gmock.h>

namespace HomeCompa::Util::Test {

class Receiver : public QObject
{
	Q_OBJECT

public:
	MOCK_METHOD(void, doSlotImpl, ());
	MOCK_METHOD(void, doSlotImplInt, (int));
	MOCK_METHOD(void, doSlotImplDouble, (double));
	MOCK_METHOD(void, doSlotImplString, (const QString&));

public slots:
	void doSlot();
	void doSlot(int);
	void doSlot(double);
	void doSlot(const QString&);
};

}
