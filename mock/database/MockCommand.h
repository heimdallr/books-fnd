#pragma once

#include <gmock/gmock.h>

#include "database/interface/ICommand.h"

namespace HomeCompa::DB::Test {

class MockCommand : public ICommand
{
public: // ICommand
	MOCK_METHOD(bool, Execute, (), (override));
	MOCK_METHOD(int, Bind, (size_t index), (override));
	MOCK_METHOD(int, BindInt, (size_t index, int value), (override));
	MOCK_METHOD(int, BindLong, (size_t index, long long int value), (override));
	MOCK_METHOD(int, BindDouble, (size_t index, double value), (override));
	MOCK_METHOD(int, BindString, (size_t index, std::string_view value), (override));
	MOCK_METHOD(int, Bind, (std::string_view name), (override));
	MOCK_METHOD(int, BindInt, (std::string_view name, int value), (override));
	MOCK_METHOD(int, BindLong, (std::string_view name, long long int value), (override));
	MOCK_METHOD(int, BindDouble, (std::string_view name, double value), (override));
	MOCK_METHOD(int, BindString, (std::string_view name, std::string_view value), (override));
};

static_assert(!std::is_abstract_v<MockCommand>, "MockCommand is abstract!");

} // namespace HomeCompa::DB::Test
