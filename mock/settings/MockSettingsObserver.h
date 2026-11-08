#pragma once

#include <gmock/gmock.h>

#include "settings/ISettingsObserver.h"

namespace HomeCompa::Test {

class MockSettingsObserver : public ISettingsObserver
{
public: // ISettingsObserver
	MOCK_METHOD(void, HandleValueChanged, (const QString& key, const QVariant& value), (override));
};

static_assert(!std::is_abstract_v<MockSettingsObserver>, "MockSettingsObserver is abstract!");

}
