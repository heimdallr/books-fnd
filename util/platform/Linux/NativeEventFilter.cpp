#include "NativeEventFilter.h"

using namespace HomeCompa::Util;

class NativeEventFilter::Impl
{
};

NativeEventFilter::NativeEventFilter(QCoreApplication& app)
{
}

NativeEventFilter::~NativeEventFilter() = default;

void NativeEventFilter::Register(IObserver* observer)
{
}

void NativeEventFilter::Unregister(IObserver* observer)
{
}
