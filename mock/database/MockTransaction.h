#pragma once

#include <gmock/gmock.h>

#include "database/interface/ITransaction.h"

namespace HomeCompa::DB::Test {

class MockTransaction : public ITransaction
{
public:
	MOCK_METHOD(bool, Commit, (), (override));
	MOCK_METHOD(bool, Rollback, (), (override));
	MOCK_METHOD(std::unique_ptr<ICommand>, CreateCommand, (std::string_view command), (override));
	MOCK_METHOD(std::unique_ptr<IQuery>, CreateQuery, (std::string_view command), (override));
};

static_assert(!std::is_abstract_v<MockTransaction>, "MockTransaction is abstract!");

}
