#pragma once

#include <QString>

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"

#include "export/logging.h"

class QCommandLineParser;

namespace HomeCompa::Log {

class LOGGING_EXPORT LoggingInitializer final
{
	NON_COPY_MOVABLE(LoggingInitializer)

public:
	static constexpr auto CONSOLE = "console";

	static QString AddLogFileOption(QCommandLineParser& parser, QString defaultPath);

public:
	explicit LoggingInitializer(const QString& path = {});
	~LoggingInitializer();

private:
	struct Impl;
	PropagateConstPtr<Impl> m_impl;
};

} // namespace HomeCompa::Log
