#include "process.h"

#include <QDir>
#include <QEventLoop>
#include <QProcess>
#include <QRegularExpression>

#include "log.h"

namespace HomeCompa::Util
{
QStringList SplitStringWithQuotes(const QString& str)
{
	QRegularExpression              regex("\"([^\"]*)\"|([^\\s,]+)");
	QRegularExpressionMatchIterator it = regex.globalMatch(str);
	QStringList                     result;
	while (it.hasNext())
	{
		QRegularExpressionMatch match = it.next();
		if (match.captured(1).length() > 0)
			result.append(match.captured(1));
		else if (match.captured(2).length() > 0)
			result.append(match.captured(2));
	}
	return result;
}

bool RunProcess(const QString& file, const QString& parameters, const QString& cwd)
{
	QProcess   process;
	QEventLoop eventLoop;
	if (!cwd.isEmpty())
		process.setWorkingDirectory(QDir::toNativeSeparators(cwd));
	const auto args = SplitStringWithQuotes(parameters);

	QByteArray fixed;
	int        errorCode = 0;
	QObject::connect(&process, &QProcess::started, [&] {
		PLOGV << QString("%1 %2 launched").arg(file, args.join(" "));
	});
	QObject::connect(&process, &QProcess::errorOccurred, [&](const auto error) {
		errorCode = static_cast<int>(error) + 1;
		PLOGE << QString("%1 %2 error: %3").arg(file, args.join(" ")).arg(errorCode);
		eventLoop.exit(errorCode);
	});
	QObject::connect(&process, &QProcess::finished, [&](const int code, const QProcess::ExitStatus) {
		eventLoop.exit(code);
	});
	QObject::connect(&process, &QProcess::readyReadStandardError, [&] {
		PLOGE << process.readAllStandardError();
	});
	QObject::connect(&process, &QProcess::readyReadStandardOutput, [&] {
		PLOGV << process.readAllStandardOutput();
	});

	process.start(QDir::toNativeSeparators(file), args, QIODevice::ReadWrite);
	if (errorCode)
		return errorCode;

	process.closeWriteChannel();

	return eventLoop.exec() == 0;
}

} // namespace HomeCompa::Util
