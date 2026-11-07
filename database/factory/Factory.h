#pragma once

#include <memory>
#include <string>

#include "database/interface/IFactory.h"

#include "export/dbfactory.h"

namespace HomeCompa::DB {

class IDatabase;
class IDatabaseWritable;

}

namespace HomeCompa::DB::Factory {

namespace Details {

template <typename T, typename... AllowedTypes>
concept IsOneOf = (... || std::same_as<T, AllowedTypes>);

template <typename T>
concept ExportedDatabaseType = IsOneOf<T, IDatabase, IDatabaseWritable>;

}

template <Details::ExportedDatabaseType T>
DBFACTORY_EXPORT std::unique_ptr<T> Create(IFactory::Impl impl, const std::string& connection);

extern template DBFACTORY_EXPORT std::unique_ptr<IDatabase> Create(IFactory::Impl impl, const std::string& connection);
extern template DBFACTORY_EXPORT std::unique_ptr<IDatabaseWritable> Create(IFactory::Impl impl, const std::string& connection);

} // namespace HomeCompa::DB::Factory

namespace HomeCompa::DB {

class DBFACTORY_EXPORT DatabaseFactory final : public IFactory
{
private: // IFactory
	std::unique_ptr<IDatabase>         Create(Impl impl, const std::string& connection) const override;
	std::unique_ptr<IDatabaseWritable> CreateWritable(Impl impl, const std::string& connection) const override;
};

}
