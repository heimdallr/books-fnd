#include <QGuiApplication>

#include <gtest/gtest.h>

int main(int argc, char* argv[])
{
#ifdef Q_OS_LINUX
	qputenv("QT_QPA_PLATFORM", "offscreen");
#endif

	[[maybe_unused]] const QGuiApplication app(argc, argv);
	::testing::InitGoogleTest(&argc, argv);
	return RUN_ALL_TESTS();
}

namespace {

TEST(Util, Main)
{
	SUCCEED();
}

}
