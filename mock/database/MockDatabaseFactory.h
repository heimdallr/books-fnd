#pragma once

#include <gmock/gmock.h>

#include "database/interface/IFactory.h"

namespace HomeCompa::DB::Test {

class MockFactory : public IFactory
{
public:
	MOCK_METHOD(std::unique_ptr<IDatabase>, Create, (Impl impl, const std::string& connection), (const, override));
	MOCK_METHOD(std::unique_ptr<IDatabaseWritable>, CreateWritable, (Impl impl, const std::string& connection), (const, override));
};

static_assert(!std::is_abstract_v<MockFactory>, "MockFactory is abstract!");

}
