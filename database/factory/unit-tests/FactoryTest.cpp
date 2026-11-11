#include "database/factory/Factory.h"

#include <gtest/gtest.h>

using namespace HomeCompa::DB;

namespace {

TEST(DBFactory, CreateSqliteDatabase)
{
	const auto db = Factory::Create<IDatabase>(IFactory::Impl::Sqlite, R"(path="file:my_shared_mem_db?mode=memory&cache=shared")");
	EXPECT_TRUE(!!db);
}

TEST(DBFactory, CreateSqliteDatabaseWritable)
{
	const auto db = Factory::Create<IDatabaseWritable>(IFactory::Impl::Sqlite, R"(path="file:my_shared_mem_db?mode=memory&cache=shared")");
	EXPECT_TRUE(!!db);
}

}
