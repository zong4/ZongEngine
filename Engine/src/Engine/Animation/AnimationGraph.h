#pragma once

#include "Animation.h"
#include "AnimationNodeProcessor.h"
#include "Skeleton.h"

#include "AnimationNodeProcessor.h"

#include "Engine/Asset/Asset.h"

#include "Engine/Core/Base.h"
#include "Engine/Core/Identifier.h"
#include "Engine/Core/TimeStep.h"

#include <choc/containers/choc_SingleReaderSingleWriterFIFO.h>

#include <algorithm>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace Engine::AnimationGraph {

	struct IDs
	{
		static constexpr Identifier Event{ "Event" };
		static constexpr Identifier Pose{ "Pose" };
		static constexpr Identifier QuickState{ "Quick State" };
		static constexpr Identifier State{ "State" };
		static constexpr Identifier StateMachine{ "State Machine" };
		static constexpr Identifier Transition{ "Transition" };

		IDs() = delete;
	};

	// base "graph" for animations.
	// Used for both AnimationGraph (a graph that produces animation poses)
	// and TranistionGraph (a graph that evaluates whether a transition should occur)
	struct Graph : public NodeProcessor, public RefCounted
	{
		Graph(std::string_view dbgName, UUID id);

		std::vector<Scope<NodeProcessor>> Nodes;
		std::vector<Scope<StreamWriter>> EndpointInputStreams;
		NodeProcessor EndpointOutputStreams;
		std::vector<Scope<StreamWriter>> LocalVariables;

		OutputEvent out_Event;

		NodeProcessor* FindNodeByID(UUID id);

	public:
		//==============================================================================
		/// Graph Construction Public API

		template<typename T>
		void AddGraphInputStream(Identifier id, T&& externalObjectOrDefaultValue)
		{
			AddInStream(id) = EndpointInputStreams.emplace_back(new StreamWriter(std::forward<T>(externalObjectOrDefaultValue), id))->outV;
		}

		template<typename T>
		void AddGraphOutputStream(Identifier id, T& externalObjectOrDefaultValue)
		{
			AddOutStream(id, externalObjectOrDefaultValue);
			EndpointOutputStreams.AddInStream(id);

			AddConnection(Outs.at(id), EndpointOutputStreams.Ins.at(id));
		}

		template<typename T>
		void AddLocalVariableStream(Identifier id, T&& externalObjectOrDefaultValue)
		{
			LocalVariables.emplace_back(new StreamWriter(std::forward<T>(externalObjectOrDefaultValue), id));
		}

		void AddNode(Scope<NodeProcessor>&& node);
		void AddNode(NodeProcessor*&& node);

	public:
		//==============================================================================
		/// Graph Connections Public API

		// Node Output Value -> Node Input Value
		bool AddValueConnection(UUID sourceNodeID, Identifier sourceEndpointID, UUID destinationNodeID, Identifier destinationEndpointID) noexcept;

		// Node Output Event -> Node Input Event
		bool AddEventConnection(UUID sourceNodeID, Identifier sourceEndpointID, UUID destinationNodeID, Identifier destinationEndpointID) noexcept;

		// Graph Input Value -> Node Input Value
		bool AddInputValueRoute(const std::vector<Scope<StreamWriter>>& endpointInputStreams, Identifier graphInputID, UUID destinationNodeID, Identifier destinationEndpointID) noexcept;

		// Graph Input Value -> Graph Output Value
		bool AddInputValueRouteToGraphOutput(const std::vector<Scope<StreamWriter>>& endpointInputStreams, Identifier graphInputID, Identifier graphOutValueID) noexcept;

		// Graph Input Event -> Node Input Event
//		bool AddInputEventsRoute(Identifier graphInputEventID, UUID destinationNodeID, Identifier destinationEndpointID) noexcept;

		// Node Output Value -> Graph Output Value
		bool AddToGraphOutputConnection(UUID sourceNodeID, Identifier sourceEndpointID, Identifier graphOutValueID);

		/** Node Output Event -> Graph Output Event */
		bool AddToGraphOutEventConnection(UUID sourceNodeID, Identifier sourceEndpointID, Ref<Graph> root, Identifier graphOutEventID);

		/** Graph Local Variable (StreamWriter) -> Node Input Value. Local Variable must not be of function type! */
		bool AddLocalVariableRoute(Identifier graphLocalVariableID, UUID destinationNodeID, Identifier destinationEndpointID) noexcept;

	public:
		//==============================================================================
		/// (Animation)NodeProcessor Public API
		void Init() override;
		float Process(float timestep) override;

	public:
		using HandleOutgoingEventFn = void(void* userContext, Identifier eventID);

		//Flushes any outgoing events that are currently queued.
		// This must be called periodically if the graph is generating events, otherwise
		// the FIFO will overflow.
		void HandleOutgoingEvents(void* userContext, HandleOutgoingEventFn* handleEvent);

	private:
		//==============================================================================
		/// Graph Connections Internal Methods

		void AddConnection(OutputEvent& source, InputEvent& destination) noexcept;
		void AddConnection(const choc::value::ValueView& source, choc::value::ValueView& destination) noexcept;

		void AddRoute(InputEvent& source, InputEvent& destination) noexcept;
		void AddRoute(OutputEvent& source, OutputEvent& destination, Identifier eventID) noexcept;

		choc::fifo::SingleReaderSingleWriterFIFO<Identifier> OutgoingEvents;
	};


	// AnimationGraph is responsible for updating a character's AnimationData each frame
	// It is a directed graph of nodes that defines the "logic" for how to evaluate the pose
	// that a rigged mesh should be placed into each frame
	struct AnimationGraph : public Graph
	{
		AnimationGraph(std::string_view dbgName, UUID id, AssetHandle skeleton);
		AnimationGraph(std::string_view dbgName, UUID id, const Skeleton* skeleton);

	public:
		void Init() final;
		const Skeleton* GetSkeleton() const;

		float GetAnimationDuration() const override;
		float GetAnimationTimePos() const override;
		void SetAnimationTimePos(float timePos) override;

		const Pose* GetPose() const;

	private:
		const Skeleton* m_Skeleton = nullptr;
		bool m_IsInitialized = false;

	};


	// TransitionGraph is responsible for evaluating whether a state transition should occur
	struct TransitionGraph : public Graph
	{
		TransitionGraph(std::string_view dbgName, UUID id);

	public:
		void Init() final;

	private:

	};

} // namespace Engine::AnimationGraph
