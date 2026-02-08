#pragma once

#include "fnd/NonCopyMovable.h"
#include "fnd/memory.h"
#include "fnd/observer.h"

#include "export/util.h"

class QCoreApplication;

namespace HomeCompa::Util
{

class UTIL_EXPORT NativeEventFilter
{
	NON_COPY_MOVABLE(NativeEventFilter)

public:
	class IObserver : public Observer
	{
	public:
		virtual void OnQueryEndSession(long long* result) = 0;
	};

public:
	explicit NativeEventFilter(QCoreApplication& app);
	~NativeEventFilter();

	void Register(IObserver* observer);
	void Unregister(IObserver* observer);

private:
	class Impl;
	PropagateConstPtr<Impl> m_impl;
};

}
