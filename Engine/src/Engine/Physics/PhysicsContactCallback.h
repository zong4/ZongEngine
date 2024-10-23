#pragma once

#include "Engine/Scene/Entity.h"

#include <functional>

namespace Hazel {

	enum class ContactType : int8_t { None = -1, CollisionBegin, CollisionEnd, TriggerBegin, TriggerEnd };
	using ContactCallbackFn = std::function<void(ContactType, Entity, Entity)>;

}
