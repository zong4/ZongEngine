#pragma once

// To create a new type of Animation Node:
// 1) Create a header file (or add to existing) and declare the new node type (derived from NodeProcessor).
// 2) At the end of the header file, "describe" the new node using node descriptor macros (refer AnimationNodes.h for an example)
// 3) Create a source file (or add to existing) and add the implementation  of the new node type's functions.
// 4) declare the new node's pin types and default values (for input pins) in Editor/NodeGraphEditor/AnimationGraph/AnimationGraphNodes.cpp
// 5) register the new node type in the node registry for runtime (Hazel::AnimationGraph::Registry, in Animation/AnimationGraphFactory.cpp)
// 6) register the new node type in the node registry for editor (Hazel::NodeGraph::Factory<Hazel::AnimationGraph::AnimationGraphNodeFactory>::Registry, in Editor/NodeGraphEditor/AnimationGraph/AnimationGraphNodes.cpp)

#include "AnimationNodes.h"
#include "ArrayNodes.h"
#include "BlendNodes.h"
#include "IKNodes.h"
#include "LogicNodes.h"
#include "MathNodes.h"
#include "StateMachineNodes.h"
#include "TriggerNodes.h"
