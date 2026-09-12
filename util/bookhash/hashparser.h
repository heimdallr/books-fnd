#pragma once

#include "flihash.h"

namespace HomeCompa::Util
{

struct HashParser
{
	struct HashImageItem
	{
		QString id;
		QString hash;
		QString pHash;
		bool    linked { true };
	};

	struct Section
	{
		Section* parent { nullptr };
		size_t   count { 0 };
		size_t   size { 0 };
		uint64_t simHash { 0 };
		using Ptr = std::unique_ptr<Section>;
		std::unordered_map<QString, Ptr> children;
	};

	using HashImageItems = std::vector<HashImageItem>;
};

} // namespace HomeCompa::Util
