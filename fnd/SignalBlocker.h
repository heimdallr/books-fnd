#pragma once

#include <QtCore/QSignalBlocker>

template <typename T>
class SignalBlocker
{
public:
	explicit SignalBlocker(T* object)
		: object_(object)
		, blocker_(object)
	{
	}

	T* operator->()
	{
		return object_;
	}

private:
	T*             object_;
	QSignalBlocker blocker_;
};
