#include "init.h"

#include <QCommandLineParser>

#include <plog/Appenders/RollingFileAppender.h>
#include <plog/Formatters/TxtFormatter.h>

#include "LogAppender.h"
#include "QtLoHandler.h"
#include "log.h"

using namespace HomeCompa::Log;

struct LoggingInitializer::Impl
{
	plog::RollingFileAppender<plog::TxtFormatter> rollingFileAppender;
	LogAppender                                   logAppender;
	QtLogHandler                                  qtLogHandler;

	explicit Impl(const std::filesystem::path& path)
		: rollingFileAppender(path.string().data(), 1024 * 1024 * 1024, 10)
		, logAppender(&rollingFileAppender)
	{
	}
};

LoggingInitializer::LoggingInitializer(const std::filesystem::path& path)
	: m_impl(path)
{
}

LoggingInitializer::~LoggingInitializer() = default;

QString LoggingInitializer::AddLogFileOption(QCommandLineParser& parser, const QString& defaultPath)
{
	static constexpr auto LOG = "log";
	parser.addOption(
		{
			{ QString(LOG[0]), QString(LOG) },
			"Log file path",
			defaultPath
    }
	);
	return LOG;
}
