#pragma once

#include "flihash.h"

#include "export/util.h"

class QCryptographicHash;

namespace HomeCompa::Util
{

UTIL_EXPORT void ParseFb2Hash(BookHashItem& bookHashItem, QCryptographicHash& cryptographicHash);

}
