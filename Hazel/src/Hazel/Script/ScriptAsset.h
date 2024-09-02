#pragma once

#include "Hazel/Asset/Asset.h"

namespace Hazel {

	// TODO(Peter): Consider changing this so that it doesn't have to be an asset
	class ScriptFileAsset : public Asset
	{
	public:
		ScriptFileAsset() = default;
		ScriptFileAsset(const char* classNamespace, const char* className)
			: m_ClassNamespace(classNamespace), m_ClassName(className)
		{
		}

		const std::string& GetClassNamespace() const { return m_ClassNamespace; }
		const std::string& GetClassName() const { return m_ClassName; }

		static AssetType GetStaticType() { return AssetType::ScriptFile; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }

	private:
		std::string m_ClassNamespace = "";
		std::string m_ClassName = "";
	};

}
