#include "Factory.h"

#include <cassert>

#include "database/interface/IDatabase.h"

#include "database/impl/sqlite/Database.h"

namespace HomeCompa::DB::Factory {

namespace {

template <typename T>
using FactoryCreator = std::unique_ptr<T> (*)(const std::string& connection);

constexpr std::pair<FactoryCreator<IDatabase>, FactoryCreator<IDatabaseWritable>> DATABASE_CREATIRS[] {
	{
	 &DB::Impl::Sqlite::Create<IDatabase>,
	 &DB::Impl::Sqlite::Create<IDatabaseWritable>,
	 },
};

size_t ToIndex(const IFactory::Impl impl)
{
	const auto index = static_cast<size_t>(impl);
	assert(index < std::size(DATABASE_CREATIRS));
	return index;
}

template <typename T>
FactoryCreator<T> GetCreator(const IFactory::Impl impl)
{
	return DATABASE_CREATIRS[ToIndex(impl)];
}

template <>
FactoryCreator<IDatabase> GetCreator(const IFactory::Impl impl)
{
	return DATABASE_CREATIRS[ToIndex(impl)].first;
}

template <>
FactoryCreator<IDatabaseWritable> GetCreator(const IFactory::Impl impl)
{
	return DATABASE_CREATIRS[ToIndex(impl)].second;
}

} // namespace

template <Details::ExportedDatabaseType T>
std::unique_ptr<T> Create(const IFactory::Impl impl, const std::string& connection)
{
	const auto creator = GetCreator<T>(impl);
	assert(creator);
	return creator(connection);
}

template DBFACTORY_EXPORT std::unique_ptr<IDatabase> Create(IFactory::Impl impl, const std::string& connection);
template DBFACTORY_EXPORT std::unique_ptr<IDatabaseWritable> Create(IFactory::Impl impl, const std::string& connection);

} // namespace HomeCompa::DB::Factory

using namespace HomeCompa::DB;

std::unique_ptr<IDatabase> DatabaseFactory::Create(const Impl impl, const std::string& connection) const
{
	return Factory::Create<IDatabase>(impl, connection);
}

std::unique_ptr<IDatabaseWritable> DatabaseFactory::CreateWritable(const Impl impl, const std::string& connection) const
{
	return Factory::Create<IDatabaseWritable>(impl, connection);
}
