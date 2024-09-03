#pragma once

#include "NodeProcessor.h"
#include "WaveSource.h"
#include "Nodes/WavePlayer.h"

#include "containers/choc_SingleReaderSingleWriterFIFO.h"

#define LOG_DBG_MESSAGES 0

#if LOG_DBG_MESSAGES
#define DBG(...) HZ_CORE_WARN(__VA_ARGS__)
#else
#define DBG(...)
#endif

#define DECLARE_ID(name) static constexpr Identifier name{ #name }

namespace Hazel::SoundGraph
{
	//==============================================================================
	/// Raw Sound Graph containing Inputs, Outputs and Nodes
	struct SoundGraph final : public NodeProcessor, public RefCounted
	{
		struct IDs
		{
			DECLARE_ID(InLeft);
			DECLARE_ID(InRight);
			DECLARE_ID(OutLeft);
			DECLARE_ID(OutRight);

			DECLARE_ID(Play);

			DECLARE_ID(OnFinished);
		private:
			IDs() = delete;
		};

		explicit SoundGraph(std::string_view dbgName, UUID id)
			: NodeProcessor(dbgName.data(), id), EndpointOutputStreams(*this)
		{
			AddInEvent(IDs::Play);

			out_OnFinish.AddDestination(std::make_shared<InputEvent>(*this,
			[&](float v)
			{
				static constexpr float dummyValue = 1.0f;
				choc::value::ValueView value(choc::value::Type::createFloat32(), (void*)&dummyValue, nullptr);
				OutgoingEvents.push({ CurrentFrame, IDs::OnFinished, value });
			}));
			
			AddOutEvent(IDs::OnFinished, out_OnFinish);

			OutgoingEvents.reset(1024);
			OutgoingMessages.reset(1024);

			out_Channels.reserve(2);
		}

		OutputEvent out_OnFinish{ *this };

		std::vector<std::unique_ptr<NodeProcessor>> Nodes;
		std::vector<WavePlayer*> WavePlayers;
		std::vector<std::unique_ptr<StreamWriter>> EndpointInputStreams;
		NodeProcessor EndpointOutputStreams; // TODO: outputs might get invalidated if the graph is ever relocated

		struct InterpolatedValue
		{
			float current;
			float target;
			float increment;
			int steps = 0;
			StreamWriter* endpoint;
			void SetTarget(float newTarget, int numSteps) noexcept
			{
				target = newTarget;
				increment = (target - current) / numSteps;
				steps = numSteps;
			}
			void Reset(float initialValue) noexcept
			{
				current = initialValue;
				target = initialValue;
				increment = 0.0f;
				steps = 0;
			}

			inline void Process() noexcept
			{
				if (steps > 0)
				{
					current += increment;
					steps--;

					if (steps == 0)
						current = target;

					*endpoint << current;
				}
			}
		};
		std::unordered_map<Identifier, InterpolatedValue> InterpInputs;

		std::vector<std::unique_ptr<StreamWriter>> LocalVariables;

		std::vector<Identifier> OutputChannelIDs;

		/** Reset Graph to its initial state. */
		// TODO: create new clone from the Prototype, or somehow reset graph to its initial snapshot state?
		void Reset()
		{
			OutgoingEvents.reset(1024);
			OutgoingMessages.reset(1024);
		}
		
		//==============================================================================
		NodeProcessor* FindNodeByID(UUID id)
		{
			auto it = std::find_if(Nodes.begin(), Nodes.end(),
								   [id](const std::unique_ptr<NodeProcessor>& nodePtr)
								   {
									   return nodePtr->ID == id;
								   });

			if (it != Nodes.end())
				return it->get();
			else
				return nullptr;
		}

		const NodeProcessor* FindNodeByID(UUID id) const
		{
			auto it = std::find_if(Nodes.begin(), Nodes.end(),
			   [id](const std::unique_ptr<NodeProcessor>& nodePtr)
			{
				return nodePtr->ID == id;
			});

			if (it != Nodes.end())
				return it->get();
			else
				return nullptr;
		}

		//==============================================================================
		/// Graph Construction Public API
		
		template<typename T>
		void AddGraphInputStream(Identifier id, T&& externalObjectOrDefaultValue)
		{
			// TODO: concider replacing this vector with NodeProcessor with dynamic topology?
			EndpointInputStreams.emplace_back(new StreamWriter(AddInStream(id), std::forward<T>(externalObjectOrDefaultValue), id));

			if (std::is_same_v<T, float>)
			{
				// TODO: we don't want this interpolation for nested graphs!
				InterpInputs.try_emplace(id, InterpolatedValue{ 0.0f, 0.0f, 0.0f, 0, EndpointInputStreams.back().get() });
			}
		}

		template<>
		void AddGraphInputStream(Identifier id, choc::value::Value&& externalObjectOrDefaultValue)
		{
			// TODO: might want to support doubles in the future, for time type of inputs
			const bool isFloat = externalObjectOrDefaultValue.isFloat32();

			// TODO: consider replacing this vector with NodeProcessor with dynamic topology?
			EndpointInputStreams.emplace_back(new StreamWriter(AddInStream(id), std::forward<choc::value::Value>(externalObjectOrDefaultValue), id));

			if (isFloat)
			{
				// TODO: we don't want this interpolation for nested graphs!
				InterpInputs.try_emplace(id, InterpolatedValue{ 0.0f, 0.0f, 0.0f, 0, EndpointInputStreams.back().get() });
			}
		}

		std::vector<float> out_Channels;
		void AddGraphOutputStream(Identifier id)
		{
			AddOutStream<float>(id, out_Channels.emplace_back(0.0f));
			EndpointOutputStreams.AddInStream(id);

			AddConnection(Outs.at(id), EndpointOutputStreams.Ins.at(id));
		}

		template<typename T>
		void AddLocalVariableStream(Identifier id, T&& externalObjectOrDefaultValue)
		{
			choc::value::Value dummy;
			LocalVariables.emplace_back(new StreamWriter(dummy.getViewReference(), std::forward<T>(externalObjectOrDefaultValue), id));
		}

		void AddNode(std::unique_ptr<NodeProcessor>&& node)
		{
			HZ_CORE_ASSERT(node);
			Nodes.emplace_back(std::move(node));
		}

		void AddNode(NodeProcessor*&& node)
		{
			HZ_CORE_ASSERT(node);
			Nodes.emplace_back(std::unique_ptr<NodeProcessor>(std::move(node)));
		}

	private:
		//==============================================================================
		/// Graph Connections Internal Methods

		void AddConnection(OutputEvent& source, InputEvent& destination) noexcept
		{
			source.AddDestination(std::make_shared<InputEvent>(destination));
		}

		void AddConnection(choc::value::ValueView& source, choc::value::ValueView& destination) noexcept
		{
			destination = source;
		}

		/** Connect Input Event to Input Event */
		void AddRoute(InputEvent& source, InputEvent& destination) noexcept
		{
			InputEvent* dest = &destination;
			source.Event = [dest](float v) { (*dest)(v); };
		}

		/** Connect Output Event to Output Event */
		void AddRoute(OutputEvent& source, OutputEvent& destination) noexcept
		{
			OutputEvent* dest = &destination;
			source.AddDestination(std::make_shared<InputEvent>(*this, [dest](float v) { (*dest)(v); }));
		}

	public:
		//==============================================================================
		/// Graph Connections Public API

		/** Node Output Value -> Node Input Value */
		bool AddValueConnection(UUID sourceNodeID, Identifier sourceNodeEndpointID, UUID destinationNodeID, Identifier destinationNodeEndpointID) noexcept
		{
			auto* sourceNode = FindNodeByID(sourceNodeID);
			auto* destinationNode = FindNodeByID(destinationNodeID);

			if (!sourceNode || !destinationNode)
			{
				HZ_CORE_ASSERT(false);
				return false;
			}

			AddConnection(sourceNode->OutValue(sourceNodeEndpointID), destinationNode->InValue(destinationNodeEndpointID));
			return true;
		}

		/** Node Output Event -> Node Input Event */
		bool AddEventConnection(UUID sourceNodeID, Identifier sourceNodeEndpointID, UUID destinationNodeID, Identifier destinationNodeEndpointID) noexcept
		{
			auto* sourceNode = FindNodeByID(sourceNodeID);
			auto* destinationNode = FindNodeByID(destinationNodeID);

			if (!sourceNode || !destinationNode)
			{
				HZ_CORE_ASSERT(false);
				return false;
			}

			AddConnection(sourceNode->OutEvent(sourceNodeEndpointID), destinationNode->InEvent(destinationNodeEndpointID));
			return true;
		}

		/** Graph Input Value -> Node Input Value */
		bool AddInputValueRoute(Identifier graphInputEventID, UUID destinationNodeID, Identifier destinationNodeEndpointID) noexcept
		{
			auto* destinationNode = FindNodeByID(destinationNodeID);
			auto endpoint = std::find_if(EndpointInputStreams.begin(), EndpointInputStreams.end(),
										 [graphInputEventID](const std::unique_ptr<StreamWriter>& endpoint) { return endpoint->DestinationID == graphInputEventID; });
			

			if (!destinationNode || endpoint == EndpointInputStreams.end())
			{
				HZ_CORE_ASSERT(false);
				return false;
			}

			AddConnection(/*InValue(graphInputEventID)*/ (*endpoint)->outV.getViewReference(), destinationNode->InValue(destinationNodeEndpointID));
			return true;
		}

		/** Graph Input Event -> Node Input Event */
		bool AddInputEventsRoute(Identifier graphInputEventID, UUID destinationNodeID, Identifier destinationNodeEndpointID) noexcept
		{
			auto* destinationNode = FindNodeByID(destinationNodeID);

			if (!destinationNode)
			{
				HZ_CORE_ASSERT(false);
				return false;
			}

			AddRoute(InEvent(graphInputEventID), destinationNode->InEvent(destinationNodeEndpointID));
			return true;
		}

		/** Node Output Value -> Graph Output Value */
		bool AddToGraphOutputConnection(UUID sourceNodeID, Identifier sourceNodeEndpointID, Identifier graphOutValueID)
		{
			auto* sourceNode = FindNodeByID(sourceNodeID);

			if (!sourceNode)
			{
				HZ_CORE_ASSERT(false);
				return false;
			}

			AddConnection(sourceNode->OutValue(sourceNodeEndpointID), EndpointOutputStreams.InValue(graphOutValueID));
			return true;
		}

		/** Node Output Event -> Graph Output Event */
		bool AddToGraphOutEventConnection(UUID sourceNodeID, Identifier sourceNodeEndpointID, Identifier graphOutEventID)
		{
			auto* sourceNode = FindNodeByID(sourceNodeID);
			if (!sourceNode)
			{
				HZ_CORE_ASSERT(false);
				return false;
			}

			AddRoute(sourceNode->OutEvent(sourceNodeEndpointID), OutEvent(graphOutEventID));
			return true;
		}

		/** Graph Local Variable (StreamWriter) -> Node Input Value. Local Variable must not be of function type! */
		bool AddLocalVariableRoute(Identifier graphLocalVariableID, UUID destinationNodeID, Identifier destinationNodeEndpointID) noexcept
		{
			auto* destinationNode = FindNodeByID(destinationNodeID);
			auto endpoint = std::find_if(LocalVariables.begin(), LocalVariables.end(),
										 [graphLocalVariableID](const std::unique_ptr<StreamWriter>& endpoint) { return endpoint->DestinationID == graphLocalVariableID; });

			if (!destinationNode || endpoint == LocalVariables.end())
			{
				HZ_CORE_ASSERT(false);
				return false;
			}

			AddConnection(/*InValue(graphInputEventID)*/ (*endpoint)->outV.getViewReference(), destinationNode->InValue(destinationNodeEndpointID));
			return true;
		}

		//==============================================================================
		void Init() final
		{
			for (auto& node : Nodes)
			{
				if (auto* wavePlayer = dynamic_cast<WavePlayer*>(node.get()))
					WavePlayers.push_back(wavePlayer);
			}

			// Initialization order implied by the order of adding nodes to the graph
			for (auto& node : Nodes)
				node->Init();

			bIsInitialized = true;
		}

		void BeginProcessBlock()
		{
			for (auto& wavePlayer : WavePlayers)
				wavePlayer->buffer.Refill();
		}

		void Process() final
		{
			for (std::pair<const Identifier, InterpolatedValue>& interpValue : InterpInputs)
				interpValue.second.Process();

			for (auto& node : Nodes)
				node->Process(); //? This unique_ptr-> access is very heavy to do every frame

			++CurrentFrame;
		}

		// This should reset nodes to their initial state
		void Reinit()
		{
			OutgoingEvents.reset();
			OutgoingMessages.reset();

			for (auto& node : Nodes)
				node->Init();
		}

		//==============================================================================
		/// Graph Render Public API

		bool IsPlayable() const { return bIsInitialized; }

		/** Used in handleOutgoingEvents. */
		using HandleOutgoingEventFn = void(void* userContext, uint64_t frameIndex, Identifier endpointName, const choc::value::ValueView&);
		/** Used in handleOutgoingEvents. */
		using HandleConsoleMessageFn = void(void* userContext, uint64_t frameIndex, const char* message);

		/**
			Flushes any outgoing event and console messages that are currently queued.
			This must be called periodically if the graph is generating events, otherwise
			the FIFO will overflow.
		*/
		void HandleOutgoingEvents(void* userContext, HandleOutgoingEventFn* handleEvent, HandleConsoleMessageFn* handleConsoleMessage)
		{
			OutgoingEvent outEvent;
			while (OutgoingEvents.pop(outEvent))
				handleEvent(userContext, outEvent.FrameIndex, outEvent.EndpointID, outEvent.value);

			OutgoingMessage outMessage;
			while (OutgoingMessages.pop(outMessage))
				handleConsoleMessage(userContext, outMessage.FrameIndex, outMessage.Message);
		}

		// TODO: should this be queued and processed at the beginning of the next callback?
		bool SendInputValue(uint32_t endpointID, choc::value::ValueView value, bool interpolate)
		{
			auto endpoint = std::find_if(EndpointInputStreams.begin(), EndpointInputStreams.end(),
										 [endpointID](const std::unique_ptr<StreamWriter>& endpoint)
										 {
											 return (uint32_t)endpoint->DestinationID == endpointID;
										 });
			if (endpoint == EndpointInputStreams.end())
				return false;

			if (value.isFloat32()) // TODO: the future, might want to support doubles in for time variable type
			{
				// interpolate
				auto& interpInput = InterpInputs.at(endpoint->get()->DestinationID);
				if (interpolate)
				{
					interpInput.SetTarget(value.getFloat32(), 480); // TODO: get this 480 samples (10ms) from actual current samplerate
				}
				else
				{
					interpInput.Reset(value.getFloat32());
					*(*endpoint) << value;
				}
			}
			else
			{
				*(*endpoint) << value;
			}
			
			return true;
		}

		// TODO: should this be queued and processed at the beginning of the next callback?
		bool SendInputEvent(Identifier endpointID, choc::value::ValueView value)
		{
			auto endpoint = InEvs.find(endpointID);

			if (endpoint == InEvs.end() || !endpoint->second.Event)
				return false;

			// TODO: for now only handling floats, might remove function argument in the future
			endpoint->second((float)value);
			return true;
		}

		std::vector<Identifier> GetInputEventEndpoints() const
		{
			std::vector<Identifier> handles;
			handles.reserve(InEvs.size());

			for (const auto& [handle, endpoint] : InEvs)
				handles.push_back(handle);

			return handles;
		}

		std::vector<Identifier> GetParameters() const
		{
			std::vector<Identifier> handles;
			handles.reserve(EndpointInputStreams.size());

			for (const auto& endpoint : EndpointInputStreams)
				handles.push_back(endpoint->DestinationID);

			return handles;
		}

		using RefillCallback = bool(*)(Audio::WaveSource&, void* userData, uint32_t numFrames);
		void SetRefillWavePlayerBufferCallback(RefillCallback callback, void* userData, uint32_t numFrames)
		{
			for (auto& node : Nodes)
			{
				if (auto* wavePlayer = dynamic_cast<WavePlayer*>(node.get()))
					wavePlayer->buffer.onRefill = [callback, userData, numFrames](Audio::WaveSource& source) { return callback(source, userData, numFrames); };
			}
		}

	private:
		bool bIsInitialized = false;
		uint64_t CurrentFrame = 0;

		// TODO: provide access to outgoing messages and events queues thoughout the graph
		struct OutgoingEvent
		{
			uint64_t FrameIndex;
			Identifier EndpointID;
			choc::value::ValueView value;
		};
		struct OutgoingMessage
		{
			uint64_t FrameIndex;
			const char* Message;
		};
		choc::fifo::SingleReaderSingleWriterFIFO<OutgoingEvent> OutgoingEvents;
		choc::fifo::SingleReaderSingleWriterFIFO<OutgoingMessage> OutgoingMessages;

	};

} // namespace Hazel::SoundGraph

#undef DECLARE_ID
#undef LOG_DBG_MESSAGES
#undef DBG
