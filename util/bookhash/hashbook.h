#pragma once

#include "flihash.h"

#include "export/util.h"

class QCryptographicHash;

namespace HomeCompa::Util
{

UTIL_EXPORT void ParseBookHash(BookHashItem& bookHashItem, QCryptographicHash& cryptographicHash);

void SetHash(ImageHashItem& item, QCryptographicHash& cryptographicHash);

using HashValues = std::vector<std::pair<size_t, QString>>;
using Hist       = std::unordered_map<QString, size_t>;

struct CalculateHashResult
{
	HashValues hashValues;
	QString    hash;
	size_t     count;
	size_t     size;
};

CalculateHashResult CalculateHash(Hist& hist);

}
