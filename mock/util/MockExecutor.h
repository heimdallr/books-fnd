#pragma once

#include <gmock/gmock.h>

#include "util/IExecutor.h"

namespace HomeCompa::Util::Test {

class MockExecutor : public IExecutor
{
public:
	MOCK_METHOD(size_t, DoExecute, (Task && task, int priority, bool displace), (override));
	MOCK_METHOD(void, Stop, (), (override));
};

static_assert(!std::is_abstract_v<MockExecutor>, "MockExecutor is abstract!");

}
