#pragma once
#include "Engine/Core/Base.h"

namespace Engine::ShaderDef {

	enum class AOMethod
	{
		None = 0, GTAO = BIT(1)
	};

	constexpr static std::underlying_type_t<AOMethod> GetMethodIndex(const AOMethod method)
	{
		switch (method)
		{
			case AOMethod::None: return 0;
			case AOMethod::GTAO: return 1;
		}
		return 0;
	}
	constexpr static ShaderDef::AOMethod ROMETHODS[4] = { AOMethod::None, AOMethod::GTAO };

	constexpr static AOMethod GetAOMethod(const bool gtaoEnabled)
	{
		if (gtaoEnabled)
			return AOMethod::GTAO;

		return AOMethod::None;
	}

}
