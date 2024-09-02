#include "hzpch.h"
#include "AnimationGraphNodeEditorModel.h"

#include "AnimationGraphAsset.h"

#include "Hazel/Animation/AnimationGraphFactory.h"
#include "Hazel/Animation/NodeDescriptor.h"
#include "Hazel/Asset/AnimationAssetSerializer.h"
#include "Hazel/Asset/AssetManager.h"
#include "Hazel/Core/Application.h"
#include "Hazel/Core/Events/EditorEvents.h"
#include "Hazel/Editor/NodeGraphEditor/NodeGraphUtils.h"
#include "Hazel/Project/Project.h"

#include <CDT/CDT.h>
#include <imgui-node-editor/imgui_node_editor.h>

#include <format>

namespace AG = Hazel::AnimationGraph;

namespace Hazel {

	namespace AnimationGraph {

		static std::unordered_map<Identifier, std::string> s_Names;

		void RegisterIdentifier(Identifier id, std::string_view name)
		{
			s_Names[id] = name;
		}

		const std::string& GetName(Identifier id)
		{
			static std::string empty;

			if (auto name = s_Names.find(id); name != s_Names.end())
			{
				return name->second;
			}

			return empty;
		}


		struct GeneratorOptions
		{
			std::string_view Name;
			AnimationGraphNodeEditorModel* Model = nullptr;
			Node* RootNode = nullptr;
			// TODO: Cache?...  Cache = nullptr;
		};

		// helper remembers animation graph "path" and resets it on destruct
		struct PathHelper
		{
			PathHelper(UUID& rootNode, std::vector<Node*>& currentPath) : m_CurrentPath(currentPath), m_SavedPath(currentPath), m_RootNodeId(rootNode), m_SavedRootNode(rootNode) {}
			~PathHelper() { m_CurrentPath = m_SavedPath; m_RootNodeId = m_SavedRootNode; }
			std::vector<Node*>& m_CurrentPath;
			std::vector<Node*> m_SavedPath;
			UUID& m_RootNodeId;
			UUID m_SavedRootNode;
		};


		std::pair<std::vector<std::string>, std::vector<std::string>> PreValidateGraph(AnimationGraphNodeEditorModel& model)
		{
			std::vector<std::string> errors;
			std::vector<std::string> warnings;

			const auto validateNode = [&model, &errors](const Node* node)
			{
				if (node->Type == NodeType::Comment)
					return;

				if (node->Name != "Output" && node->Name != "Input Action" && node->Name != "Event" && !model.IsPropertyNode(node) && !Factory::Contains(Identifier(node->Name)))
				{
					errors.push_back("Animation Graph Factory doesn't contain node with type ID '" + node->Name + "'");
				}

				for (const auto& in : node->Inputs)
				{
					if (in->Value.isVoid() && !model.IsPinLinked(in->ID) && !in->IsType(Types::EPinType::Flow))
						errors.push_back("Invalid linkage: node '" + node->Name + "', pin '" + in->Name + "'");
				}

				if(auto error = model.GetNodeError(node); !error.empty())
					errors.push_back(std::format("Node '{}': {}", node->Name, error));
			};

			for (auto& [id, nodes] : model.GetAllNodes())
			{
				Node* rootNode = id == 0 ? nullptr : model.FindInAllNodes(id);
				HZ_CORE_ASSERT(id == 0 || rootNode);
				model.SetCurrentPath(rootNode);

				bool hasOutputNode = false;
				for (const auto& node : nodes)
				{
					validateNode(node);

					if (node->Name == "Output")
					{
						if (hasOutputNode)
						{
							if (rootNode)
							{
								errors.push_back("Animation subgraph for '" + rootNode->Description + "' must have only one 'Output' node");
							}
							else
							{
								errors.push_back("Animation graph must have only one 'Output' node");
							}
						}
						hasOutputNode = true;
					}
				}

				if (!rootNode || (rootNode->GetTypeID() != AG::Types::StateMachine && rootNode->GetTypeID() != AG::Types::BlendSpace))
				{
					// root node is not a state machine or blend space => there must be an "output" node in the graph
					if (!hasOutputNode)
					{
						if (rootNode)
						{
							errors.push_back("Animation subgraph for '" + rootNode->Description + "' must have an 'Output' node");
						}
						else
						{
							errors.push_back("Animation graph must have an 'Output' node");
						}
					}
				}
				else
				{
					// root node is a state machine => there must be at least one state
					if (nodes.empty())
					{
						if (rootNode->GetTypeID() == AG::Types::StateMachine)
						{
							errors.push_back("Animation state machine '" + rootNode->Description + "' must have at least one state");
						}
						else if(rootNode->GetTypeID() == AG::Types::BlendSpace)
						{
							errors.push_back("Animation blend space '" + rootNode->Description + "' must have at least one vertex");
						}
					}
					if (rootNode->GetTypeID() == AG::Types::BlendSpace)
					{
						// check for problems with blend space:
						// 1) we cannot have duplicate vertices
						// 2) triangles with very small area are probably not what user intended
						// 3) triangles with very small width or height are probably not what user intended

						std::vector<CDT::V2d<float>> vertices;
						vertices.reserve(nodes.size());
						for (auto node : nodes)
						{
							auto blendSpaceVertex = static_cast<AG::Types::Node*>(node);
							vertices.emplace_back(blendSpaceVertex->Offset.x, blendSpaceVertex->Offset.y);
						}
						auto info = CDT::RemoveDuplicates(vertices);
						if (!info.duplicates.empty())
						{
							errors.push_back("Animation Blend Space '" + rootNode->Description + "' has duplicate vertices!");
						}

						CDT::Triangulation<float> triangulation;
						triangulation.insertVertices(vertices);
						triangulation.eraseSuperTriangle();
						for (auto triangle : triangulation.triangles)
						{
							auto v0 = triangulation.vertices[triangle.vertices[0]];
							auto v1 = triangulation.vertices[triangle.vertices[1]];
							auto v2 = triangulation.vertices[triangle.vertices[2]];
							auto area = fabs(0.5f * ((v1.x - v0.x) * (v2.y - v0.y) - (v2.x - v0.x) * (v1.y - v0.y)));
							if (area < 0.001f)
							{
								warnings.push_back("Animation Blend Space '" + rootNode->Description + "' has degenerate triangles!");
								break;
							}

							auto edge0Squared = (v1.x - v0.x) * (v1.x - v0.x) + (v1.y - v0.y) * (v1.y - v0.y);
							auto edge1Squared = (v2.x - v1.x) * (v2.x - v1.x) + (v2.y - v1.y) * (v2.y - v1.y);
							auto edge2Squared = (v0.x - v2.x) * (v0.x - v2.x) + (v0.y - v2.y) * (v0.y - v2.y);
							auto widthSquared = std::max({ edge0Squared, edge1Squared, edge2Squared });
							auto width = glm::sqrt(widthSquared);
							auto height = 2.0f * area / width;

							if ((width < 0.01f) || (height < 0.01f))
							{
								warnings.push_back("Animation Blend Space '" + rootNode->Description + "' has degenerate triangles!");
								break;
							}
						}
					}
				}
			}

			for (auto& [id, links] : model.GetAllLinks())
			{
				Node* rootNode = id == 0 ? nullptr : model.FindInAllNodes(id);
				HZ_CORE_ASSERT(id == 0 || rootNode);
				model.SetCurrentPath(rootNode);

				for (const auto& link : links)
				{
					const Pin* sourcePin = model.FindPin(link.StartPinID);
					const Pin* destPin = model.FindPin(link.EndPinID);

					if(!sourcePin || !destPin || sourcePin->GetType() != destPin->GetType())
					{
						errors.push_back("Invalid linkage");
					}
				}
			}

			return { errors, warnings };
		}


		class Parser
		{
		public:

			Parser(GeneratorOptions& options, Prototype& outPrototype) : m_Options{ options }, m_Prototype(outPrototype)
			{
				HZ_CORE_ASSERT(m_Options.Model);
			}


			bool Parse()
			{
				try
				{
					ConstructIO();
					ParseInputParameters();
					ParseNodes();
					ParseConnections();

					// TODO: collect errors, potentially stop parsing at the first error encountered
					return true;
				}
				catch (const std::exception& e)
				{
					HZ_CONSOLE_LOG_ERROR(std::format("Failed to construct Animation Graph Prototype!\n{}", e.what()));
					return false;
				}
			}

		private:

			// We want to pass Asset handles as primitive integer instead of choc Value class
			// because the later would impose heavy member lookup to get the value at runtime.
			choc::value::Value TranslateAssetHandle(const choc::value::ValueView& value, const bool isArray)
			{
				if (isArray) return choc::value::createArray(value.size(), [copy = value](uint32_t i) { return (int64_t)Utils::GetAssetHandleFromValue(copy[i]); });
				else         return choc::value::createInt64((int64_t)Utils::GetAssetHandleFromValue(value));
			}

			// We want to pass Bones as primitive integer (index) instead of string (name)
			// because the later would impose heavy lookup to get the bone index at runtime.
			choc::value::Value TranslateBoneName(const choc::value::ValueView& value, const bool isArray)
			{
				if (isArray) return choc::value::createArray(value.size(), [copy = value, this](uint32_t i) { return static_cast<int>(m_Options.Model->FindBoneIndex(copy[i]["Value"].getString())); });
				else         return choc::value::createInt32(static_cast<int>(m_Options.Model->FindBoneIndex(value["Value"].getString())));
			}

			// translate (in place) editor values into runtime values:
			//    asset handle --> plain int64
			//    bone name --> int (index)
			void TranslateValue(choc::value::Value& value)
			{
				const bool isArray = value.isArray();
				const choc::value::Type& type = isArray ? value.getType().getElementType() : value.getType();

				if (Utils::IsAssetHandle<AssetType::Animation>(type))
				{
					value = TranslateAssetHandle(value, isArray);
				}
				else if (type.isObjectWithClassName(type::type_name<AG::Types::TBone>()))
				{
					value = TranslateBoneName(value, isArray);
				}
			}

			bool IsOrphan(const Node* node) const
			{
				if (m_Options.RootNode && ((m_Options.RootNode->GetTypeID() == AG::Types::StateMachine) || (m_Options.RootNode->GetTypeID() == AG::Types::BlendSpace)))
					return false;

				if (node->SortIndex == Node::UndefinedSortIndex)
					return true;

				for (const auto& in : node->Inputs)
				{
					if (m_Options.Model->IsPinLinked(in->ID))
						return false;
				}

				for (const auto& out : node->Outputs)
				{
					if (m_Options.Model->IsPinLinked(out->ID))
						return false;
				}

				return true;
			}


			void ConstructIO()
			{
				auto previousRoot = m_Options.Model->SetCurrentPath(m_Options.RootNode);
				if (!m_Options.RootNode || m_Options.Model->GetAllNodes().contains(m_Options.RootNode->ID))
				{
					for (const auto node : m_Options.Model->GetNodes())
					{
						if (node->Name == "Output")
						{
							for (auto input : node->Inputs)
							{
								m_Prototype.Outputs.emplace_back(Identifier(Utils::MakeSafeIdentifier(choc::text::replace(input->Name, " ", ""))), input->Value);
							}
						}
					}
				}
				m_Options.Model->SetCurrentPath(previousRoot);
			}


			void ParseInputParameters()
			{
				// Only create inputs for top level graph.
				// Inputs of subgraphs are routed from the top level graph
				if(m_Options.RootNode == nullptr)
				{
					const Utils::PropertySet& inputs = m_Options.Model->GetInputs();
					for (const auto& inputName : inputs.GetNames())
					{
						choc::value::Value value = inputs.GetValue(inputName);

						TranslateValue(value);

						Identifier id(Utils::MakeSafeIdentifier(choc::text::replace(inputName, " ", "")));
						m_Prototype.Inputs.emplace_back(id, value);

						// Store reverse lookup of id to name.  This is used in editor to display names of inputs.
						// It is not needed in runtime.
						RegisterIdentifier(id, inputName);
					}
				}
			}


			void ParseNodes()
			{
				auto previousRoot = m_Options.Model->SetCurrentPath(m_Options.RootNode);
				if (!m_Options.RootNode || m_Options.Model->GetAllNodes().contains(m_Options.RootNode->ID))
				{
					const auto& nodes = m_Options.Model->GetNodes();
					const Utils::PropertySet& graphInputs = m_Options.Model->GetInputs();

					for (const auto node : nodes)
					{
						if (node->Type == NodeType::Comment)
							continue;

						if (node->Name == "Input Action" || node->Name == "Input" || node->Name == "Output" || node->Name == "Event")
							continue;

						// Local variable setter node is allowed to be an orphan
						const bool isLocalVariableNode = m_Options.Model->IsLocalVariableNode(node);

						// We don't want to parse nodes that don't have any connections (unless they send value to remote (in the future))
						if (IsOrphan(node) && !isLocalVariableNode)
							continue;

						if (isLocalVariableNode)
						{
							const bool isSetter = node->Inputs.size() == 1;
							if (isSetter)
							{
								const auto& input = node->Inputs[0];

								// if Local Variable doesn't have connection to other source,
								// we need to create a "default value plug" for it
								if (!input->Value.isVoid() && !input->Value.isString() && input->IsType(Types::EPinType::Flow))
								{
									choc::value::Value value = input->Value;
									TranslateValue(value);
									const std::string localVariableName = Utils::MakeSafeIdentifier(choc::text::replace(input->Name, " ", ""));
									m_Prototype.LocalVariablePlugs.emplace_back(Identifier(localVariableName), value);
								}
							}
							else
							{
								// Local variable getter.
								// If there is no setter, then we also create a "default value plug" using the value from local variable property set
								if (std::find_if(std::begin(nodes), std::end(nodes), [&](const Node* otherNode)
								{
									return m_Options.Model->IsLocalVariableNode(otherNode) && otherNode->Inputs.size() == 1 && otherNode->Inputs[0]->Name == node->Name;
								}) == std::end(nodes))
								{
									choc::value::Value value = m_Options.Model->GetLocalVariables().GetValue(node->Name);
									TranslateValue(value);
									const std::string localVariableName = Utils::MakeSafeIdentifier(choc::text::replace(node->Name, " ", ""));
									m_Prototype.LocalVariablePlugs.emplace_back(Identifier(localVariableName), value);
								}
							}
							continue;
						}

						auto& pnode = m_Prototype.Nodes.emplace_back(Identifier(node->Name), node->ID);

						auto requiresPlug = [](const Pin* pin)
						{
							return !pin->Value.isVoid() && !pin->Value.isString() && !pin->IsType(Types::Flow);
						};

						const uint32_t defaultPlugsToAdd = (uint32_t)std::count_if(node->Inputs.begin(), node->Inputs.end(), [&requiresPlug](const Pin* in) { return requiresPlug(in); });
						pnode.DefaultValuePlugs.reserve(defaultPlugsToAdd);
						for (const auto& input : node->Inputs)
						{
							if (!requiresPlug(input))
								continue;

							choc::value::Value value = input->Value;
							TranslateValue(value);
							const std::string inputValueName = choc::text::replace(input->Name, " ", "");
							pnode.DefaultValuePlugs.emplace_back(Identifier(inputValueName), value);
						}
						if (node->Name == "Blend Space Vertex")
						{
							// HACK: pack blend space vertex position into two additional inputs
							auto agNode = static_cast<const AG::Types::Node*>(node);
							pnode.DefaultValuePlugs.emplace_back(AG::IDs::X, choc::value::createFloat32(agNode->Offset.x));
							pnode.DefaultValuePlugs.emplace_back(AG::IDs::Y, choc::value::createFloat32(agNode->Offset.y));
						}

						if (node->GetTypeID() == AG::Types::BlendSpace)
						{
							// HACK: pack lerp times into two additional inputs
							auto agNode = static_cast<const AG::Types::Node*>(node);
							pnode.DefaultValuePlugs.emplace_back(AG::IDs::LerpSecondsPerUnitX, choc::value::createFloat32(agNode->LerpSecondsPerUnit.x));
							pnode.DefaultValuePlugs.emplace_back(AG::IDs::LerpSecondsPerUnitY, choc::value::createFloat32(agNode->LerpSecondsPerUnit.y));
						}

						if (node->Type == NodeType::Subroutine)
						{
							auto subroutine = CreateScope<Prototype>(m_Options.Model->GetSkeletonHandle());

							GeneratorOptions options = m_Options;
							options.RootNode = node;

							Parser parser{ options, *subroutine };
							if (parser.Parse())
							{
								pnode.Subroutine = std::move(subroutine);
							}
							else
							{
								HZ_CORE_ASSERT(false, "Failed to parse subroutine! (this should not happen - check PreValidateGraph)");
							}
						}
					}
				}
				m_Options.Model->SetCurrentPath(previousRoot);
			}


			void ParseConnections()
			{
				auto previousRoot = m_Options.Model->SetCurrentPath(m_Options.RootNode);
				if (!m_Options.RootNode || m_Options.Model->GetAllNodes().contains(m_Options.RootNode->ID))
				{
					const std::vector<Link>& links = m_Options.Model->GetLinks();

					// TODO: connection order matters, need to start from the graph Inputs

					for (const auto& link : links)
					{
						const Pin* sourcePin = m_Options.Model->FindPin(link.StartPinID);
						const Pin* destPin = m_Options.Model->FindPin(link.EndPinID);

						if (!sourcePin || !destPin)
							continue;

						const Node* sourceNode = m_Options.Model->FindNode(sourcePin->NodeID);
						const Node* destNode = m_Options.Model->FindNode(destPin->NodeID);

						if (!sourceNode || !destNode)
							continue;

						if (sourcePin->GetType() != destPin->GetType())
						{
							HZ_CORE_ASSERT(false, "sourcePin type is different from destPin.  This should have been picked up in PreValidateGraph()");
							continue;
						}

						if (sourceNode->SortIndex == Node::UndefinedSortIndex || destNode->SortIndex == Node::UndefinedSortIndex)
							continue;

						// We parse destination local variables by rerouting source local variables
						if (m_Options.Model->IsLocalVariableNode(destNode))
							continue;

						std::string sourceNodeName = Utils::MakeSafeIdentifier(sourceNode->Name);
						std::string destNodeName = Utils::MakeSafeIdentifier(destNode->Name);
						std::string sourcePinName = Utils::MakeSafeIdentifier(choc::text::replace(sourcePin->Name, " ", ""));
						std::string destPinName = Utils::MakeSafeIdentifier(choc::text::replace(destPin->Name, " ", ""));

						if (m_Options.Model->IsLocalVariableNode(sourceNode))
						{
							/* Reroute from source node Local Variable to the source of the Local Variable
								Source -> LV(setter) | LV(getter) -> destination
								..becomes
								Source -> destination
							*/
							if (auto* localVarSourcePin = m_Options.Model->FindLocalVariableSource(sourceNode->Name))
							{
								sourcePin = localVarSourcePin;
								sourceNode = m_Options.Model->FindNode(sourcePin->NodeID);
								if (!sourceNode)
								{
									HZ_CORE_ERROR("Failed to find source node while parsing Local Variable.");
									continue;
								}
								sourceNodeName = Utils::MakeSafeIdentifier(sourceNode->Name);
								sourcePinName = Utils::MakeSafeIdentifier(choc::text::replace(sourcePin->Name, " ", ""));
							}
							else // Local variable plug connection
							{
								if (destNodeName == "Output")
								{
									HZ_CORE_ASSERT(!sourcePin->IsType(EPinType::Flow), "Output pin type is flow.  This does not make sense (graph outputs need to be proper values, not 'triggers').");
									m_Prototype.Connections.emplace_back(
										Prototype::Connection::Endpoint{ m_Prototype.ID, Identifier{ sourcePinName } },
										Prototype::Connection::Endpoint{ 0, Identifier{ destPinName } },
										Prototype::Connection::LocalVariable_GraphValue
									);
								}
								else
								{
									// Note: Local variable plug means there wasn't any "setter" node for the local variable.
									//       If the local variable is "Flow" (aka "Trigger") type, and there isn't a setter, then it means the
									//       trigger will never occur.  There is no need to create any connection in this case.
									if (!sourcePin->IsType(Types::EPinType::Flow))
									{
										m_Prototype.Connections.emplace_back(
											Prototype::Connection::Endpoint{ sourceNode->ID, Identifier{sourcePinName} },
											Prototype::Connection::Endpoint{ destNode->ID, Identifier{destPinName} },
											Prototype::Connection::LocalVariable_NodeValue
										);
									}
								}

								continue;
							}
						}

						// First handle special cases of graph inputs
						// then other node connections
						if (sourceNodeName == "Input_Action")
						{
							HZ_CORE_ASSERT(false, "Input_Action is no longer supported!");
#if 0
							Prototype::Node* destination = FindNodeByID(destNode->ID);
							HZ_CORE_ASSERT(destination);

							const Identifier inputActionID = sourcePinName == "Update" ? AnimationGraph::IDs::Update : Identifier();
							// Invalid Input Action ID
							{
								HZ_CORE_ASSERT(inputActionID != Identifier());
								// TODO: collect error and terminate compilation
							}

							// In Event -> In Event
							m_Prototype.Connections.emplace_back(
								Prototype::Connection::Endpoint{ m_Prototype.ID, inputActionID },
								Prototype::Connection::Endpoint{ destNode->ID, Identifier{destPinName} },
								Prototype::Connection::GraphEvent_NodeEvent
							);
#endif
						}
						else if (destNodeName == "Output")
						{
							HZ_CORE_ASSERT(!sourcePin->IsType(EPinType::Flow), "Output pin type is flow.  This does not make sense (graph outputs need to be proper values, not 'triggers').");
							if (sourceNodeName == "Input")
							{
								m_Prototype.Connections.emplace_back(
									Prototype::Connection::Endpoint{ m_Prototype.ID, Identifier{ sourcePinName } },
									Prototype::Connection::Endpoint{ 0, Identifier{ destPinName } },
									Prototype::Connection::GraphValue_GraphValue
								);
							}
							else
							{
								m_Prototype.Connections.emplace_back(
									Prototype::Connection::Endpoint{ sourceNode->ID, Identifier{ sourcePinName } },
									Prototype::Connection::Endpoint{ 0, Identifier{ destPinName } },
									Prototype::Connection::NodeValue_GraphValue
								);
							}
						}
						else if (destNodeName == "Event")
						{
							HZ_CORE_ASSERT(destNode->Inputs.size() >= 2);
							HZ_CORE_ASSERT(destNode->Inputs[1]->GetType() == Types::EPinType::String);
							std::string_view eventName = destNode->Inputs[1]->Value.getString();
							if (sourceNodeName == "Input")
							{
								m_Prototype.Connections.emplace_back(
									Prototype::Connection::Endpoint{ m_Prototype.ID, Identifier{ sourcePinName } },
									Prototype::Connection::Endpoint{ m_Prototype.ID, Identifier{ destPinName } },
									Prototype::Connection::GraphEvent_GraphEvent
								);
							}
							else
							{
								m_Prototype.Connections.emplace_back(
									Prototype::Connection::Endpoint{ sourceNode->ID, Identifier{ sourcePinName } },
									Prototype::Connection::Endpoint{ m_Prototype.ID, Identifier{ eventName } },
									Prototype::Connection::NodeEvent_GraphEvent
								);
							}
						}
						else if (sourceNodeName == "Input")
						{
							Prototype::Node* destination = FindNodeByID(destNode->ID);
							HZ_CORE_ASSERT(destination);

							m_Prototype.Connections.emplace_back(
								Prototype::Connection::Endpoint{ m_Prototype.ID, Identifier{ sourcePinName } },
								Prototype::Connection::Endpoint{ destNode->ID, Identifier{ destPinName } },
								sourcePin->IsType(Types::EPinType::Flow) ? Prototype::Connection::GraphEvent_NodeEvent : Prototype::Connection::GraphValue_NodeValue
							);
						}
						else
						{
							HZ_CORE_ASSERT(FindNodeByID(sourceNode->ID));
							HZ_CORE_ASSERT(FindNodeByID(sourceNode->ID));
							HZ_CORE_ASSERT(FindNodeByID(destNode->ID));

							m_Prototype.Connections.emplace_back(
								Prototype::Connection::Endpoint{ sourceNode->ID, Identifier{ sourcePinName } },
								Prototype::Connection::Endpoint{ destNode->ID, Identifier{ destPinName } },
								sourcePin->IsType(Types::EPinType::Flow) ? Prototype::Connection::NodeEvent_NodeEvent : Prototype::Connection::NodeValue_NodeValue
							);
						}
					}
				}
				m_Options.Model->SetCurrentPath(previousRoot);
			}


			Prototype::Node* FindNodeByID(UUID id)
			{
				auto it = std::find_if(m_Prototype.Nodes.begin(), m_Prototype.Nodes.end(), [id](const Prototype::Node& nodePtr)
				{
					return nodePtr.ID == id;
				});

				if (it != m_Prototype.Nodes.end())
					return &(*it);
				else
					return nullptr;
			}

		private:
			GeneratorOptions& m_Options;
			Prototype& m_Prototype;
		};


		Scope<Prototype> ConstructPrototype(GeneratorOptions options)
		{
			if (!options.Model)
				return nullptr;

			auto [errors, warnings] = PreValidateGraph(*options.Model);

			if (!warnings.empty())
			{
				for (const auto& warning : warnings)
				{
					HZ_CONSOLE_LOG_WARN(warning);
				}
			}

			if (!errors.empty())
			{
				errors.insert(errors.begin(), "Graph is invalid.");
				HZ_CONSOLE_LOG_ERROR("Failed to construct Animation Graph Prototype! " + choc::text::joinStrings(errors, "\n"));
				return nullptr;
			}

			Scope<Prototype> prototype = CreateScope<Prototype>(options.Model->GetSkeletonHandle());
			prototype->DebugName = options.Name;

			Parser parser{ options, *prototype };

			if (parser.Parse())
			{
				// At this stage the Graph is constructed but not initialized

				// TODO: cache
				//if (options.Cache)
				//{
				//	options.Cache->StorePrototype(options.Name.c_str(), *prototype);
				//	options.Graph->CachedPrototype = options.Cache->GetFileForKey(options.Name.c_str());
				//}

				return prototype;
			}

			return nullptr;
		}

	} // namespace AnimationGraph


	AnimationGraphNodeEditorModel::AnimationGraphNodeEditorModel(Ref<AnimationGraphAsset> animationGraph)
		: m_AnimationGraph(animationGraph)
	{
		Deserialize();

		onEditNode = [this](Node* node) {
			SetCurrentPath(node);
			EnsureSubGraph(node);
			InvalidateGraphState();
		};

		onNodeCreated = [this]() {
			// If current root node is a state machine, then make sure there is an entry state
			// (e.g. the newly added node might be the only one. In which case it should be the entry state)
			if (m_CurrentPath.size() > 0 && m_CurrentPath.back()->GetTypeID() == AG::Types::StateMachine)
			{
				EnsureEntryState();
			}
		};

		onNodeDeleted = [this]() {
			std::unordered_set<UUID> nodesToDelete;

			// Remove transition nodes that are left dangling
			for (auto& node : GetNodes())
			{
				if (node->GetTypeID() == AG::Types::ENodeType::Transition)
				{
					if (!GetNodeConnectedToPin(node->Inputs[0]->ID))
						nodesToDelete.insert(node->ID);

					if (!GetNodeConnectedToPin(node->Outputs[0]->ID))
						nodesToDelete.insert(node->ID);
				}
			}
			for (auto node : nodesToDelete)
			{
				RemoveNode(node);
			}
			nodesToDelete.clear();

			// If deleted node is a subgraph, must also remove all subgraphs of that subgraph
			for (auto& [id, nodes] : GetAllNodes())
			{
				if (id && !FindInAllNodes(id))
				{
					for (auto node : nodes)
					{
						// if node->ID is a subgraph, then we need to delete this subgraph also
						if(GetAllNodes().find(node->ID) != GetAllNodes().end())
							nodesToDelete.insert(node->ID);
					}
					nodesToDelete.insert(id);
				}
			}
			for (auto id : nodesToDelete)
			{
				// delete all nodes in subgraph
				for (auto node : GetAllNodes().find(id)->second)
				{
					delete node;
				}

				// remove the subgraph itself
				GetAllNodes().erase(id);
				GetAllLinks().erase(id);
			}

			// If current root node is a state machine, then make sure there is still an entry state
			// (e.g. the deleted node(s) may have included the entry state)
			if (m_CurrentPath.size() > 0 && m_CurrentPath.back()->GetTypeID() == AG::Types::StateMachine)
			{
				EnsureEntryState();
			}
		};

		onLinkDeleted = [this]() {
			if(GetCurrentPath().empty() || GetCurrentPath().back()->GetTypeID() == AG::Types::State) {
				Node* outputNode = FindNodeByName("Output");
				if (outputNode)
				{
					if (!IsPinLinked(outputNode->Inputs[0]->ID))
					{
						// If "pose" output is not linked, then set its value to 0.
						// 0 is a sentinel value that is interpreted during runtime as "use the skeleton bind pose"
						// Why do we not just set the bone transforms for the bind pose here, and then we don't have to do anything
						// during runtime initialization?
						// Because its possible that the user changes skeleton later, (or more likely makes changes to the 
						// raw asset that skeleton is loaded from) - in that case we want to pick up and use those modified values, 
						// not any values that we save here.
						outputNode->Inputs[0]->Value = choc::value::createInt64(0);
					}
				}
			}
		};

		onPinValueChanged = [&](UUID nodeID, UUID pinID)
		{
			if (auto* node = FindNode(nodeID))
			{
				if (node->GetTypeID() == AG::Types::ENodeType::QuickState)
				{
					auto value = Utils::GetAssetHandleFromValue(node->Inputs[0]->Value);
					if (AssetManager::IsAssetHandleValid(value))
					{
						auto assetFileName = Project::GetEditorAssetManager()->GetMetadata(value).FilePath.stem().string();
						node->Description = assetFileName;
					}
					else
					{
						node->Description = node->Name;
					}
				}
			}
			InvalidatePlayer();
		};

		onCompile = [this](Ref<Asset> asset)
		{
			AnimationGraph::PathHelper helper(m_RootNodeId, m_CurrentPath);

			// The most important step, it determines execution order for the whole graph
			if (!Sort())
				return false;

			auto animationGraphAsset = asset.As<AnimationGraphAsset>();

			// Get name from metadata, if available (note: if loading from assetpack, we don't have metadata here)
			std::string name;
			if (asset->Handle)
			{
				const AssetMetadata& md = Project::GetEditorAssetManager()->GetMetadata(asset->Handle);
				name = md.FilePath.stem().string();
			}

			// Don't overwrite asset unless construction of prototype is successful
			auto prototype = AG::ConstructPrototype({ name, this });
			if (prototype)
			{
				animationGraphAsset->Prototype = std::move(prototype);
				return true;
			}

			return false;
		};
	}


	AssetHandle AnimationGraphNodeEditorModel::GetSkeletonHandle() const
	{
		return m_AnimationGraph->m_SkeletonHandle;
	}


	void AnimationGraphNodeEditorModel::SetSkeletonHandle(AssetHandle handle)
	{
		m_AnimationGraph->m_SkeletonHandle = handle;
	}


	void AnimationGraphNodeEditorModel::EnsureSubGraph(Node* parent)
	{
		if (GetAllNodes().find(parent->ID) == GetAllNodes().end())
		{
			GetAllNodes()[parent->ID] = {};
			GetAllLinks()[parent->ID] = {};
			if (parent->GetTypeID() == AG::Types::Transition)
			{
				auto previousRootId = m_RootNodeId;
				m_RootNodeId = parent->ID;
				auto node = m_TransitionGraphNodeFactory.SpawnNode("Transition", "Output");
				OnNodeSpawned(node);
				m_AnimationGraph->m_GraphState[parent->ID] = std::format("{{\"nodes\":{{\"node:{0}\":{{\"location\":{{\"x\":967,\"y\":376}}}}}},\"selection\":null,\"view\":{{\"scroll\":{{\"x\":279.6109619140625,\"y\":333.6357421875}},\"visible_rect\":{{\"max\":{{\"x\":1624.48876953125,\"y\":697.30859375}},\"min\":{{\"x\":223.688766479492188,\"y\":266.908599853515625}}}},\"zoom\":1.25}}}}", node->ID);
				m_RootNodeId = previousRootId;
			}
			else if (parent->GetTypeID() == AG::Types::StateMachine)
			{
				m_AnimationGraph->m_GraphState[parent->ID] = "{\"nodes\":null,\"selection\":null,\"view\":{\"scroll\":{\"x\":434.9451904296875,\"y\":431.64227294921875},\"visible_rect\":{\"max\":{\"x\":1457.2969970703125,\"y\":763.76165771484375},\"min\":{\"x\":289.9635009765625,\"y\":287.76153564453125}},\"zoom\":1.4999997615814209}}";
			}
			else if (parent->GetTypeID() == AG::Types::BlendSpace)
			{
				m_AnimationGraph->m_GraphState[parent->ID] = "{\"nodes\":null,\"selection\":null,\"view\":{\"scroll\":{\"x\":0.0,\"y\":0.0},\"visible_rect\":{\"max\":{\"x\":400.0,\"y\":400.0},\"min\":{\"x\":-400.0,\"y\":-400.0}},\"zoom\":1.0}}";
			}
			else
			{
				auto node = m_AnimationGraphNodeFactory.SpawnNode("Animation", "Output");
				OnNodeSpawned(node);
				m_AnimationGraph->m_GraphState[parent->ID] = std::format("{{\"nodes\":{{\"node:{0}\":{{\"location\":{{\"x\":968,\"y\":446}}}}}},\"selection\":null,\"view\":{{\"scroll\":{{\"x\":465.8328857421875,\"y\":137.405792236328125}},\"visible_rect\":{{\"max\":{{\"x\":1202.6761474609375,\"y\":836.28948974609375}},\"min\":{{\"x\":271.59063720703125,\"y\":80.1105422973632812}}}},\"zoom\":1.7152024507522583}}}}", node->ID);
			}
		}
	}


	Node* AnimationGraphNodeEditorModel::CreateTransitionNode()
	{
		if (auto node = AG::StateMachineNodeFactory::CreateTransitionNode())
		{
			node->Category = "StateMachine";
			OnNodeSpawned(node);
			OnNodeCreated();
			return GetNodes().back();
		}
		return nullptr;
	}


	Node* AnimationGraphNodeEditorModel::FindInAllNodes(UUID id)
	{
		for (auto& [subgraphID, nodes] : GetAllNodes())
		{
			for (auto node : nodes)
			{
				if (node->ID == id)
				{
					return node;
				}
			}
		}
		return nullptr;
	}


	Node* AnimationGraphNodeEditorModel::FindParent(UUID id)
	{
		for (auto& [subgraphID, nodes] : GetAllNodes())
		{
			for (auto node : nodes)
			{
				if (node->ID == id)
				{
					return subgraphID == 0? nullptr : FindInAllNodes(subgraphID);
				}
			}
		}
		return nullptr;
	}


	const Nodes::Registry& AnimationGraphNodeEditorModel::GetNodeTypes() const
	{
		auto currentPath = GetCurrentPath();
		if (!currentPath.empty())
		{
			if (currentPath.back()->GetTypeID() == AG::Types::ENodeType::StateMachine)
			{
				return m_StateMachineNodeFactory.Registry;
			}
			else if (currentPath.back()->GetTypeID() == AG::Types::ENodeType::Transition)
			{
				return m_TransitionGraphNodeFactory.Registry;
			}
			else if (currentPath.back()->GetTypeID() == AG::Types::ENodeType::BlendSpace)
			{
				return m_BlendSpaceNodeFactory.Registry;
			}
		}
		return m_AnimationGraphNodeFactory.Registry;
	}


	bool AnimationGraphNodeEditorModel::SaveGraphState(const char* data, size_t size)
	{
		HZ_CORE_INFO_TAG("Animation", "SaveGraphState() node = {0}: {1}", m_RootNodeId, data);
		m_AnimationGraph->m_GraphState.at(m_RootNodeId) = data;
		return true;
	}


	const std::string& AnimationGraphNodeEditorModel::LoadGraphState()
	{
		HZ_CORE_INFO_TAG("Animation", "LoadGraphState() node = {0}: {1}", m_RootNodeId, m_AnimationGraph->m_GraphState.at(m_RootNodeId));
		return m_AnimationGraph->m_GraphState.at(m_RootNodeId);
	}


	Node* AnimationGraphNodeEditorModel::SetCurrentPath(Node* node)
	{
		Node* previous = nullptr;
		if (!m_CurrentPath.empty())
		{
			previous = m_CurrentPath.back();
		}

		if (!node)
		{
			m_RootNodeId = 0;
			m_CurrentPath.clear();
		}
		else
		{
			m_RootNodeId = node->ID;
			auto i = std::find(m_CurrentPath.begin(), m_CurrentPath.end(), node);
			if (i == m_CurrentPath.end())
			{
				m_CurrentPath.emplace_back(node);
			}
			else
			{
				m_CurrentPath.erase(i + 1, m_CurrentPath.end());
			}
		}

		return previous;
	}


	void AnimationGraphNodeEditorModel::RemovePropertyFromGraph(EPropertyType type, const std::string& name)
	{
		if (type == EPropertyType::Invalid)
		{
			HZ_CORE_ASSERT(false);
			return;
		}

		auto& properties = GetPropertySet(type);
		properties.Remove(name);

		UUID previousRootId = m_RootNodeId;
		for (const auto& [id, _] : GetAllNodes())
		{
			m_RootNodeId = id;
			std::vector<UUID> nodesToDelete;
			for (const auto& node : GetNodes())
			{
				if (IsPropertyNode(type, node, name))
				{
					nodesToDelete.push_back(node->ID);
				}
			}
			RemoveNodes(nodesToDelete);
		}
		m_RootNodeId = previousRootId;

		InvalidatePlayer();
	}


	void AnimationGraphNodeEditorModel::OnGraphPropertyNameChanged(EPropertyType type, const std::string& oldName, const std::string& newName)
	{
		if (type == EPropertyType::Invalid)
			return;

		const auto getPropertyValuePin = [](Node* node, std::string_view propertyName) -> Pin*
		{
			HZ_CORE_ASSERT(!node->Outputs.empty() || !node->Inputs.empty());

			if (!node->Outputs.empty())
				return node->Outputs[0];
			else if (!node->Inputs.empty())
				return node->Inputs[0];

			return nullptr;
		};

		bool renamed = false;

		UUID previousRootId = m_RootNodeId;
		for (const auto& [id, _] : GetAllNodes())
		{
			m_RootNodeId = id;
			for (auto& node : GetNodes())
			{
				if (IsPropertyNode(type, node, oldName))
				{
					if (Pin* pin = getPropertyValuePin(node, oldName))
					{
						pin->Name = newName;

						if (pin->Kind == PinKind::Output)
						{
							if (Link* link = GetLinkConnectedToPin(pin->ID))
							{
								if (Pin* endPin = FindPin(link->EndPinID))
									endPin->Value = choc::value::createString(newName);
							}
						}

						renamed = true;
					}

					if (type == EPropertyType::LocalVariable)
					{
						node->Name = newName;
						renamed = true;
					}
				}
			}
		}
		m_RootNodeId = previousRootId;

		if (renamed)
			InvalidatePlayer();
	}


	void AnimationGraphNodeEditorModel::OnGraphPropertyTypeChanged(EPropertyType type, const std::string& inputName)
	{
		if (type == EPropertyType::Invalid)
			return;

		auto newProperty = GetPropertySet(type).GetValue(inputName);

		const bool isArray = newProperty.isArray();
		auto [pinType, storageKind] = GetNodeFactory()->GetPinTypeAndStorageKindForValue(newProperty);

		const auto getPropertyValuePin = [](Node* node, std::string_view propertyName) -> Pin**
		{
			HZ_CORE_ASSERT(!node->Outputs.empty() || !node->Inputs.empty());

			if (!node->Outputs.empty())
				return &node->Outputs[0];
			else if (!node->Inputs.empty())
				return &node->Inputs[0];

			return nullptr;
		};

		bool pinChanged = false;

		UUID previousRootId = m_RootNodeId;
		for (const auto& [id, _] : GetAllNodes())
		{
			m_RootNodeId = id;
			for (auto& node : GetNodes())
			{
				if (IsPropertyNode(type, node, inputName))
				{
					if (Pin** pin = getPropertyValuePin(node, inputName))
					{
						// create new pin of the new type
						Pin* newPin = GetNodeFactory()->CreatePinForType(pinType);

						// copy data from the old pin to the new pin
						*newPin = **pin;

						// delete old pin
						delete (*pin);

						// emplace new pin instead of the old pin
						*pin = newPin;

						//pin->Type = pinType;
						newPin->Storage = storageKind;
						newPin->Value = newProperty; // TODO: should use one or the other
						node->Color = GetValueColor(newProperty);

						if (Link* connectedLink = GetLinkConnectedToPin(newPin->ID))
						{
							ax::NodeEditor::DeleteLink(ax::NodeEditor::LinkId(connectedLink->ID));
							RemoveLink(connectedLink->ID);
							// TODO: add visual indication for the user that the node pin has been disconnected, or that the link is invalid
						}

						pinChanged = true;
					}
				}
			}
		}
		m_RootNodeId = previousRootId;

		if (pinChanged)
			InvalidatePlayer();
	}


	void AnimationGraphNodeEditorModel::OnGraphPropertyValueChanged(EPropertyType type, const std::string& inputName)
	{
		if (type == EPropertyType::Invalid)
			return;

		auto newValue = GetPropertySet(type).GetValue(inputName);
		const auto& valueType = newValue.getType();

		// If wave asset input changed, need to initialize audio data readers for new files
		// which we do when recompiling.
		// TODO: perhaps we could somehow do a clean swap of the readers while the player is playing?
		if (Utils::IsAssetHandle<AssetType::Audio>(valueType) || (valueType.isArray() && Utils::IsAssetHandle<AssetType::Audio>(valueType.getElementType())))
		{
			InvalidatePlayer();
		}

		// TODO (0x): when we have animation graph preview, we will also need to InvalidatePlayer() for other asset types (such as animation clip)

		const auto getPropertyValuePin = [](Node* node, std::string_view propertyName) -> Pin*
		{
			HZ_CORE_ASSERT(!node->Outputs.empty() || !node->Inputs.empty());

			if (!node->Outputs.empty())
				return node->Outputs[0];
			else if (!node->Inputs.empty())
				return node->Inputs[0];

			return nullptr;
		};


		UUID previousRootId = m_RootNodeId;
		for (const auto& [id, _] : GetAllNodes())
		{
			m_RootNodeId = id;
			for (auto& node : GetNodes())
			{
				if (IsPropertyNode(type, node, inputName))
				{
					if (Pin* pin = getPropertyValuePin(node, inputName))
						pin->Value = newValue;
				}
			}
		}
		m_RootNodeId = previousRootId;

		if (type == EPropertyType::LocalVariable)
			InvalidatePlayer();
	}


	Node* AnimationGraphNodeEditorModel::SpawnGraphInputNode(const std::string& inputName)
	{
		Node* newNode = AG::AnimationGraphNodeFactory::SpawnGraphPropertyNode(m_AnimationGraph, inputName, EPropertyType::Input);

		if (!newNode)
			return nullptr;

		OnNodeSpawned(newNode);
		OnNodeCreated();
		return GetNodes().back();
	}


	Node* AnimationGraphNodeEditorModel::SpawnLocalVariableNode(const std::string& variableName, bool getter)
	{
		// We don't allow to create multiple setter nodes for graph Local Variables
		if (!getter)
		{
			for (const auto& node : GetNodes())
			{
				if (node->Category == GetPropertyToken(EPropertyType::LocalVariable)
					&& node->Name == variableName
					&& node->Inputs.size() == 1
					&& node->Inputs[0]->Name == variableName)
				{
					HZ_CONSOLE_LOG_TRACE("Setter node for the variable '{}' already exists!", variableName);
					ax::NodeEditor::SelectNode((uint64_t)node->ID, false);
					ax::NodeEditor::NavigateToSelection();
					return nullptr;
				}
			}
		}

		Node* newNode = AG::AnimationGraphNodeFactory::SpawnGraphPropertyNode(m_AnimationGraph, variableName, EPropertyType::LocalVariable, getter);

		if (!newNode)
			return nullptr;

		OnNodeSpawned(newNode);
		OnNodeCreated();

		return GetNodes().back();
	}


	void AnimationGraphNodeEditorModel::PromoteQuickStateToState(Node* quickState)
	{
		HZ_CORE_ASSERT(quickState);

		auto state = CreateNode("StateMachine", "State");
		HZ_CORE_ASSERT(state);

		// Could copy the quick-state description, but I think it's better we don't
		// (chances are the promoted state is going to be changed, and the quick state node description may no longer be appropriate)
		//stateNode->Description = quickState->Description;
		state->SetPosition(quickState->GetPosition());

		Ref<AnimationGraphAsset> animationGraph;
		auto extension = Project::GetEditorAssetManager()->GetDefaultExtensionForAssetType(AssetType::AnimationGraph);
		HZ_CORE_VERIFY(AnimationGraphAssetSerializer::TryLoadData("Resources/Animation/PromotedStateAnimationGraph" + extension, animationGraph));
		// Regenerate the IDs in the newly copied graph.
		// Reason for this is that we need IDs to be unique across all sub-graphs
		// (and we might promote more than one quick state)
		std::unordered_map<UUID, UUID> idMapping;
		for (auto& node : animationGraph->m_Nodes[0])
		{
			idMapping[node->ID] = UUID();
			node->ID = idMapping[node->ID];

			for (auto& pin : node->Inputs)
			{
				idMapping[pin->ID] = UUID();
				pin->ID = idMapping[pin->ID];
				pin->NodeID = node->ID;
				if ((pin->Name == "Animation") && (node->Name == "Animation Player"))
				{
					pin->Value = quickState->Inputs[0]->Value;
				}
			}
			for (auto& pin : node->Outputs)
			{
				idMapping[pin->ID] = UUID();
				pin->ID = idMapping[pin->ID];
				pin->NodeID = node->ID;
			}
		}

		for (auto& link : animationGraph->m_Links[0])
		{
			link.ID = UUID();
			link.StartPinID = idMapping[link.StartPinID];
			link.EndPinID = idMapping[link.EndPinID];
		}

		// remap all transitions out of the quick state
		for (auto output : quickState->Outputs)
		{
			Pin* pin = state->Outputs.emplace_back(AnimationGraph::Types::CreatePinForType(AG::Types::EPinType::Flow, "Transition"));
			pin->ID = output->ID;
			pin->NodeID = state->ID;
			pin->Kind = PinKind::Output;
		}

		// remap all transitions into the quick state
		for (auto input : quickState->Inputs)
		{
			if (auto link = GetLinkConnectedToPin(input->ID); link)
			{
				Pin* pin = state->Inputs.emplace_back(AnimationGraph::Types::CreatePinForType(AG::Types::EPinType::Flow, "Transition"));
				pin->ID = input->ID;
				pin->NodeID = state->ID;
				pin->Kind = PinKind::Input;
			}
		}

		quickState->Outputs.clear();
		quickState->Inputs.clear();

		// delete the quick state
		UUID id = quickState->ID;
		RemoveNode(quickState->ID);

		// re-ID the new state node to have the same ID as the old quick state node.
		// This ensures that the new state nodes remain selected in the UI
		// (same as the old quick state nodes were)
		state->ID = id;
		for (auto output : state->Outputs)
		{
			output->NodeID = state->ID;
		}

		for (auto input : state->Inputs)
		{
			input->NodeID = state->ID;
		}

		GetAllLinks()[state->ID] = animationGraph->m_Links[0];
		GetAllNodes()[state->ID] = animationGraph->m_Nodes[0];
		m_AnimationGraph->m_GraphState[state->ID] = animationGraph->m_GraphState[0];
	}


	void AnimationGraphNodeEditorModel::EnsureEntryStates()
	{
		AnimationGraph::PathHelper helper(m_RootNodeId, m_CurrentPath);
		for (auto& [id, nodes] : GetAllNodes())
		{
			Node* rootNode = id == 0 ? nullptr : FindInAllNodes(id);
			HZ_CORE_ASSERT(id == 0 || rootNode);

			if (rootNode && rootNode->GetTypeID() == AG::Types::ENodeType::StateMachine)
			{
				SetCurrentPath(rootNode);
				EnsureEntryState();
			}
		}
	}


	uint32_t AnimationGraphNodeEditorModel::FindBoneIndex(std::string_view boneName) const
	{
		if (boneName == "root")
			return 0;

		AssetHandle skeletonHandle = GetSkeletonHandle();
		auto skeletonAsset = AssetManager::GetAsset<SkeletonAsset>(skeletonHandle);
		if (skeletonAsset)
		{
			auto boneIndex = skeletonAsset->GetSkeleton().GetBoneIndex(boneName);
			if (boneIndex != Skeleton::NullIndex)
				return boneIndex + 1;
		}

		return Skeleton::NullIndex;
	}


	std::string AnimationGraphNodeEditorModel::GetNodeError(const Node* node)
	{
		if (!node)
		{
			HZ_CORE_ASSERT(false);
			return "node is null";
		}

		// A node with a sub-graph is well defined only if the sub-graph is well defined
		if (node->Type == NodeType::Subroutine)
		{
			if (!GetAllNodes().contains(node->ID) || GetAllNodes().at(node->ID).size() <= (node->GetTypeID() == AG::Types::State ? 1 : 0))
				return std::format("{} sub-graph has not been defined", node->Name);

			auto previousRootId = m_RootNodeId;
			m_RootNodeId = node->ID;
			const auto& subnodes = GetAllNodes().at(node->ID);

			for (auto& subNode : subnodes)
			{
				auto error = GetNodeError(subNode);
				if (!error.empty())
				{
					m_RootNodeId = previousRootId;
					return error;
				}
			}

			// State nodes must have an 'Output' node in the sub-graph
			if (node->GetTypeID() == AG::Types::State)
			{
				auto it = std::find_if(subnodes.begin(), subnodes.end(), [](const Node* n) { return n->Name == "Output"; });
				if (it == subnodes.end())
				{
					m_RootNodeId = previousRootId;
					return std::format("State '{}' graph has no 'Output' node", node->Name);
				}
			}

			// Transition nodes must have an 'Output' node in the sub-graph, and the transition condition must not be always false
			// (always true is allowed - this is a transition that happens straight away, it will still blend smoothly
			//  between source and destination states)
			if (node->GetTypeID() == AG::Types::Transition)
			{
				auto it = std::find_if(subnodes.begin(), subnodes.end(), [](const Node* n) { return n->Name == "Output"; });
				if (it == subnodes.end())
				{
					m_RootNodeId = previousRootId;
					return "Transition graph has no 'Output' node";
				}

				auto outputNode = *it;
				if (!IsPinLinked(outputNode->Inputs[0]->ID) && (!outputNode->Inputs[0]->Value.isBool() || !outputNode->Inputs[0]->Value.getBool()))
				{
					m_RootNodeId = previousRootId;
					return "Transition will never occur.";
				}
			}

			m_RootNodeId = previousRootId;
			return {};
		}

		// If node has an asset handle input that is null -> not well defined
		// If node has an input that is a pose and is not connected to anything -> not well defined
		// If node has an input that is a bone, and that bone is not found in the skeleton -> not well defined
		for (auto input : node->Inputs)
		{
			// note: Asset handle pins start off as integers with value 0.
			//       They're only asset handle values once user has edited them
			if (input->GetType() == AG::Types::EPinType::AnimationAsset)
			{
				AssetHandle assetHandle = 0;

				if (input->Storage != StorageKind::Array && Utils::IsAssetHandle<AssetType::Animation>(input->Value))
					assetHandle = Utils::GetAssetHandleFromValue(input->Value);

				if (!AssetManager::IsAssetHandleValid(assetHandle) && !GetNodeConnectedToPin(input->ID))
					return std::format("{} asset is not valid", input->Name);
			}

			if ((input->Name == "Pose") && !GetNodeConnectedToPin(input->ID))
				return std::format("{} input is not connected", input->Name);

			if (input->GetType() == AG::Types::EPinType::Bone)
			{
				HZ_CORE_ASSERT(input->Storage != StorageKind::Array, "Found bone pin that is an array.  This needs to be handled!");

				auto selectedBone = input->Value["Value"].getString();
				uint32_t boneIndex = FindBoneIndex(selectedBone);
				if (boneIndex == Skeleton::NullIndex)
					return std::format("{} '{}' not found in skeleton", input->Name, selectedBone);
			}
		}

		// If node has an output that is connected to something, it must be the same type
		for (auto output : node->Outputs)
		{
			Link* link = GetLinkConnectedToPin(output->ID);
			if (!link)
				continue;

			Pin* otherPin = FindPin(link->EndPinID);
			if (!otherPin)
				return std::format("{} output is connected to a pin that does not exist", output->Name);

			if (otherPin->GetType() != output->GetType())
				return std::format("{} output is connected to a pin of different type", output->Name);
		}
	
		return {};
	}


	bool AnimationGraphNodeEditorModel::Sort()
	{
		AnimationGraph::PathHelper helper(m_RootNodeId, m_CurrentPath);

		for (auto& [id, nodes] : GetAllNodes())
		{
			Node* rootNode = id == 0 ? nullptr : FindInAllNodes(id);
			HZ_CORE_ASSERT(id == 0 || rootNode);

			SetCurrentPath(rootNode);

			// State Machine graphs need the entry state first, then other states (in any order).
			if (rootNode && rootNode->GetTypeID() == AG::Types::ENodeType::StateMachine)
			{
				for (auto& node : nodes)
				{
					AG::Types::Node* stateNode = static_cast<AG::Types::Node*>(node);
					node->SortIndex = stateNode->IsEntryState? 0 : 1;
				}
			}
			else
			{
				for (auto& node : GetNodes())
					node->SortIndex = Node::UndefinedSortIndex;

				std::vector<Node*> visited;
				visited.reserve(GetNodes().size());

				uint32_t currentIndex = 0;

				auto correctSort = [&](Node* context, Node* previous, auto& correctSort) -> void
				{
					visited.push_back(context);

					// Local variables are a special case.
					// We treat all nodes corresponding to the same LV
					// as the same node
					if (IsLocalVariableNode(context))
					{
						std::vector<Node*> localVarNodes;
						std::vector<const Pin*> localVarOutputs;

						const auto isSetter = [](const Node* n) { return n->Inputs.size() == 1; };

						volatile int32_t localVarSortIndex = context->SortIndex;

						// Collect all of the nodes corresponding to this Local Variable
						for (auto& n : GetNodes())
						{
							if (IsSameLocalVariableNodes(n, context))
								localVarNodes.push_back(n);
						}

						// Go over inputs, which should be just one
						// because its value can reference only one output
						for (auto n : localVarNodes)
						{
							if (isSetter(n))
							{
								// Find node connected to this input
								Node* node = GetNodeConnectedToPin(n->Inputs[0]->ID);

								// Local variable setter might not have connected input,
								// we still need to assign it some valid sort index.
								if (!node && n->SortIndex == Node::UndefinedSortIndex)
								{
									n->SortIndex = currentIndex++;
									localVarSortIndex = n->SortIndex;
								}

								if (!node || (std::find(visited.begin(), visited.end(), node) != visited.end()) || (node->SortIndex != Node::UndefinedSortIndex)) break;

								correctSort(node, n, correctSort);

								if (n->SortIndex == Node::UndefinedSortIndex)
								{
									n->SortIndex = currentIndex++;
									localVarSortIndex = n->SortIndex;
								}

								// in some complex cases we can get into this situation, where we get to the same LV node from different branches
								if (context->SortIndex != Node::UndefinedSortIndex && localVarSortIndex == Node::UndefinedSortIndex)
									localVarSortIndex = context->SortIndex;

								break;
							}
						}
						if (localVarSortIndex == Node::UndefinedSortIndex)
							localVarSortIndex = 0;

						context->SortIndex = localVarSortIndex;

						// Collect outputs from all instances of getter nodes
						for (auto n : localVarNodes)
						{
							if (!isSetter(n))
							{
								localVarOutputs.push_back(n->Outputs[0]);
								if (n->SortIndex == Node::UndefinedSortIndex)
									n->SortIndex = context->SortIndex;
							}
						}
						// Go over collected outputs
						for (const auto out : localVarOutputs)
						{
							// Find node connected to this output
							Node* node = GetNodeConnectedToPin(out->ID);
							if (!node || node == previous || node->SortIndex != Node::UndefinedSortIndex) continue;

							correctSort(node, context, correctSort);
						}
					}
					else // Non Local Variable nodes are much simpler case
					{
						for (const auto in : context->Inputs)
						{
							// Find node connected to this input
							Node* node = GetNodeConnectedToPin(in->ID);
							if (!node || node == previous || node->SortIndex != Node::UndefinedSortIndex) continue;

							correctSort(node, context, correctSort);
						}

						if (context->SortIndex == Node::UndefinedSortIndex)
							context->SortIndex = currentIndex++;

						for (auto out : context->Outputs)
						{
							// Find node connected to this output
							Node* node = GetNodeConnectedToPin(out->ID);
							if (!node || node == previous || node->SortIndex != Node::UndefinedSortIndex) continue;

							correctSort(node, context, correctSort);
						}
					}
					visited.pop_back();
				};

				for (auto* node : GetNodes())
				{
					if (node->Name == "Output" || node->Name == "Event")
					{
						visited = {};
						node->SortIndex = currentIndex++;
						correctSort(node, nullptr, correctSort);
					}
				}
			}

			// Sort Nodes
			std::vector<Node*> sortedNodes = GetNodes();

			std::sort(sortedNodes.begin(), sortedNodes.end(), [&](const Node* first, const Node* second)
			{
				if (first->SortIndex == Node::UndefinedSortIndex)
					return false;
				if (second->SortIndex == Node::UndefinedSortIndex)
					return true;
				return first->SortIndex < second->SortIndex;
			});

			SetNodes(std::move(sortedNodes));

			for (uint32_t i = 0; i < GetNodes().size(); ++i)
			{
				auto node = GetNode(i);
				if (node->SortIndex == Node::UndefinedSortIndex)
					continue;

				node->SortIndex = i;
			}
		}

		// Sort Links
		for (auto& [id, links] : GetAllLinks())
		{
			Node* rootNode = id == 0 ? nullptr : FindInAllNodes(id);
			HZ_CORE_ASSERT(id == 0 || rootNode);

			// State Machine graph does not need links to be sorted
			if (rootNode && rootNode->GetTypeID() == AG::Types::StateMachine)
				continue;

			SetCurrentPath(rootNode);

			std::vector<Link> sortedLinks = GetLinks();
			std::sort(sortedLinks.begin(), sortedLinks.end(), [&](const Link& first, const Link& second)
			{
				bool isLess = false;

				auto getIndex = [](auto vec, auto valueFunc)
				{
					auto it = std::find_if(vec.begin(), vec.end(), valueFunc);
					HZ_CORE_ASSERT(it != vec.end());
					return it - vec.begin();
				};

				auto node1 = GetNodeForPin(first.StartPinID);
				auto node2 = GetNodeForPin(second.StartPinID);

				const size_t index1 = node1 ? getIndex(GetNodes(), [node1](const Node* n) { return n->ID == node1->ID; }) : ~0;
				const size_t index2 = node2 ? getIndex(GetNodes(), [node2](const Node* n) { return n->ID == node2->ID; }) : ~0;

				if (index1 != index2)
				{
					isLess = index1 < index2;
				}
				else
				{
					auto nodeEnd1 = GetNodeForPin(first.EndPinID);
					auto nodeEnd2 = GetNodeForPin(second.EndPinID);

					const size_t indexEnd1 = getIndex(GetNodes(), [node1](const Node* n) { return n->ID == node1->ID; });
					const size_t indexEnd2 = getIndex(GetNodes(), [node2](const Node* n) { return n->ID == node2->ID; });

					isLess = indexEnd1 < indexEnd2;
				}

				return isLess;
			});

			SetLinks(std::move(sortedLinks));
		}

		SetCurrentPath(nullptr);
		return true;
	}


	void AnimationGraphNodeEditorModel::Serialize()
	{
		Sort();
		AssetImporter::Serialize(m_AnimationGraph);
		AssetManager::ReloadData(m_AnimationGraph->Handle);
	}


	void AnimationGraphNodeEditorModel::Deserialize()
	{
		// graph is already loaded
	}


	const Nodes::AbstractFactory* AnimationGraphNodeEditorModel::GetNodeFactory() const
	{
		auto currentPath = GetCurrentPath();
		if (!currentPath.empty())
		{
			if (currentPath.back()->GetTypeID() == AG::Types::ENodeType::StateMachine)
			{
				return &m_StateMachineNodeFactory;
			}
			else if (currentPath.back()->GetTypeID() == AG::Types::ENodeType::Transition)
			{
				return &m_TransitionGraphNodeFactory;
			}
			else if (currentPath.back()->GetTypeID() == AG::Types::ENodeType::BlendSpace)
			{
				return &m_BlendSpaceNodeFactory;
			}
		}
		return &m_AnimationGraphNodeFactory;
	}


	void AnimationGraphNodeEditorModel::AssignSomeDefaultValue(Pin* pin)
	{
		if (!pin || pin->Kind == PinKind::Output)
			return;

		// First we try to get default value override from the factory
		if (const Node* node = FindNode(pin->NodeID))
		{
			if (auto optValueOverride = AG::GetPinDefaultValueOverride(choc::text::replace(node->Name, " ", ""), choc::text::replace(pin->Name, " ", "")))
			{
				pin->Value = *optValueOverride;
				return;
			}
		}

		if (pin->Storage == StorageKind::Array)
		{
			// TODO: JP. in the future we might want to store actual array type since this is editor-only nodes
			pin->Value = choc::value::Value();
			return;
		}

		// If we couldn't find the default value override in the factory,
		// we construct a default node of the same type and get the default value from it.
		const Pin* defaultPin = AG::Types::CreatePinForType(pin->GetType());

		pin->Value = defaultPin->Value;

		delete defaultPin;
	}


	void AnimationGraphNodeEditorModel::OnCompileGraph()
	{
		HZ_CORE_INFO_TAG("Animation", "Compiling graph...");

		if (onCompile)
		{
			if (onCompile(m_AnimationGraph.As<Asset>()))
			{
				InvalidatePlayer(false);

				HZ_CORE_INFO_TAG("Animation", "Graph has been compiled.");
				if (onCompiledSuccessfully)
					onCompiledSuccessfully();

				HZ_CONSOLE_LOG_TRACE("Animation graph successfully compiled.");

			}
			else
			{
				HZ_CONSOLE_LOG_ERROR("Failed to compile animation graph!");
			}
		}
		else
		{
			HZ_CONSOLE_LOG_ERROR("Failed to compile animation graph!");
		}
	}


	void AnimationGraphNodeEditorModel::EnsureEntryState()
	{
		HZ_CORE_ASSERT(m_CurrentPath.size() > 0 && m_CurrentPath.back()->GetTypeID() == AG::Types::StateMachine);
		bool bFoundEntry = std::find_if(GetNodes().begin(), GetNodes().end(), [](const Node* node) {
			const auto* stateNode = static_cast<const AG::Types::Node*>(node);
			return stateNode->IsEntryState;
		}) != GetNodes().end();
		if (!bFoundEntry)
		{
			// set the first non-transition node to be entry state
			for (auto node : GetNodes())
			{
				auto* stateNode = static_cast<AG::Types::Node*>(node);
				if (stateNode->GetTypeID() != AG::Types::Transition)
				{
					stateNode->IsEntryState = true;
					break;
				}
			}
		}
	}
}
