#pragma once

#define ZONG_SERIALIZE_PROPERTY(propName, propVal, outputNode) outputNode << YAML::Key << #propName << YAML::Value << propVal

#define ZONG_SERIALIZE_PROPERTY_ASSET(propName, propVal, outputData) outputData << YAML::Key << #propName << YAML::Value << (propVal ? (uint64_t)propVal->Handle : 0);

#define ZONG_DESERIALIZE_PROPERTY(propertyName, destination, node, defaultValue)	\
if (node.IsMap())																\
{																				\
	if (auto foundNode = node[#propertyName])									\
	{																			\
		try																		\
		{																		\
			destination = foundNode.as<decltype(defaultValue)>();				\
		}																		\
		catch (const std::exception& e)											\
		{																		\
			ZONG_CONSOLE_LOG_ERROR(e.what());										\
																				\
			destination = defaultValue;											\
		}																		\
	}																			\
	else																		\
	{																			\
		destination = defaultValue;												\
	}																			\
}																				\
else																			\
{																				\
	destination = defaultValue;													\
}

#define ZONG_DESERIALIZE_PROPERTY_ASSET(propName, destination, inputData, assetClass)\
		{AssetHandle assetHandle = inputData[#propName] ? inputData[#propName].as<uint64_t>() : 0;\
		if (AssetManager::IsAssetHandleValid(assetHandle))\
		{ destination = AssetManager::GetAsset<assetClass>(assetHandle); }\
		else\
		{ ZONG_CORE_ERROR_TAG("AssetManager", "Tried to load invalid asset {0}.", #assetClass); }}
