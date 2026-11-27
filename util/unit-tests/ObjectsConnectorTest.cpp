#include "util/ObjectsConnector.h"

#include <QEventLoop>

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "private/emitter.h"
#include "private/receiver.h"

using namespace HomeCompa::Util::Test;
using namespace HomeCompa::Util;

namespace {

constexpr auto OBJECT_ID = "ID";

TEST(ObjectsConnector, Direct)
{
	Emitter  emitter;
	Receiver receiver;
	emit     emitter.doSignal();
	EXPECT_NO_THROW(ObjectsConnector::registerEmitter(OBJECT_ID, &emitter, SIGNAL(doSignal())));
	emit emitter.doSignal();
	EXPECT_NO_THROW(ObjectsConnector::registerReceiver(OBJECT_ID, &receiver, SLOT(doSlot())));
	EXPECT_CALL(receiver, doSlotImpl);
	emit emitter.doSignal();
	EXPECT_NO_THROW(ObjectsConnector::unregisterReceiver(OBJECT_ID, &receiver, SLOT(doSlot())));
	emit emitter.doSignal();
}

TEST(ObjectsConnector, Queued)
{
	Emitter  emitter;
	Receiver receiver;
	emit     emitter.doSignal();
	EXPECT_NO_THROW(ObjectsConnector::registerEmitter(OBJECT_ID, &emitter, SIGNAL(doSignal()), true));
	emit emitter.doSignal();
	EXPECT_NO_THROW(ObjectsConnector::registerReceiver(OBJECT_ID, &receiver, SLOT(doSlot()), true));
	QEventLoop eventLoop;
	EXPECT_CALL(receiver, doSlotImpl).WillOnce([&] {
		eventLoop.exit();
	});
	emit emitter.doSignal();
	EXPECT_NO_THROW(ObjectsConnector::unregisterEmitter(OBJECT_ID, &emitter, SIGNAL(doSignal())));
	emit emitter.doSignal();
	eventLoop.exec();
}

TEST(ObjectsConnector, DirectInt)
{
	Emitter  emitter;
	Receiver receiver;
	emit     emitter.doSignal(42);
	EXPECT_NO_THROW(ObjectsConnector::registerReceiver(OBJECT_ID, &receiver, SLOT(doSlot(int))));
	emit emitter.doSignal(42);
	EXPECT_NO_THROW(ObjectsConnector::registerEmitter(OBJECT_ID, &emitter, SIGNAL(doSignal(int))));
	EXPECT_CALL(receiver, doSlotImplInt(42));
	emit emitter.doSignal(42);
	EXPECT_NO_THROW(ObjectsConnector::unregisterReceiver(OBJECT_ID, &receiver, SLOT(doSlot(int))));
	emit emitter.doSignal(42);
}

TEST(ObjectsConnector, QueuedString)
{
	Emitter  emitter;
	Receiver receiver;
	emit     emitter.doSignal("rock!");
	EXPECT_NO_THROW(ObjectsConnector::registerReceiver(OBJECT_ID, &receiver, SLOT(doSlot(QString)), true));
	emit emitter.doSignal("rock!");
	EXPECT_NO_THROW(ObjectsConnector::registerEmitter(OBJECT_ID, &emitter, SIGNAL(doSignal(QString)), true));
	QEventLoop eventLoop;
	EXPECT_CALL(receiver, doSlotImplString(QString { "rock!" })).WillOnce([&](const QString& value) {
		EXPECT_EQ(value, "rock!");
		eventLoop.exit();
	});
	emit emitter.doSignal("rock!");
	EXPECT_NO_THROW(ObjectsConnector::unregisterEmitter(OBJECT_ID, &emitter, SIGNAL(doSignal(QString))));
	emit emitter.doSignal("rock!");
	eventLoop.exec();
}

} // namespace
