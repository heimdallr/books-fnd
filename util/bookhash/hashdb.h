#pragma once

#include "flihash.h"

namespace HomeCompa::DB
{

class IDatabase;

}

namespace HomeCompa::Util
{

BookHashItem ParseDbHash(DB::IDatabase& db, const QString& folder, const QString& file);

}
