#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Asset/AssetTypes.h"

#include <choc/containers/choc_Value.h>
#include <choc/text/choc_StringUtilities.h>

#include <imgui/imgui.h>

#include <string>
#include <utility>

namespace Hazel::Utils {

	std::string GetTypeName(const choc::value::Value& v);
	choc::value::Type GetTypeFromName(const std::string_view name);


	template<AssetType assetType, typename T>
	bool IsAssetHandle(const T& t)
	{
		static_assert(assetType == AssetType::Audio || assetType == AssetType::Animation || assetType == AssetType::Skeleton, "IsAssetHandle<T, AssetType> requires AssetType::Audio, AssetType::Animation, or AssetType::Skeleton!");
		static_assert(std::is_same_v<T, choc::value::Type> || std::is_same_v<T, choc::value::Value> || std::is_same_v<T, choc::value::ValueView>, "IsAssetHandle<T, AssetType> requires T to be choc::value::Type, choc::value::Value, or choc::value::ValueView");

		if constexpr (assetType == AssetType::Audio) return t.isObjectWithClassName("AudioAsset") || t.isObjectWithClassName("UUID");
		if constexpr (assetType == AssetType::Animation) return t.isObjectWithClassName("AnimationAsset");
		if constexpr (assetType == AssetType::Skeleton) return t.isObjectWithClassName("SkeletonAsset");

		return false;
	}

	template<typename T>
	bool IsAssetHandle(const T& t)
	{
		return IsAssetHandle<AssetType::Audio>(t) || IsAssetHandle<AssetType::Animation>(t) || IsAssetHandle<AssetType::Skeleton>(t);
	}


	template<AssetType assetType>
	choc::value::Type CreateAssetHandleType()
	{
		static_assert(assetType == AssetType::Audio || assetType == AssetType::Animation || assetType == AssetType::Skeleton, "CreateAssetHandleType<AssetType> requires AssetType::Audio, AssetType::Animation, or AssetType::Skeleton!");

		choc::value::Type t;
		if constexpr (assetType == AssetType::Audio)     t = choc::value::Type::createObject("AudioAsset");
		if constexpr (assetType == AssetType::Animation) t = choc::value::Type::createObject("AnimationAsset");
		if constexpr (assetType == AssetType::Skeleton)  t = choc::value::Type::createObject("SkeletonAsset");

		t.addObjectMember("Value", choc::value::Type::createInt64());
		return t;
	}


	template<AssetType assetType>
	choc::value::Value CreateAssetHandleValue(AssetHandle id)
	{
		static_assert(assetType == AssetType::Audio || assetType == AssetType::Animation || assetType == AssetType::Skeleton, "CreateAssetHandleValue<AssetType> requires AssetType::Audio, AssetType::Animation, or AssetType::Skeleton!");

		if constexpr (assetType == AssetType::Audio)     return choc::value::createObject("AudioAsset", "Value", choc::value::Value((int64_t)id));
		if constexpr (assetType == AssetType::Animation) return choc::value::createObject("AnimationAsset", "Value", choc::value::Value((int64_t)id));
		if constexpr (assetType == AssetType::Skeleton)  return choc::value::createObject("SkeletonAsset", "Value", choc::value::Value((int64_t)id));
	}


	inline choc::value::Value CreateAssetHandleValue(AssetType assetType, AssetHandle id)
	{
		switch (assetType)
		{
			case AssetType::Audio:     return CreateAssetHandleValue<AssetType::Audio>(id);
			case AssetType::Animation: return CreateAssetHandleValue<AssetType::Animation>(id);
			case AssetType::Skeleton:  return CreateAssetHandleValue<AssetType::Skeleton>(id);
			default:
				ZONG_CORE_ASSERT(false, "Unknown asset type!");
				return {};
		}
	}


	template<typename T>
	AssetHandle GetAssetHandleFromValue(const T& v)
	{
		static_assert(std::is_same_v<T, choc::value::Value> || std::is_same_v<T, choc::value::ValueView>, "GetAssetHandleFromValue<T> requires T to be choc::value::Value, or choc::value::ValueView");
		ZONG_CORE_ASSERT(IsAssetHandle(v));

		// Assuming asset handle value stored as int64 [Data]["Value"] member in choc Value object
		return (AssetHandle)(v["Value"].getInt64());
	}


	template<typename T>
	inline void SetAssetHandleValue(T& v, AssetHandle value)
	{
		static_assert(std::is_same_v<T, choc::value::Value> || std::is_same_v<T, choc::value::ValueView>, "SetAssetHandleValue<T> requires T to be choc::value::Value, or choc::value::ValueView");
		ZONG_CORE_ASSERT(IsAssetHandle(v));

		v["Value"].set((int64_t)value);
	}

	inline bool isDigit(char c) { return c >= '0' && c <= '9'; };
	inline bool isSafeIdentifierChar(char c) { return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || c == '_' || isDigit(c); };
	inline bool containsChar(const std::string& s, char c) noexcept
	{
		return s.find(c) != std::string::npos;
	};
	inline auto containsChar(const char* s, char c) noexcept
	{
		if (s == nullptr)
			return false;

		for (; *s != 0; ++s)
			if (*s == c)
				return true;

		return false;
	};

	inline std::string MakeSafeIdentifier(std::string name)
	{
		// TODO: this is taken straight from soul stuff, move this to some sort of utility header
		auto makeSafeIdentifierName = [](std::string s) -> std::string
		{
			for (auto& c : s)
				if (containsChar(" ,./;", c))
					c = '_';

			s.erase(std::remove_if(s.begin(), s.end(), [&](char c) { return !isSafeIdentifierChar(c); }), s.end());

			// Identifiers can't start with a digit
			if (isDigit(s[0]))
				s = "_" + s;

			return s;
		};
		name = makeSafeIdentifierName(name);

		constexpr const char* reservedWords[] =
		{
			"alignas", "alignof", "and", "and_eq", "asm", "atomic_cancel", "atomic_commit", "atomic_noexcept", "auto",
			"bitand", "bitor", "bool", "break", "case", "catch", "char", "char8_t", "char16_t", "char32_t", "class",
			"compl", "concept", "const", "consteval", "constexpr", "constinit", "const_cast", "continue", "co_await",
			"co_return", "co_yield", "decltype", "default", "delete", "do", "double", "dynamic_cast", "else", "enum",
			"explicit", "export", "extern", "false", "float", "for", "friend", "goto", "if", "inline", "int", "long",
			"mutable", "namespace", "new", "noexcept", "not", "not_eq", "nullptr", "operator", "or", "or_eq", "private",
			"protected", "public", "reflexpr", "register", "reinterpret_cast", "requires", "return", "short", "signed",
			"sizeof", "static", "static_assert", "static_cast", "struct", "switch", "synchronized", "template", "this",
			"thread_local", "throw", "true", "try", "typedef", "typeid", "typename", "union", "unsigned", "using",
			"virtual", "void", "volatile", "wchar_t", "while", "xor", "xor_eq"
		};

		for (auto r : reservedWords)
			if (name == r)
				return name + "_";

		return name;
	}

	inline std::string MangleStructOrFunctionName(const std::string& namespacedName)
	{
		return MakeSafeIdentifier(choc::text::replace(namespacedName, ":", "_"));
	}

}
