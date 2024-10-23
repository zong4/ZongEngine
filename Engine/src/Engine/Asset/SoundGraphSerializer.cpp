#include "hzpch.h"
#include "SoundGraphSerializer.h"

#include "GraphSerializer.h"

#include "Engine/Asset/AssetManager.h"

#include "Engine/Audio/SoundGraph/Utils/SoundGraphCache.h"

#include "Engine/Editor/NodeGraphEditor/SoundGraph/SoundGraphAsset.h"
//? not ideal, but need access to the EnumRegistry, alternatively could add
//? virtual GetEnumTokens callback to GraphSerializer class
#include "Engine/Editor/NodeGraphEditor/SoundGraph/SoundGraphNodes.h"

#include "Engine/Utilities/SerializationMacros.h"
#include "Engine/Utilities/YAMLSerializationHelpers.h"

#include <choc/text/choc_JSON.h>


static constexpr auto MAX_NUM_CACHED_GRAPHS = 256; // TODO: get this value from somewhere reasonable

namespace SG = Hazel::SoundGraph;

namespace Hazel {

	SoundGraphGraphSerializer::SoundGraphGraphSerializer()
	{
		m_Cache = CreateScope<SoundGraphCache>(MAX_NUM_CACHED_GRAPHS);
	}


	SoundGraphGraphSerializer::~SoundGraphGraphSerializer() = default;


	void SoundGraphGraphSerializer::Serialize(const AssetMetadata& metadata, const Ref<Asset>& asset) const
	{
		Ref<SoundGraphAsset> graph = asset.As<SoundGraphAsset>();

		// Out
		std::ofstream fout(Project::GetEditorAssetManager()->GetFileSystemPath(metadata));
		HZ_CORE_VERIFY(fout.good());
		std::string yamlString = SerializeToYAML(graph);
		fout << yamlString;
	}


	bool SoundGraphGraphSerializer::TryLoadData(const AssetMetadata& metadata, Ref<Asset>& asset) const
	{
		const auto filepath = Project::GetEditorAssetManager()->GetFileSystemPath(metadata);
		std::ifstream stream(filepath);
		if (!stream.is_open())
		{
			HZ_CONSOLE_LOG_ERROR("Failed to open SoundGraph asset file to deserialize: {}", filepath.string());
			return false;
		}

		std::stringstream strStream;
		strStream << stream.rdbuf();

		Ref<SoundGraphAsset> soundGraph;
		if (!DeserializeFromYAML(strStream.str(), soundGraph))
		{
			HZ_CONSOLE_LOG_ERROR("Failed to deserialize SoundGraph: {}", filepath.string());
			return false;
		}

		if (!soundGraph->CachedPrototype.empty() && !soundGraph->Prototype)
		{
			// TODO: need to set new cache directory when new project is loaded
			const std::filesystem::path cacheDirectory = Project::GetCacheDirectory() / "SoundGraph";
			m_Cache->SetCacheDirectory(cacheDirectory);

			const std::string nameOld = metadata.FilePath.stem().string();
			// Cherno Hack
			std::string name = soundGraph->CachedPrototype.stem().string();
			if (name.find("sound_graph_cache_") != std::string::npos)
				name = name.substr(18);

			soundGraph->Prototype = m_Cache->ReadPtototype(name.c_str());
		}

		//? Temporary hack. If we've compiled graph in Editor, we want to keep Prototype alive to play in editor
		// this is for just compiled assets
		if (asset && !soundGraph->Prototype)
		{
			if (Ref<SG::Prototype> prototype = asset.As<SoundGraphAsset>()->Prototype)
				soundGraph->Prototype = prototype;
		}

		asset = soundGraph;
		asset->Handle = metadata.Handle;
		return true;
	}


	bool SoundGraphGraphSerializer::SerializeToAssetPack(AssetHandle handle, FileStreamWriter& stream, AssetSerializationInfo& outInfo) const
	{
		Ref<SoundGraphAsset> soundGraph = AssetManager::GetAsset<SoundGraphAsset>(handle);

		std::string yamlString = SerializeToYAML(soundGraph);
		outInfo.Offset = stream.GetStreamPosition();
		stream.WriteString(yamlString);	// TODO: JP. Why do we serialize YAML string to asset pack? In Runtime we should only use Graph Prototypes.
		outInfo.Size = stream.GetStreamPosition() - outInfo.Offset;
		return true;
	}


	Ref<Asset> SoundGraphGraphSerializer::DeserializeFromAssetPack(FileStreamReader& stream, const AssetPackFile::AssetInfo& assetInfo) const
	{
		stream.SetStreamPosition(assetInfo.PackedOffset);
		std::string yamlString;
		stream.ReadString(yamlString);

		Ref<SoundGraphAsset> soundGraph;
		bool result = DeserializeFromYAML(yamlString, soundGraph);
		if (!result)
			return nullptr;

		if (!soundGraph->CachedPrototype.empty() && !soundGraph->Prototype)
		{
			// TODO: need to set new cache directory when new project is loaded
			const std::filesystem::path cacheDirectory = Project::GetCacheDirectory() / "SoundGraph";
			m_Cache->SetCacheDirectory(cacheDirectory);

			const std::string name = soundGraph->CachedPrototype.stem().string();
			soundGraph->Prototype = m_Cache->ReadPtototype(name.c_str());
		}

		//? Temporary hack. If we've compiled graph in Editor, we want to keep Prototype alive to play in editor
		// this is for just compiled assets
		if (soundGraph && !soundGraph->Prototype)
		{
			if (Ref<SG::Prototype> prototype = soundGraph->Prototype)
				soundGraph->Prototype = prototype;
		}

		return soundGraph;
	}


	std::string SoundGraphGraphSerializer::SerializeToYAML(Ref<SoundGraphAsset> graph) const
	{
		YAML::Emitter out;

		out << YAML::BeginMap; // Nodes, Links, Graph State

		// Nodes
		DefaultGraphSerializer::SerializeNodes(out, graph->Nodes);

		// Links
		DefaultGraphSerializer::SerializeLinks(out, graph->Links);

		// Graph IO
		HZ_SERIALIZE_PROPERTY(GraphInputs, graph->GraphInputs.ToExternalValue(), out);
		HZ_SERIALIZE_PROPERTY(GraphOutputs, graph->GraphOutputs.ToExternalValue(), out);
		HZ_SERIALIZE_PROPERTY(LocalVariables, graph->LocalVariables.ToExternalValue(), out);

		// Graph State
		HZ_SERIALIZE_PROPERTY(GraphState, graph->GraphState, out);

		// Wave Sources
		out << YAML::Key << "Waves" << YAML::Value;
		out << YAML::BeginSeq;
		for (const auto& wave : graph->WaveSources)
		{
			out << YAML::BeginMap; // wave
			{
				HZ_SERIALIZE_PROPERTY(WaveAssetHandle, wave, out);
			}
			out << YAML::EndMap; // wave
		}
		out << YAML::EndSeq; // Waves

		//? should probably serialize it as binary
		// Graph Prototype
#if 0
		if (graph->Prototype)
		{
			out << YAML::Key << "Prototype" << YAML::Value;
			out << YAML::BeginMap; // Prototype

			const auto& prototype = graph->Prototype;

			HZ_SERIALIZE_PROPERTY(PrototypeName, prototype->DebugName, out);
			HZ_SERIALIZE_PROPERTY(PrototypeID, prototype->ID, out);

			auto serializeEndpoint = [&out](const SG::Prototype::Endpoint& endpoint)
			{
				out << YAML::BeginMap; // endpoing
				{
					HZ_SERIALIZE_PROPERTY(EndpointID, (uint32_t)endpoint.EndpointID, out);
					HZ_SERIALIZE_PROPERTY(DefaultValue, choc::json::toString(endpoint.DefaultValue), out); // TODO: need to store type
				}
				out << YAML::EndMap; // endpoint
			};

			auto serializeSequence = [&out](const char* sequenceName, auto serializationFunction)
			{
				out << YAML::Key << sequenceName << YAML::Value;
				out << YAML::BeginSeq;
				{
					serializationFunction();
				}
				out << YAML::EndSeq; // Inputs
			};

			serializeSequence("Inputs", [&]
			{
				for (const auto& in : prototype->Inputs)
					serializeEndpoint(in);
			});

			serializeSequence("Outputs", [&]
			{
				for (auto& out : prototype->Outputs)
				{
					serializeEndpoint(out);
				}
			});

			serializeSequence("OutputChannelIDs", [&]
			{
				for (const auto& id : prototype->OutputChannelIDs)
				{
					out << YAML::BeginMap;
					HZ_SERIALIZE_PROPERTY(ID, (uint32_t)id, out);
					out << YAML::EndMap;
				}
			});

			serializeSequence("Nodes", [&]
			{
				for (const auto& node : prototype->Nodes)
				{
					out << YAML::BeginMap;
					HZ_SERIALIZE_PROPERTY(NodeTypeID, (uint32_t)node.NodeTypeID, out);
					HZ_SERIALIZE_PROPERTY(ID, (uint64_t)node.ID, out);

					serializeSequence("DefaultValuePlugs", [&]
					{
						for (const auto& valuePlug : node.DefaultValuePlugs)
						{
							serializeEndpoint(valuePlug);
						}
					});
					out << YAML::EndMap;
				}
			});

			serializeSequence("Connections", [&]
			{
				for (const auto& connection : prototype->Connections)
				{
					out << YAML::BeginMap; // connection
					{
						out << YAML::Key << "Source" << YAML::Value;
						out << YAML::BeginMap; // endpoing
						{
							HZ_SERIALIZE_PROPERTY(NodeID, (uint64_t)connection.Source.NodeID, out);
							HZ_SERIALIZE_PROPERTY(EndpointID, (uint32_t)connection.Source.EndpointID, out);
						}
						out << YAML::EndMap; // endpoint

						out << YAML::Key << "Destination" << YAML::Value;
						out << YAML::BeginMap; // endpoing
						{
							HZ_SERIALIZE_PROPERTY(NodeID, (uint64_t)connection.Destination.NodeID, out);
							HZ_SERIALIZE_PROPERTY(EndpointID, (uint32_t)connection.Destination.EndpointID, out);
						}
						out << YAML::EndMap; // endpoint

						HZ_SERIALIZE_PROPERTY(Type, (uint32_t)connection.Type, out);
					}
					out << YAML::EndMap; // connection
				}
			});

			out << YAML::EndMap; // Prototype
		}
#endif
		const std::string pathStr = graph->CachedPrototype.empty() ? std::string("") : graph->CachedPrototype.string();
		HZ_SERIALIZE_PROPERTY(CachedPrototype, pathStr, out);

		out << YAML::EndMap; // Nodes, Links, Graph State, Waves, Prototype

		return std::string(out.c_str());
	}


	bool SoundGraphGraphSerializer::DeserializeFromYAML(const std::string& yamlString, Ref<SoundGraphAsset>& graph) const
	{
		YAML::Node data;

		try
		{
			data = YAML::Load(yamlString);
		}
		catch (const std::exception& e)
		{
			HZ_CONSOLE_LOG_FATAL(e.what());
			return false;
		}

		graph = Ref<SoundGraphAsset>::Create();

		// Graph IO (must be deserialized before nodes to be able to spawn IO nodes)
		{
			choc::value::Value graphInputsValue;
			choc::value::Value graphOutputsValue;
			choc::value::Value graphLocalVariablesValue;

			HZ_DESERIALIZE_PROPERTY(GraphInputs, graphInputsValue, data, choc::value::Value());
			HZ_DESERIALIZE_PROPERTY(GraphOutputs, graphOutputsValue, data, choc::value::Value());
			HZ_DESERIALIZE_PROPERTY(LocalVariables, graphLocalVariablesValue, data, choc::value::Value());

			graph->GraphInputs = Utils::PropertySet::FromExternalValue(graphInputsValue);
			graph->GraphOutputs = Utils::PropertySet::FromExternalValue(graphOutputsValue);
			graph->LocalVariables = Utils::PropertySet::FromExternalValue(graphLocalVariablesValue);
		}

		using PinCandidate = DefaultGraphSerializer::DeserializationFactory::PinCandidate;
		using NodeCandidate = DefaultGraphSerializer::DeserializationFactory::NodeCandidate;

		// Nodes
		auto constructNode = [&graph](const NodeCandidate& candidate, const std::optional<std::vector<PinCandidate>>& inputs, const std::optional<std::vector<PinCandidate>>& outputs)
		{
			// graph IO, Local Variables
			auto optType = magic_enum::enum_cast<EPropertyType>(choc::text::replace(candidate.Category, " ", ""));

			Node* newNode = nullptr;

			if (optType.has_value())
			{
				auto getLocalVariableName = [&]()
				{
					HZ_CORE_ASSERT(inputs.has_value() || outputs.has_value())
					std::string name;
					if (outputs && !outputs->empty())    name = outputs->at(0).Name;
					else if (inputs && !inputs->empty()) name = inputs->at(0).Name;
					HZ_CORE_ASSERT(!name.empty());
					return name;
				};

				switch (*optType)
				{
					case EPropertyType::Input: newNode = SG::SoundGraphNodeFactory::SpawnGraphPropertyNode(graph, getLocalVariableName(), *optType); break;
					case EPropertyType::Output: newNode = SG::SoundGraphNodeFactory::SpawnGraphPropertyNode(graph, getLocalVariableName(), *optType); break;
					case EPropertyType::LocalVariable: newNode = SG::SoundGraphNodeFactory::SpawnGraphPropertyNode(graph, candidate.Name, *optType, (bool)candidate.NumOutputs); break;
					default:
						HZ_CORE_ASSERT(false, "Deserialized invalid SoundGraph property type.");
						break;
				}
			}
			else
			{
				if (candidate.Type == NodeType::Comment)
				{
					// Comments are special case where the name is editable and is displayed in the comment header
					//! Alternatively we could make a special pin to use as the header for the comment node, just to keep this logic more generic.
					newNode = SG::SoundGraphNodeFactory::SpawnNodeStatic(candidate.Category, magic_enum::enum_name<NodeType::Comment>());
					newNode->Name = candidate.Name;
				}
				else
				{
					newNode = SG::SoundGraphNodeFactory::SpawnNodeStatic(candidate.Category, candidate.Name);
				}
			}

			HZ_CORE_ASSERT(!newNode ^ (newNode->Type == candidate.Type));

			return newNode;
		};

		auto deserializePin = [](const PinCandidate& candidate, Pin* factoryPin, const NodeCandidate& node)
		{
			auto optType = magic_enum::enum_cast<SG::SGTypes::ESGPinType>(candidate.GetTypeString());

			if (candidate.GetTypeString() == "Object")
				optType = SG::SGTypes::ESGPinType::AudioAsset;

			if (!optType.has_value())
				return false;

			// Check the type is correct
			const SG::SGTypes::ESGPinType candidateType = *optType;

			if (candidateType != factoryPin->GetType())
			{
				const std::string_view candidateTypeStr = magic_enum::enum_name<SG::SGTypes::ESGPinType>(candidateType);
				const std::string_view factoryPinTypeStr = factoryPin->GetTypeString();

				HZ_CONSOLE_LOG_ERROR("Pin type of the deserialized Node Pin ({0} - '{1}' ({2})) does not match the Pin type of the factory Node Pin ({0} - '{3}' ({4})).",
									node.Name, candidate.Name, candidateTypeStr, factoryPin->Name, factoryPinTypeStr);
				return false;
			}
			else if (candidate.Name != factoryPin->Name)
			{
				// TODO: JP. this may not be correct when/if we change the Comment node to use Pin to display text instead of Node name
				HZ_CONSOLE_LOG_ERROR("Pin name of the deserialized Node Pin ({0} - '{1}') does not match the Pin name of the factory Node Pin ({0} - '{2}').", node.Name, candidate.Name, factoryPin->Name);
				return false;
			}

			// Flow or Audio types are not serialized
			if (factoryPin->IsType(SG::SGTypes::ESGPinType::Flow) || factoryPin->IsType(SG::SGTypes::ESGPinType::Audio))
				return true;

			//? (Old JSON serialization) Deserialize value from string
#if 0
			const bool isLegacyCustomValueType = choc::text::contains(valueStr, "Value");
			const bool isCustomValueType = !isLegacyCustomValueType && choc::text::contains(valueStr, "Data");
			const bool isBasicChocValue = !isLegacyCustomValueType && !isCustomValueType;

			auto parseBasicValue = [&pin, type](std::string_view valueString) -> choc::value::Value
			{
				choc::value::Value value = choc::json::parseValue(valueString);

				// choc json arithmetic value type is just 'number' (double),
				// we need to cast it to the correct type

				if (pin->Storage == StorageKind::Value && !value.isString() && !value.isVoid()) // string can be name of connected Graph Input
				{
					switch (type)
					{
						case SG::Types::EPinType::Int: value = choc::value::createInt32(value.get<int32_t>()); break;
						case SG::Types::EPinType::Float: value = choc::value::createFloat32(value.get<float>()); break;
						default:
							break;
					}
				}
				else if (pin->Storage == StorageKind::Array && value.isArray() && value.size() && !value[0].isString())
				{
					for (auto& element : value)
					{
						switch (type)
						{
							case SG::Types::EPinType::Int: element = choc::value::createInt32(element.get<int32_t>()); break;
							case SG::Types::EPinType::Float: element = choc::value::createFloat32(element.get<float>()); break;
							default:
								break;
						}
					}
				}

				return value;
			};

			auto parseCustomValueObject = [&pin](std::string_view valueString) -> choc::value::Value
			{
				const choc::value::Value valueObj = choc::json::parseValue(valueString);

				// TODO: choc value to json overwrites class name with "JSON", instead of json, parse Value object to/from YAML

				if (valueObj.getType() == pin->Value.getType())
				{
					return valueObj;
				}
				else
				{
					// This can trigger if we've trying to load old version of a Node.
					HZ_CORE_ASSERT(false, "Type of the deserialized Node Pin does not match the type of the factory Node Pin.");

					return choc::value::Value();
				}
			};

			auto parseLegacyCustomValueType = [](std::string_view valueString) -> choc::value::Value
			{
				choc::value::Value value = choc::json::parse(valueString);

				if (value["TypeName"].isVoid())
				{
					HZ_CORE_ASSERT(false, "Failed to deserialize custom value type, missing \"TypeName\" property.");
					return {};
				}

				choc::value::Value customObject = choc::value::createObject(value["TypeName"].get<std::string>());
				if (value.isObject())
				{
					for (uint32_t i = 0; i < value.size(); i++)
					{
						choc::value::MemberNameAndValue nameValue = value.getObjectMemberAt(i);
						customObject.addMember(nameValue.name, nameValue.value);
					}
				}
				else
				{
					HZ_CORE_ASSERT(false, "Failed to load custom value type. It must be serialized as object.")
				}

				return customObject;
			};

			const choc::value::Value deserializedValue = isBasicChocValue ? parseBasicValue(valueStr)
				: isCustomValueType ? parseCustomValueObject(valueStr)
				: parseLegacyCustomValueType(valueStr);
#endif

			// TODO: JP. for now we use 'void' and 'string' for connected pins (this is needed for graph parsing,
			//           but perhaps we could assign 'void' type for the compilation step only?)
			//           note: AudioAsset is "AudioAsset" type in the asset, but int64_t in the factory
			if (candidate.Value.getType() == factoryPin->Value.getType()
				|| Utils::IsAssetHandle<AssetType::Audio>(candidate.Value)  // TEMP
				|| candidate.Value.isObjectWithClassName("TSGEnum")         // TEMP
				|| candidate.Value.isVoid() || candidate.Value.isString())
			{
				factoryPin->Value = candidate.Value;
				return true;
			}
			else
			{
				// This can trigger if we've trying to load old version of a Node.
				HZ_CONSOLE_LOG_ERROR("Value type of the deserialized Node Pin ({0} - '{1}') does not match the Value type of the factory Node Pin ({0} - '{2}').", node.Name, candidate.Name, factoryPin->Name);
				return false;
			}
		};

		DefaultGraphSerializer::DeserializationFactory factory{ constructNode, deserializePin };

		try
		{
			DefaultGraphSerializer::TryLoadNodes(data, graph->Nodes, factory);
		}
		catch (const std::exception& e)
		{
			HZ_CONSOLE_LOG_FATAL(e.what());

			graph = Ref<SoundGraphAsset>::Create();
			return false;
		}

		// Assign enum tokens to pins
		for (auto& node : graph->Nodes)
		{
			for (auto& pin : node->Inputs)
			{
				if (pin->IsType(SG::SGTypes::ESGPinType::Enum))
				{
					if (const auto* tokens = SG::SoundGraphNodeFactory::GetEnumTokens(node->Name, pin->Name))
						pin->EnumTokens = tokens;
				}
			}
		}

		// Links
		DefaultGraphSerializer::TryLoadLinks(data, graph->Links);

		// Graph State
		HZ_DESERIALIZE_PROPERTY(GraphState, graph->GraphState, data, std::string());

		// Wave Sources
		for (auto wave : data["Waves"])
		{
			UUID ID;

			HZ_DESERIALIZE_PROPERTY(WaveAssetHandle, ID, wave, uint64_t(0));
			graph->WaveSources.push_back(ID);
		}

		// Graph Prototype
		HZ_DESERIALIZE_PROPERTY(CachedPrototype, graph->CachedPrototype, data, std::string());
		return true;
	}

} // namespace Hazel
