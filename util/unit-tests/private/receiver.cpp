#include "receiver.h"

using namespace HomeCompa::Util::Test;

void Receiver::doSlot()
{
	doSlotImpl();
}

void Receiver::doSlot(const int value)
{
	doSlotImplInt(value);
}

void Receiver::doSlot(const double value)
{
	doSlotImplDouble(value);
}

void Receiver::doSlot(const QString& value)
{
	doSlotImplString(value);
}
