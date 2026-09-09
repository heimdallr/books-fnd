#include "NativeEventFilter.h"

#include <functional>

#include "fnd/observable.h"

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
    explicit SignalHandler(std::function<void()> onSignal, QObject *parent = nullptr)
        : QObject(parent)
        , m_onSignal{std::move(onSignal)}
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

private:
    static void termSignalHandler(int)
    {
        char a = 1;
        std::ignore = ::write(sigFd[0], &a, sizeof(a));
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

    void handleSignal()
    {
        snRead->setEnabled(false);
        char tmp;
        std::ignore = read(sigFd[1], &tmp, sizeof(tmp));

        m_onSignal();

        snRead->setEnabled(true);
    }

private:
    static int sigFd[2];
    QSocketNotifier *snRead;
    const std::function<void()> m_onSignal;
};

int SignalHandler::sigFd[2];

}


class NativeEventFilter::Impl : public Observable<IObserver>
{
    SignalHandler m_signalHandler{[this]{
        qintptr_t result = 0;
        Perform(&IObserver::OnQueryEndSession, &result);
    }};
};

NativeEventFilter::NativeEventFilter(QCoreApplication& /*app*/)
{
}

NativeEventFilter::~NativeEventFilter() = default;

void NativeEventFilter::Register(IObserver* observer)
{
    m_impl->Register(observer);
}

void NativeEventFilter::Unregister(IObserver* observer)
{
    m_impl->Unregister(observer);
}
