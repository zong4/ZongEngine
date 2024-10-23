#pragma once

#include "SoundGraphNodes.h"
#include "SoundGraphAsset.h"

#include "Engine/Audio/SoundGraph/SoundGraphSource.h"
#include "Engine/Audio/SoundGraph/Utils/SoundGraphCache.h"
#include "Engine/Editor/NodeGraphEditor/NodeEditorModel.h"
#include "Engine/Editor/NodeGraphEditor/NodeGraphUtils.h"
#include "Engine/Editor/NodeGraphEditor/PropertySet.h"

namespace Hazel {

	//==================================================================================
	/// SoundGraph Graph Editor Model
	class SoundGraphNodeEditorModel : public IONodeEditorModel
	{
	public:
		explicit SoundGraphNodeEditorModel(Ref<SoundGraphAsset> graphAsset);
		~SoundGraphNodeEditorModel();

		std::vector<Node*>& GetNodes() override { return m_GraphAsset->Nodes; }
		std::vector<Link>& GetLinks() override { return m_GraphAsset->Links; }
		const std::vector<Node*>& GetNodes() const override { return m_GraphAsset->Nodes; }
		const std::vector<Link>& GetLinks() const override { return m_GraphAsset->Links; }

		const std::vector<std::string>& GetInputTypes() const override { static std::vector<std::string> types = { "Float", "Int", "Bool", "AudioAsset", "Trigger" }; return types; }

		Utils::PropertySet& GetInputs() override { return m_GraphAsset->GraphInputs; }
		Utils::PropertySet& GetOutputs() override { return m_GraphAsset->GraphOutputs; }
		Utils::PropertySet& GetLocalVariables() override { return m_GraphAsset->LocalVariables; }

		const Utils::PropertySet& GetInputs() const override { return m_GraphAsset->GraphInputs; }
		const Utils::PropertySet& GetOutputs() const override { return m_GraphAsset->GraphOutputs; }
		const Utils::PropertySet& GetLocalVariables() const override { return m_GraphAsset->LocalVariables; }

		const Nodes::Registry& GetNodeTypes() const override { return m_NodeFactory.Registry; }

		void OnGraphPropertyValueChanged(EPropertyType type, const std::string& inputName) override;

		Node* SpawnGraphInputNode(const std::string& inputName) override;
		Node* SpawnLocalVariableNode(const std::string& inputName, bool getter) override;

		uint64_t GetCurrentPlaybackFrame() const;
		// Should be called to update sound source
		void OnUpdate(Timestep ts);

	private:
		// Sort nodes in order that makes sense
		bool Sort() override;
		void Serialize() override { Sort(); AssetImporter::Serialize(m_GraphAsset); };
		void Deserialize() override;

		bool SaveGraphState(const char* data, size_t size) override;
		const std::string& LoadGraphState() override;

		const Nodes::AbstractFactory* GetNodeFactory() const override { return &m_NodeFactory; }

		void AssignSomeDefaultValue(Pin* pin) final;

		void OnCompileGraph() override;

	private:
		SoundGraph::SoundGraphNodeFactory m_NodeFactory;
		Ref<SoundGraphAsset> m_GraphAsset = nullptr;
		Scope<SoundGraphSource> m_SoundGraphSource = nullptr;
		Scope<SoundGraphCache> m_Cache = nullptr;
	};

} // namespace Hazel
