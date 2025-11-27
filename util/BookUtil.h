#pragma once

#include "zip.h"

#include "export/util.h"

namespace HomeCompa::Util::Remove
{

struct Book
{
	long long id;
	QString   folder;
	QString   file;
};

using Books = std::vector<Book>;

using AllFilesItem = std::tuple<std::vector<QString>, std::shared_ptr<Zip::ProgressCallback>>;
using AllFiles     = std::unordered_map<QString, AllFilesItem>;
UTIL_EXPORT AllFiles CollectBookFiles(Books& books, const std::function<std::shared_ptr<Zip::ProgressCallback>()>& progressProvider);
UTIL_EXPORT AllFiles CollectImageFiles(const AllFiles& bookFiles, const QString& collectionFolder, const std::function<std::shared_ptr<Zip::ProgressCallback>()>& progressProvider);
UTIL_EXPORT void     RemoveFiles(AllFiles& allFiles, const QString& collectionFolder);

}
