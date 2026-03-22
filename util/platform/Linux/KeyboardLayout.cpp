#include "KeyboardLayout.h"

#include <cassert>

namespace HomeCompa
{

namespace
{

constexpr std::pair<const char*, const wchar_t*> LOCALES[] {
	{ "en", L"00000409" },
	{ "ru", L"00000419" },
	{ "uk", L"00000422" },
};

}

void SetKeyboardLayout(const std::string& locale)
{
	assert(false);
}

}
