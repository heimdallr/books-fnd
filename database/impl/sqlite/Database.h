#pragma once

#include <memory>
#include <string>

#include "log.h"

#include "export/dbsqlite.h"

namespace HomeCompa::DB {

class IDatabase;
class IDatabaseWritable;

}

namespace HomeCompa::DB::Impl::Sqlite {

DBSQLITE_EXPORT std::unique_ptr<IDatabase> CreateDatabase(const std::string& connection);
DBSQLITE_EXPORT std::unique_ptr<IDatabaseWritable> CreateDatabaseWritable(const std::string& connection);

inline std::string_view LogStatement(const std::string_view statement)
{
#ifndef NDEBUG
	PLOGD << statement;
#endif
	return statement;
}

} // namespace HomeCompa::DB::Impl::Sqlite
