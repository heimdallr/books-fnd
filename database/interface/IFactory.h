#pragma once

#include <memory>
#include <string>

#include "database/interface/IDatabase.h"

namespace HomeCompa::DB {

class IFactory // NOLINT(cppcoreguidelines-special-member-functions)
{
public:
	enum class Impl
	{
		Sqlite,
	};

public:
	virtual ~IFactory() = default;

public:
	virtual std::unique_ptr<IDatabase>         Create(Impl impl, const std::string& connection) const         = 0;
	virtual std::unique_ptr<IDatabaseWritable> CreateWritable(Impl impl, const std::string& connection) const = 0;

	template <typename T>
	std::unique_ptr<T> CreateT(Impl impl, const std::string& connection) const = delete;
};

template <>
inline std::unique_ptr<IDatabase> IFactory::CreateT<IDatabase>(const Impl impl, const std::string& connection) const
{
	return Create(impl, connection);
}

template <>
inline std::unique_ptr<IDatabaseWritable> IFactory::CreateT<IDatabaseWritable>(const Impl impl, const std::string& connection) const
{
	return CreateWritable(impl, connection);
}

} // namespace HomeCompa::DB
