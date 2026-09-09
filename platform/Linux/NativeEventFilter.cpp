#include "NativeEventFilter.h"

#include <QCoreApplication>
#include <QSocketNotifier>
#include <QObject>
#include <sys/socket.h>
#include <signal.h>

using namespace HomeCompa::Platform;
using namespace HomeCompa;

namespace
{

class SignalHandler : public QObject {
public:
    explicit SignalHandler(QObject *parent = nullptr)
        : QObject(parent)
    {
        if (::socketpair(AF_UNIX, SOCK_STREAM, 0, sigFd) == -1)
            qFatal("Couldn't create socketpair");

        snRead = new QSocketNotifier(sigFd[1], QSocketNotifier::Read, this);
        connect(snRead, &QSocketNotifier::activated, this, &SignalHandler::handleSignal);

        setupSignalHandlers();
    }

    ~SignalHandler()
    {
        ::close(sigFd[0]);
        ::close(sigFd[1]);
    }

    static void termSignalHandler(int)
    {
        char a = 1;
        (void)::write(sigFd[0], &a, sizeof(a));
    }

    void setupSignalHandlers()
    {
        struct sigaction action;
        action.sa_handler = SignalHandler::termSignalHandler;
        sigemptyset(&action.sa_mask);
        action.sa_flags = SA_RESTART;

        sigaction(SIGINT, &action, nullptr);
        sigaction(SIGTERM, &action, nullptr);
    }

private:
    void handleSignal()
    {
        snRead->setEnabled(false);
        char tmp;
        (void)::read(sigFd[1], &tmp, sizeof(tmp));

        QCoreApplication::exit();

        snRead->setEnabled(true);
    }

private:
    static int sigFd[2];
    QSocketNotifier *snRead;
};

int SignalHandler::sigFd[2];

}


class NativeEventFilter::Impl
{
    SignalHandler m_signalHandler;
};

NativeEventFilter::NativeEventFilter(QCoreApplication& /*app*/)
{
}

NativeEventFilter::~NativeEventFilter() = default;

void NativeEventFilter::Register(IObserver*)
{
}

void NativeEventFilter::Unregister(IObserver*)
{
}
