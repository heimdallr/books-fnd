#pragma once

#include <gmock/gmock.h>

#include "database/interface/IQuery.h"

namespace HomeCompa::DB::Test {

class MockQuery : public IQuery
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

public: // IQuery
	MOCK_METHOD(bool, Eof, (), (const, override));
	MOCK_METHOD(void, Next, (), (override));
	MOCK_METHOD(void, Reset, (), (override));
	MOCK_METHOD(size_t, ColumnCount, (), (const, override));
	MOCK_METHOD(std::string, ColumnName, (size_t index), (const, override));
	MOCK_METHOD(bool, IsNull, (size_t index), (const, override));
	MOCK_METHOD(int, GetInt, (size_t index), (const, override));
	MOCK_METHOD(long long int, GetLong, (size_t index), (const, override));
	MOCK_METHOD(double, GetDouble, (size_t index), (const, override));
	MOCK_METHOD(std::string, GetString, (size_t index), (const, override));
	MOCK_METHOD(QString, GetQString, (size_t index), (const, override));
	MOCK_METHOD(const char*, GetRawString, (size_t index), (const, override));
};

static_assert(!std::is_abstract_v<MockQuery>, "MockQuery is abstract!");

} // namespace HomeCompa::DB::Test
