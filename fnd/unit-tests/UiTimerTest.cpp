#include "fnd/UiTimer.h"

#include <QDateTime>
#include <QEventLoop>
#include <QTimer>

#include <gtest/gtest.h>

using namespace HomeCompa::Util;

namespace {

TEST(UiTimerTest, Test)
{
	QEventLoop eventLoop;
	bool       called = false;
	const auto timer  = CreateUiTimer([&, startedAt = QDateTime::currentMSecsSinceEpoch()]() {
		called = true;
		eventLoop.exit();
		EXPECT_GT(QDateTime::currentMSecsSinceEpoch() - startedAt, 100);
	});
	ASSERT_TRUE(!!timer);
	timer->start();
	eventLoop.exec();
	EXPECT_TRUE(called);
}

}
