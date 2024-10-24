#pragma once

#include "Engine/Asset/Asset.h"
#include "Engine/Editor/NodeGraphEditor/NodeGraphUtils.h"

#include "yaml-cpp/yaml.h"

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include <glm/glm.hpp>

#include "choc/containers/choc_Value.h"

namespace YAML {

	template<>
	struct convert<glm::vec2>
	{
		static Node encode(const glm::vec2& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			return node;
		}

		static bool decode(const Node& node, glm::vec2& rhs)
		{
			if (!node.IsSequence() || node.size() != 2)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec3>
	{
		static Node encode(const glm::vec3& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::vec3& rhs)
		{
			if (!node.IsSequence() || node.size() != 3)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::vec4>
	{
		static Node encode(const glm::vec4& rhs)
		{
			Node node;
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			node.push_back(rhs.w);
			return node;
		}

		static bool decode(const Node& node, glm::vec4& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.x = node[0].as<float>();
			rhs.y = node[1].as<float>();
			rhs.z = node[2].as<float>();
			rhs.w = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<glm::quat>
	{
		static Node encode(const glm::quat& rhs)
		{
			Node node;
			node.push_back(rhs.w);
			node.push_back(rhs.x);
			node.push_back(rhs.y);
			node.push_back(rhs.z);
			return node;
		}

		static bool decode(const Node& node, glm::quat& rhs)
		{
			if (!node.IsSequence() || node.size() != 4)
				return false;

			rhs.w = node[0].as<float>();
			rhs.x = node[1].as<float>();
			rhs.y = node[2].as<float>();
			rhs.z = node[3].as<float>();
			return true;
		}
	};

	template<>
	struct convert<Engine::AssetHandle>
	{
		static Node encode(const Engine::AssetHandle& rhs)
		{
			Node node;
			node.push_back((uint64_t)rhs);
			return node;
		}

		static bool decode(const Node& node, Engine::AssetHandle& rhs)
		{
			rhs = node.as<uint64_t>();
			return true;
		}
	};

	template<>
	struct convert<choc::value::Value>
	{
		static Node encode(const choc::value::ValueView& rhs)
		{
			Node out;
			out.SetStyle(EmitterStyle::Flow);

					if (rhs.isVoid())	{ out["Type"] = "void";		out["Value"] = "null"; }
			else	if (rhs.isString())	{ out["Type"] = "string";	out["Value"] = rhs.get<std::string>(); }
			else	if (rhs.isBool())	{ out["Type"] = "bool";		out["Value"] = rhs.get<bool>(); }
			else	if (rhs.isInt32())	{ out["Type"] = "int";		out["Value"] = rhs.get<int>(); }
			else	if (rhs.isInt64())	{ out["Type"] = "int64";	out["Value"] = rhs.get<int64_t>(); }
			else	if (rhs.isFloat32()){ out["Type"] = "float";	out["Value"] = rhs.get<float>(); }
			else	if (rhs.isFloat64()){ out["Type"] = "double";	out["Value"] = rhs.get<double>(); }
			else	if (rhs.isObject())	{ out["Type"] = "Object";	out["Value"] = encodeObject(rhs); out.SetStyle(EmitterStyle::Block); }
			else	if (rhs.isArray()
					|| rhs.isVector())	{ out["Type"] = "Array";	out["Value"] = encodeArrayOrVector(rhs);
										out.SetStyle(EmitterStyle::Block); }

			return out;
		}

	private:
		static Node encodeArrayOrVector(const choc::value::ValueView& v)
		{
			Node out;

			auto num = v.size();

			for (uint32_t i = 0; i < num; ++i)
			{
				out.push_back(encode(v[i]));
			}

			return out;
		}

		static Node encodeObject(const choc::value::ValueView& object)
		{
			Node out;
			out["ClassName"] = std::string(object.getObjectClassName());

			// We encode/decode UUID as "UUID"{ "Value": uint64_t }
			// So that in the readable YAML it is shown as the correct uint64_t,
			// while for choc::Value it's casted to int64_t
			const bool isUUID = Engine::Utils::IsAssetHandle(object);
			if (isUUID)
			{
				const auto& value = object["Value"];
				ZONG_CORE_ASSERT(value.isInt64());

				out["Value"] = (uint64_t)(value.get<int64_t>());
			}
			else
			{
				Node members;

				auto numMembers = object.size();

				for (uint32_t i = 0; i < numMembers; ++i)
				{
					auto member = object.getObjectMemberAt(i);
					members[member.name] = encode(member.value);
				}

				out["Members"] = members;
			}

			return out;
		}

	public:
		static bool decode(const Node& node, choc::value::Value& rhs)
		{
			if (!node.IsMap() || !node["Type"] || !node["Value"])
				return false;

			rhs = choc::value::Value();
			
			auto type = node["Type"].as<std::string>();

			// TODO: JP. perhaps we should assign 'void' Value as a fallback?

					if (type == "void")		rhs = choc::value::Value();
			else	if (type == "string")	rhs = choc::value::Value(node["Value"].as<std::string>());
			else	if (type == "bool")		rhs = choc::value::Value(node["Value"].as<bool>());
			else	if (type == "int")		rhs = choc::value::Value(node["Value"].as<int>());
			else	if (type == "int64")	rhs = choc::value::Value(node["Value"].as<int64_t>());
			else	if (type == "float")	rhs = choc::value::Value(node["Value"].as<float>());
			else	if (type == "double")	rhs = choc::value::Value(node["Value"].as<double>());
			else	if (type == "Object")	return decodeObject(node["Value"], rhs);
			else	if (type == "Array")	return decodeArrayOrVector(node["Value"], rhs);
			else
			{
				ZONG_CORE_ASSERT(false);
				return false;
			}

			return true;
		}

	private:
		static bool decodeArrayOrVector(const Node& node, choc::value::Value& rhs)
		{

			Node out;
			Node elements;

			auto num = (uint32_t)node.size();

			bool success = true;

			auto getElementValue = [&node, &success](uint32_t index)
			{
				choc::value::Value element;

				if (!decode(node[index], element))
					success = false;

				ZONG_CORE_ASSERT(success);

				return element;
			};

			// TODO: JP. for not we don't distinguish types of the array,
			//		so uniform arrays can be deserialized as complex arrays
			rhs = choc::value::createArray(num, getElementValue);

			return success;
		}

		static bool decodeObject(const Node& node, choc::value::Value& rhs)
		{
			if (!node.IsMap() || !node["ClassName"])
				return false;

			rhs = choc::value::createObject(node["ClassName"].as<std::string>());

			// We encode/decode UUID as "UUID"{ "Value": uint64_t }
			// So that in the readable YAML it is shown as the correct uint64_t,
			// while for choc::Value it's casted to int64_t
			const bool isUUID = Engine::Utils::IsAssetHandle(rhs) && node["Value"];
			if (isUUID)
			{
				rhs.addMember("Value", choc::value::createInt64((int64_t)node["Value"].as<uint64_t>()));
			}
			else
			{
				Node members = node["Members"];
				if (!members)
					return false;

				for (auto member : members)
				{
					choc::value::Value memberValue;
					if (!decode(member.second, memberValue))
						return false;

					rhs.addMember(member.first.as<std::string>(), memberValue);
				}
			}

			return true;
		}
	};

}

namespace Engine {

	inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec2& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << YAML::EndSeq;
		return out;
	}

	inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec3& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}


	inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::vec4& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.x << v.y << v.z << v.w << YAML::EndSeq;
		return out;
	}

	inline YAML::Emitter& operator<<(YAML::Emitter& out, const glm::quat& v)
	{
		out << YAML::Flow;
		out << YAML::BeginSeq << v.w << v.x << v.y << v.z << YAML::EndSeq;
		return out;
	}


	inline YAML::Emitter& operator<<(YAML::Emitter& out, const choc::value::Value& v)
	{
		out << YAML::convert<choc::value::Value>::encode(v);
		return out;
	}

	inline YAML::Emitter& operator<<(YAML::Emitter& out, const choc::value::ValueView& v)
	{
		out << YAML::convert<choc::value::Value>::encode(v);
		return out;
	}

}
