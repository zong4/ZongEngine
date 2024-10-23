#include "hzpch.h"
#include "SoundGraphNodes.h"

#include "Engine/Utilities/StringUtils.h"

#include <array>

namespace Alias = Hazel::SoundGraph::NameAliases;
namespace SG = Hazel::SoundGraph;

namespace Hazel::SoundGraph {

	//==========================================================================
	/// Base Nodes
	namespace Base {
		static Node* Input_Action()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");
			auto* node = new SGTypes::SGNode(nodeName.c_str(), ImColor(255, 128, 128));
			node->Outputs.push_back(SGTypes::CreatePinForType(SGTypes::ESGPinType::Flow, "Play"));
			return node;
		}

		static Node* Output_Audio()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");
			auto* node = new SGTypes::SGNode(nodeName.c_str(), ImColor(255, 128, 128));
			node->Inputs.push_back(SGTypes::CreatePinForType(SGTypes::ESGPinType::Audio, "Left"));
			node->Inputs.push_back(SGTypes::CreatePinForType(SGTypes::ESGPinType::Audio, "Right"));
			return node;
		}

		static Node* On_Finished()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");
			auto* node = new SGTypes::SGNode(nodeName.c_str(), ImColor(255, 128, 128));
			node->Inputs.push_back(SGTypes::CreatePinForType(SGTypes::ESGPinType::Flow, "On Finished"));
			return node;
		}
	}

	namespace Utility {
		static Node* Comment()
		{
			const std::string nodeName = choc::text::replace(__func__, "_", " ");

			auto* node = new SGTypes::SGNode(nodeName.c_str());
			node->Category = "Utility";

			node->Type = NodeType::Comment;
			node->Color = ImColor(255, 255, 255, 20);
			node->Size = ImVec2(300, 200);

			return node;
		}
	}

	template<class TNodeType>
	static Node* CreateNode(std::string_view name, const ImColor& color, NodeType type)
	{
		static_assert(DescribedNode<TNodeType>::value, "NodeProcessor type must be described.");
		Node* node = SoundGraphNodeFactory::CreateNodeForProcessor<TNodeType>();
		node->Name = name;
		node->Color = color;
		node->Type = type;

		auto changePinType = [](Pin*& pin, SGTypes::ESGPinType newType)
		{
			if (pin->GetType() == newType)
				return;

			// create new pin of the new type
			Pin* newPin = SGTypes::CreatePinForType(newType);

			// copy data from the old pin to the new pin
			*newPin = *pin;

			// delete old pin
			delete (pin);

			// emplace new pin instead of the old pin
			pin = newPin;
		};

		// set up pin type overrides for the alias,
		// might want to move this somewhere else at some point
		if (name == Alias::AddAudio || name == Alias::MultAudio || name == Alias::SubtractAudio
			|| name == Alias::MinAudio || name == Alias::MaxAudio || name == Alias::ClampAudio)
		{
			for (auto& in : node->Inputs)
				changePinType(in, SGTypes::ESGPinType::Audio);
			for (auto& out : node->Outputs)
				changePinType(out, SGTypes::ESGPinType::Audio);
		}
		else if (name == Alias::AddFloatAudio || name == Alias::MultAudioFloat || name == Alias::MapRangeAudio)
		{
			changePinType(node->Inputs[0], SGTypes::ESGPinType::Audio);
			changePinType(node->Outputs[0], SGTypes::ESGPinType::Audio);
		}

		//? this shouldn't be here
		if (name == Alias::MapRangeAudio)
		{
			node->Inputs[1]->Value = choc::value::Value(-1.0f);
			node->Inputs[2]->Value = choc::value::Value(1.0f);
			node->Inputs[3]->Value = choc::value::Value(-1.0f);
			node->Inputs[4]->Value = choc::value::Value(1.0f);
		}

		return node;
	}

	template<class TNodeType>
	static std::pair<std::string, std::function<Node* ()>> CreateRegistryEntry(std::string_view typeName, const ImColor& colour, NodeType type, bool isAlias = false)
	{
		const std::string name = isAlias ? std::string(typeName) : Utils::CreateUserFriendlyTypeName(typeName);
		const auto constructor = [=] { return CreateNode<TNodeType>(name, colour, type); };
		return { name, constructor };
	}

	namespace Colors {
		static const ImU32 Float = SGTypes::GetPinColor(SGTypes::ESGPinType::Float);
		static const ImU32 Int = ImColor(128, 195, 248);// SGTypes::GetPinColor(SGTypes::ESGPinType::Int);
		static const ImU32 Audio = SGTypes::GetPinColor(SGTypes::ESGPinType::Audio);
		static const ImU32 Music = ImColor(255, 89, 183);
		static const ImU32 Trigger = ImColor(232, 239, 255); // SGTypes::GetPinColour(SGTypes::ESGPinType::Flow);
		static const ImU32 AudioAsset = SGTypes::GetPinColor(SGTypes::ESGPinType::AudioAsset);
		static const ImU32 Noise = IM_COL32(240, 235, 113, 255);
		static const ImU32 Envelope = IM_COL32(217, 196, 255, 255);
		static const ImU32 WavePlayer = IM_COL32(54, 207, 145, 255);
	}

#define DECLARE_PIN_TYPE(MemberPointer, PinTypeOverride) { Nodes::SanitizeMemberName(#MemberPointer, true),  PinTypeOverride }
#define DECLARE_DEF_VALUE(MemberPointer, ValueOverride) { Nodes::SanitizeMemberName(#MemberPointer, true), choc::value::Value(ValueOverride) }

	// TODO: add static_assert "Described"
	static const std::unordered_map<std::string, SGTypes::ESGPinType> PinTypeOverrides =
	{
		DECLARE_PIN_TYPE(WavePlayer::out_OutLeft,	SGTypes::ESGPinType::Audio),
		DECLARE_PIN_TYPE(WavePlayer::out_OutRight,	SGTypes::ESGPinType::Audio),
		DECLARE_PIN_TYPE(Noise::in_Type,			SGTypes::ESGPinType::Enum),
		DECLARE_PIN_TYPE(Noise::out_Value,			SGTypes::ESGPinType::Audio),
		DECLARE_PIN_TYPE(Sine::out_Sine,			SGTypes::ESGPinType::Audio),
	};

	// TODO: should this info be just stored inside of the NodeProcessor?
	static const std::unordered_map<std::string, choc::value::Value> DefaultPinValues =
	{
		DECLARE_DEF_VALUE(WavePlayer::in_StartTime, 0.0f),
		DECLARE_DEF_VALUE(WavePlayer::in_Loop, false),
		DECLARE_DEF_VALUE(WavePlayer::in_NumberOfLoops, int32_t(-1)),
		DECLARE_DEF_VALUE(GetRandom<float>::in_Seed, int32_t(-1)),
		DECLARE_DEF_VALUE(GetRandom<int>::in_Seed, int32_t(-1)),
		DECLARE_DEF_VALUE(GetRandom<int64_t>::in_Seed, int32_t(-1)),
		DECLARE_DEF_VALUE(Random<float>::in_Seed, int32_t(-1)),
		DECLARE_DEF_VALUE(Random<int>::in_Seed, int32_t(-1)),
		DECLARE_DEF_VALUE(Noise::in_Seed, int32_t(-1)),
		DECLARE_DEF_VALUE(Sine::in_Frequency, float(440.0f)),
		DECLARE_DEF_VALUE(Sine::in_PhaseOffset, float(0.0f)),

		DECLARE_DEF_VALUE(RepeatTrigger::in_Period, 0.2f),
		DECLARE_DEF_VALUE(TriggerCounter::in_StartValue, 0.0f),
		DECLARE_DEF_VALUE(TriggerCounter::in_StepSize, 1.0f),
		DECLARE_DEF_VALUE(TriggerCounter::in_ResetCount, int(0)),

		DECLARE_DEF_VALUE(ADEnvelope::in_AttackTime, 1.0f),
		DECLARE_DEF_VALUE(ADEnvelope::in_DecayTime, 1.0f),
		DECLARE_DEF_VALUE(ADEnvelope::in_AttackCurve, 1.0f),
		DECLARE_DEF_VALUE(ADEnvelope::in_DecayCurve, 1.0f),

		DECLARE_DEF_VALUE(BPMToSeconds::in_BPM, 90.0f),
		DECLARE_DEF_VALUE(NoteToFrequency<float>::in_MIDINote, 60.0f),
		DECLARE_DEF_VALUE(NoteToFrequency<int>::in_MIDINote, int(60)),
		DECLARE_DEF_VALUE(FrequencyToNote::in_Frequency, 440.0f),

		DECLARE_DEF_VALUE(LinearToLogFrequency::in_Value, 0.5f),
		DECLARE_DEF_VALUE(LinearToLogFrequency::in_Min, 0.0f),
		DECLARE_DEF_VALUE(LinearToLogFrequency::in_Max, 1.0f),
		DECLARE_DEF_VALUE(LinearToLogFrequency::in_MinFrequency, 20.0f),
		DECLARE_DEF_VALUE(LinearToLogFrequency::in_MaxFrequency, 20000.0f),

		DECLARE_DEF_VALUE(FrequencyLogToLinear::in_Frequency, 1000.0f),
		DECLARE_DEF_VALUE(FrequencyLogToLinear::in_MinFrequency, 20.0f),
		DECLARE_DEF_VALUE(FrequencyLogToLinear::in_MaxFrequency, 20000.0f),
		DECLARE_DEF_VALUE(FrequencyLogToLinear::in_Min, 0.0f),
		DECLARE_DEF_VALUE(FrequencyLogToLinear::in_Max, 1.0f),

		DECLARE_DEF_VALUE(MapRange<int>::in_In, 0),
		DECLARE_DEF_VALUE(MapRange<int>::in_InRangeA, 0),
		DECLARE_DEF_VALUE(MapRange<int>::in_InRangeB, 100),
		DECLARE_DEF_VALUE(MapRange<int>::in_OutRangeA, 0),
		DECLARE_DEF_VALUE(MapRange<int>::in_OutRangeB, 100),

		DECLARE_DEF_VALUE(MapRange<float>::in_In, 0.0f),
		DECLARE_DEF_VALUE(MapRange<float>::in_InRangeA, 0.0f),
		DECLARE_DEF_VALUE(MapRange<float>::in_OutRangeA, 0.0f),
	};

#undef DECLARE_PIN_TYPE
#undef DECLARE_DEF_VALUE


	namespace EnumTokens {

		static const std::vector<Token> NoiseType
		{
			{ "WhiteNoise",    Noise::ENoiseType::WhiteNoise },
			{ "PinkNoise",     Noise::ENoiseType::PinkNoise },
			{ "BrownianNoise", Noise::ENoiseType::BrownianNoise }
		};

	} // namespace EnumTokens


	std::optional<choc::value::Value> GetPinDefaultValueOverride(std::string_view nodeName, std::string_view memberNameSanitized)
	{
		// Passed in names must not contain namespace
		HZ_CORE_ASSERT(nodeName.find("::") == std::string_view::npos);
		HZ_CORE_ASSERT(memberNameSanitized.find("::") == std::string_view::npos);

		const std::string fullName = choc::text::joinStrings<std::array<std::string_view, 2>>({ nodeName, memberNameSanitized }, "::");
		if (!DefaultPinValues.count(fullName))
		{
			//return choc::value::Value(1);
			return {};
		}

		return DefaultPinValues.at(fullName);
	}


	std::optional<SGTypes::ESGPinType> GetPinTypeForMemberOverride(std::string_view nodeName, std::string_view memberNameSanitized)
	{
		// Passed in names must not contain namespace
		HZ_CORE_ASSERT(nodeName.find("::") == std::string_view::npos);
		HZ_CORE_ASSERT(memberNameSanitized.find("::") == std::string_view::npos);

		const std::string fullName = choc::text::joinStrings<std::array<std::string_view, 2>>({ nodeName, memberNameSanitized }, "::");
		if (!PinTypeOverrides.count(fullName))
			return {};

		return PinTypeOverrides.at(fullName);
	}


	Node* SoundGraphNodeFactory::SpawnGraphPropertyNode(const Ref<SoundGraphAsset>& graph, std::string_view propertyName, EPropertyType type, std::optional<bool> isLocalVarGetter)
	{
		HZ_CORE_ASSERT(graph);
		HZ_CORE_ASSERT(((type != EPropertyType::LocalVariable) ^ isLocalVarGetter.has_value()), "'isLocalVarGetter' property must be set if 'type' is 'LocalVariable'");

		const bool isLocalVariable = isLocalVarGetter.has_value() && (type == EPropertyType::LocalVariable);

		const Utils::PropertySet* properties = (type == EPropertyType::Input) ? &graph->GraphInputs
			: (type == EPropertyType::Output) ? &graph->GraphOutputs
			: (type == EPropertyType::LocalVariable) ? &graph->LocalVariables
			: nullptr;

		if (!properties)
			return nullptr;

		if (!properties->HasValue(propertyName))
		{
			HZ_CORE_ERROR("SpawnGraphPropertyNode() - property with the name \"{}\" doesn't exist!", propertyName);
			return nullptr;
		}

		choc::value::Value value = properties->GetValue(propertyName);

		const ImColor typeColour = GetValueColorStatic(value);
		const auto [pinType, storageKind] = GetPinTypeAndStorageKindForValueStatic(value);

		if (!isLocalVariable && value.isVoid())
		{
			HZ_CORE_ERROR("SpawnGraphPropertyNode() - invalid Graph Property value to spawn Graph Property node. Property name {}", propertyName);
			return nullptr;
		}

		const std::string propertyTypeString = Utils::SplitAtUpperCase(magic_enum::enum_name<EPropertyType>(type));

		const std::string nodeName(isLocalVariable ? propertyName : propertyTypeString);

		Node* newNode = new SGTypes::SGNode(nodeName, typeColour);

		newNode->Category = isLocalVariable ? propertyTypeString : nodeName;

		if (isLocalVarGetter.has_value())
			newNode->Type = NodeType::Simple;

		{
			Pin* newPin = SGTypes::CreatePinForType(pinType, propertyName);
			newPin->Storage = storageKind;
			newPin->Value = value;

			using TList = decltype(newNode->Inputs);
			TList& list = (type == EPropertyType::Input) ? newNode->Outputs
				: (type == EPropertyType::Output) ? newNode->Inputs
				: *isLocalVarGetter ? newNode->Outputs
				: newNode->Inputs;

			newPin->Kind = &list == &newNode->Inputs ? PinKind::Input : PinKind::Output;

			list.push_back(newPin);
		}

		return newNode;
	}

} // namespace Hazel::SoundGraph


//! To register a new node:
//! 1. Create new NodeProcessor type (e.g. in Audio/SoundGraph/Nodes/)
//! 2. Describe it in Audio/SoundGraph/Nodes/NodeDescriptors.h.
//! 3. Add new entry here to the Registry under appropriate category, specify Colour and Type.
//! 4. Optionally add pin type overrides to PinTypeOverrides if you need to represent in/out of type, for example, 'float' as a PinType::Audio.
//! 5. Optionally add default pin value to DefaultPinValues registry, otherwise the value is initialized to T(1).
//! 6. Add the same new NodeProcessor type entry to the NodeProcessors registry in SoundGraphFactory.cpp.
//
// The static factory classes can still be used to describe utility nodes which don't have corresponding NodeProcessor,
// like graph input nodes, or a comment node etc. They are a special case when parsing editing graph into a Graph Prototype.

#define NODE_CATEGORY(category, nodeFactories, ...) { std::string { #category }, std::map<std::string, std::function<Node*()>>{ nodeFactories, __VA_ARGS__ }}
#define DECLARE_NODE(category, name) {choc::text::replace(#name, "_", " "), category::name}
#define DECLARE_ARGS(name, args) {#name, args}
#define DECLARE_NODE_N(TNodeType, Color, Type) SG::CreateRegistryEntry<TNodeType>(#TNodeType, Color, Type)
#define DECLARE_NODE_ALIAS(AliasName, TNodeType, Color, Type) SG::CreateRegistryEntry<TNodeType>(AliasName, Color, Type, true)

namespace Colors = Hazel::SoundGraph::Colors;

template<>
const Hazel::Nodes::Registry Hazel::Nodes::Factory<SG::SoundGraphNodeFactory>::Registry =
{
	NODE_CATEGORY(Base,
		DECLARE_NODE(SG::Base, Input_Action),
		DECLARE_NODE(SG::Base, Output_Audio),
		DECLARE_NODE(SG::Base, On_Finished),
		DECLARE_NODE_N(SG::WavePlayer,                                    Colors::WavePlayer, NodeType::Blueprint),
	),
	NODE_CATEGORY(Math,
		DECLARE_NODE_ALIAS(Alias::AddAudio,       SG::Add<float>,         Colors::Audio,      NodeType::Simple),
		DECLARE_NODE_ALIAS(Alias::AddFloatAudio,  SG::Add<float>,         Colors::Audio,      NodeType::Simple),
		DECLARE_NODE_N(SG::Add<float>,                                    Colors::Float,      NodeType::Simple),
		DECLARE_NODE_N(SG::Add<int>,                                      Colors::Int,        NodeType::Simple),
		DECLARE_NODE_N(SG::Divide<float>,                                 Colors::Float,      NodeType::Simple),
		DECLARE_NODE_N(SG::Divide<int>,                                   Colors::Int,        NodeType::Simple),
		DECLARE_NODE_N(SG::Log,                                           Colors::Float,      NodeType::Simple),
		DECLARE_NODE_N(SG::LinearToLogFrequency,                          Colors::Float,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::FrequencyLogToLinear,                          Colors::Float,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Modulo,                                        Colors::Int,        NodeType::Simple),
		DECLARE_NODE_ALIAS(Alias::MultAudioFloat, SG::Multiply<float>,    Colors::Audio,      NodeType::Simple),
		DECLARE_NODE_ALIAS(Alias::MultAudio,      SG::Multiply<float>,    Colors::Audio,      NodeType::Simple),
		DECLARE_NODE_N(SG::Multiply<float>,                               Colors::Float,      NodeType::Simple),
		DECLARE_NODE_N(SG::Multiply<int>,                                 Colors::Int,        NodeType::Simple),
		DECLARE_NODE_N(SG::Power,                                         Colors::Float,      NodeType::Simple),
		DECLARE_NODE_ALIAS(Alias::SubtractAudio,  SG::Subtract<float>,    Colors::Audio,      NodeType::Simple),
		DECLARE_NODE_N(SG::Subtract<float>,                               Colors::Float,      NodeType::Simple),
		DECLARE_NODE_N(SG::Subtract<int>,                                 Colors::Int,        NodeType::Simple),

		DECLARE_NODE_N(SG::MapRange<float>,                               Colors::Float,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::MapRange<int>,                                 Colors::Int,        NodeType::Blueprint),
		DECLARE_NODE_ALIAS(Alias::MapRangeAudio,  SG::MapRange<float>,    Colors::Audio,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Min<float>,                                    Colors::Float,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Min<int>,                                      Colors::Int,        NodeType::Blueprint),
		DECLARE_NODE_ALIAS(Alias::MinAudio,       SG::Min<float>,         Colors::Audio,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Max<float>,                                    Colors::Float,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Max<int>,                                      Colors::Int,        NodeType::Blueprint),
		DECLARE_NODE_ALIAS(Alias::MaxAudio,       SG::Max<float>,         Colors::Audio,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Clamp<float>,                                  Colors::Float,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Clamp<int>,                                    Colors::Int,        NodeType::Blueprint),
		DECLARE_NODE_ALIAS(Alias::ClampAudio,     SG::Clamp<float>,       Colors::Audio,      NodeType::Blueprint),
	),
	NODE_CATEGORY(Array,
		DECLARE_NODE_N(SG::Get<float>,                                    Colors::Float,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Get<int>,                                      Colors::Int,        NodeType::Blueprint),
		DECLARE_NODE_ALIAS(Alias::GetWave,        SG::Get<int64_t>,       Colors::AudioAsset, NodeType::Blueprint),
		DECLARE_NODE_N(SG::GetRandom<float>,                              Colors::Float,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::GetRandom<int>,                                Colors::Int,        NodeType::Blueprint),
		DECLARE_NODE_ALIAS(Alias::GetRandomWave,  SG::GetRandom<int64_t>, Colors::AudioAsset, NodeType::Blueprint),
	),
	NODE_CATEGORY(Generators,
		DECLARE_NODE_N(SG::Noise,                                         Colors::Noise,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::Sine,                                          Colors::Noise,      NodeType::Blueprint),
	),
	NODE_CATEGORY(Trigger,
		DECLARE_NODE_N(SG::RepeatTrigger,                                 Colors::Trigger,    NodeType::Blueprint),
		DECLARE_NODE_N(SG::TriggerCounter,                                Colors::Trigger,    NodeType::Blueprint),
		DECLARE_NODE_N(SG::DelayedTrigger,                                Colors::Trigger,    NodeType::Blueprint),
	),
	NODE_CATEGORY(Envelope,
		DECLARE_NODE_ALIAS(Alias::ADEnvelope,     SG::ADEnvelope,         Colors::Envelope,   NodeType::Blueprint),
	),
	NODE_CATEGORY(Music,
		DECLARE_NODE_ALIAS(Alias::BPMToSeconds,   SG::BPMToSeconds,       Colors::Music,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::NoteToFrequency<float>,                        Colors::Music,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::NoteToFrequency<int>,                          Colors::Music,      NodeType::Blueprint),
		DECLARE_NODE_N(SG::FrequencyToNote,                               Colors::Music,      NodeType::Blueprint),
	),
	NODE_CATEGORY(Utility,
		DECLARE_NODE(SG::Utility, Comment),
	),
};

#undef NODE_CATEGORY
#undef DECLARE_NODE
#undef DECLARE_ARGS
#undef DECLARE_NODE_N
#undef DECLARE_NODE_ALIAS


#define DECLARE_PIN_ENUM(MemberPointer, Tokens) { std::string(Hazel::Nodes::SanitizeMemberName(#MemberPointer, true)),  Tokens }

template<>
const Hazel::Nodes::EnumTokensRegistry Hazel::Nodes::Factory<SG::SoundGraphNodeFactory>::EnumTokensRegistry =
{
	DECLARE_PIN_ENUM(SG::Noise::in_Type, &SG::EnumTokens::NoiseType)
};

#undef DECLARE_PIN_ENUM
