#pragma once

#include <memory>
#include <string>

#include "export/dbfactory.h"

namespace HomeCompa::DB {

class IDatabase;
class IDatabaseWritable;

}

namespace HomeCompa::DB::Factory {

enum class Impl
{
	Sqlite,
};

DBFACTORY_EXPORT std::unique_ptr<IDatabase> Create(Impl impl, const std::string& connection);
DBFACTORY_EXPORT std::unique_ptr<IDatabaseWritable> CreateWritable(Impl impl, const std::string& connection);

} // namespace HomeCompa::DB::Factory
