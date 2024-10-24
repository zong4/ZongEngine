#include "PhysicsStatsPanel.h"

#include "Engine/ImGui/ImGui.h"
#include "Engine/Debug/Profiler.h"

namespace Engine {

	PhysicsStatsPanel::PhysicsStatsPanel() {}
	PhysicsStatsPanel::~PhysicsStatsPanel()
	{
		m_PhysicsScene = nullptr;
	}

	void PhysicsStatsPanel::SetSceneContext(const Ref<Scene>& context)
	{
		if (context == nullptr)
			m_PhysicsScene = nullptr;
		else
			m_PhysicsScene = context->GetPhysicsScene();
	}

	void PhysicsStatsPanel::OnImGuiRender(bool& isOpen)
	{
		ZONG_PROFILE_FUNC();

		/*if (ImGui::Begin("Physics Stats", &isOpen) && m_PhysicsScene->IsValid())
		{
#ifndef ZONG_DIST
			const auto& stats = m_PhysicsScene->GetSimulationStats();

			if (UI::PropertyGridHeader("General Statistics"))
			{
				ImGui::Text("Active Constraints: %d", stats.nbActiveConstraints);
				ImGui::Text("Active Dynamic Actors: %d", stats.nbActiveDynamicBodies);
				ImGui::Text("Active Kinematic Actors: %d", stats.nbActiveKinematicBodies);
				ImGui::Text("Static Actors: %d", stats.nbStaticBodies);
				ImGui::Text("Dynamic Actors: %d", stats.nbDynamicBodies);
				ImGui::Text("Kinematic Actors: %d", stats.nbKinematicBodies);

				for (size_t i = 0; i < physx::PxGeometryType::eGEOMETRY_COUNT; i++)
					ImGui::Text("%s Shapes: %d", PhysXUtils::PhysXGeometryTypeToString((physx::PxGeometryType::Enum)i), stats.nbShapes[i]);

				ImGui::Text("Aggregates: %d", stats.nbAggregates);
				ImGui::Text("Articulations: %d", stats.nbArticulations);
				ImGui::Text("Axis Solver Constraints: %d", stats.nbAxisSolverConstraints);
				ImGui::Text("Compressed Contact Size (Bytes): %d", stats.compressedContactSize);
				ImGui::Text("Required Contact Constraint Size (Bytes): %d", stats.requiredContactConstraintMemory);
				ImGui::Text("Peak Constraint Size (Bytes): %d", stats.peakConstraintMemory);

				ImGui::TreePop();
			}

			ImGui::Spacing();
			ImGui::Separator();
			ImGui::Spacing();

			static char s_SearchBuffer[256];
			ImGui::SetNextItemWidth(-1);
			ImGui::InputTextWithHint("##actorsearch", "Search Actors, Shapes, etc...", s_SearchBuffer, 255);

			const auto& actors = m_PhysicsScene->GetActors();

			size_t searchLength = strlen(s_SearchBuffer);

			if (UI::PropertyGridHeader(fmt::format("Static Actors ({0})", stats.nbStaticBodies), searchLength > 0))
			{
				for (const auto& [entityID, actor] : actors)
				{
					if (actor->IsDynamic())
						continue;

					const auto& tag = actor->GetEntity().GetComponent<TagComponent>().Tag;

					if (searchLength > 0)
					{
						std::string searchBuffer = s_SearchBuffer;
						Utils::String::ToLower(searchBuffer);
						if (Utils::String::ToLowerCopy(tag).find(searchBuffer) == std::string::npos)
							continue;
					}

					std::string label = fmt::format("{0}##{1}", tag, entityID);

					if (UI::PropertyGridHeader(label, false))
					{
						UI::BeginPropertyGrid();
						UI::BeginDisabled();
						glm::vec3 translation = actor->GetTranslation();
						glm::quat rotation = actor->GetRotation();
						glm::vec3 rotationEuler = glm::eulerAngles(rotation);
						UI::Property("Translation", translation);
						UI::Property("Rotation", rotationEuler);
						UI::EndDisabled();
						UI::EndPropertyGrid();

						const auto& collisionShapes = actor->GetCollisionShapes();

						ImGui::Text("Shapes: %d", collisionShapes.size());
						for (const auto& shape : collisionShapes)
						{
							std::string shapeLabel = fmt::format("{0}##{1}", shape->GetShapeName(), entityID);
							bool shapeOpen = UI::PropertyGridHeader(shapeLabel, false);
							if (shapeOpen)
							{
								UI::BeginPropertyGrid();
								UI::BeginDisabled();

								glm::vec3 offset = shape->GetOffset();
								bool isTrigger = shape->IsTrigger();

								UI::Property("Offset", offset);
								UI::Property("Is Trigger", isTrigger);

								const auto& material = shape->GetMaterial();
								float staticFriction = material.getStaticFriction();
								float dynamicFriction = material.getDynamicFriction();
								float restitution = material.getRestitution();

								UI::Property("Static Friction", staticFriction);
								UI::Property("Dynamic Friction", staticFriction);
								UI::Property("Restitution", restitution);

								UI::EndDisabled();
								UI::EndPropertyGrid();
								ImGui::TreePop();
							}
						}

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}
			
			if (UI::PropertyGridHeader(fmt::format("Dynamic Actors ({0})", stats.nbDynamicBodies), searchLength > 0))
			{
				for (const auto& [entityID, actor] : actors)
				{
					if (!actor->IsDynamic())
						continue;

					const auto& tag = actor->GetEntity().GetComponent<TagComponent>().Tag;

					if (searchLength > 0)
					{
						std::string searchBuffer = s_SearchBuffer;
						Utils::String::ToLower(searchBuffer);
						if (Utils::String::ToLowerCopy(tag).find(searchBuffer) == std::string::npos)
							continue;
					}

					std::string label = fmt::format("{0}##{1}", tag, entityID);
					if (UI::PropertyGridHeader(label, false))
					{
						UI::BeginPropertyGrid();
						UI::BeginDisabled();

						glm::vec3 translation = actor->GetTranslation();
						glm::quat rotation = actor->GetRotation();
						glm::vec3 rotationEuler = glm::eulerAngles(rotation);
						UI::Property("Translation", translation);
						UI::Property("Rotation", rotationEuler);

						bool isKinematic = actor->IsKinematic();
						UI::Property("Is Kinematic", isKinematic);

						float mass = actor->GetMass();
						UI::Property("Mass", mass);
						float inverseMass = actor->GetInverseMass();
						UI::Property("Inverse Mass", inverseMass);

						bool hasGravity = actor->IsGravityEnabled();
						UI::Property("Has Gravity", hasGravity);

						bool isSleeping = actor->IsSleeping();
						UI::Property("Is Sleeping", isSleeping);

						glm::vec3 linearVelocity = actor->GetLinearVelocity();
						float maxLinearVelocity = actor->GetMaxLinearVelocity();
						glm::vec3 angularVelocity = actor->GetAngularVelocity();
						float maxAngularVelocity = actor->GetMaxAngularVelocity();
						UI::Property("Linear Velocity", linearVelocity);
						UI::Property("Max Linear Velocity", maxLinearVelocity);
						UI::Property("Angular Velocity", angularVelocity);
						UI::Property("Max Angular Velocity", maxAngularVelocity);

						float linearDrag = actor->GetLinearDrag();
						float angularDrag = actor->GetAngularDrag();
						UI::Property("Linear Drag", linearDrag);
						UI::Property("Angular Drag", angularDrag);

						UI::EndDisabled();
						UI::EndPropertyGrid();

						const auto& collisionShapes = actor->GetCollisionShapes();

						ImGui::Text("Shapes: %d", collisionShapes.size());

						for (const auto& shape : collisionShapes)
						{
							std::string shapeLabel = fmt::format("{0}##{1}", shape->GetShapeName(), entityID);
							if (UI::PropertyGridHeader(shapeLabel, false))
							{
								UI::BeginPropertyGrid();
								UI::BeginDisabled();

								glm::vec3 offset = shape->GetOffset();
								bool isTrigger = shape->IsTrigger();

								UI::Property("Offset", offset);
								UI::Property("Is Trigger", isTrigger);

								const auto& material = shape->GetMaterial();
								float staticFriction = material.getStaticFriction();
								float dynamicFriction = material.getDynamicFriction();
								float restitution = material.getRestitution();

								UI::Property("Static Friction", staticFriction);
								UI::Property("Dynamic Friction", staticFriction);
								UI::Property("Restitution", restitution);

								UI::EndDisabled();
								UI::EndPropertyGrid();
								ImGui::TreePop();
							}
						}

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}

			const auto& joints = m_PhysicsScene->GetJoints();

			if (UI::PropertyGridHeader(fmt::format("Joints ({0})", joints.size()), searchLength > 0))
			{
				for (const auto& [entityID, joint] : joints)
				{
					const auto& tag = joint->GetEntity().GetComponent<TagComponent>().Tag;

					if (searchLength > 0)
					{
						std::string searchBuffer = s_SearchBuffer;
						Utils::String::ToLower(searchBuffer);
						if (Utils::String::ToLowerCopy(tag).find(searchBuffer) == std::string::npos
							&& Utils::String::ToLowerCopy(joint->GetDebugName()).find(searchBuffer) == std::string::npos)
						{
							continue;
						}
					}

					std::string label = fmt::format("{0} ({1})##{1}", joint->GetDebugName(), tag, entityID);
					if (UI::PropertyGridHeader(label, false))
					{
						UI::BeginPropertyGrid();
						UI::BeginDisabled();

						bool isBreakable = joint->IsBreakable();
						UI::Property("Is Breakable", isBreakable);

						if (isBreakable)
						{
							bool isBroken = joint->IsBroken();
							UI::Property("Is Broken", isBroken);

							float breakForce, breakTorque;
							joint->GetBreakForceAndTorque(breakForce, breakTorque);
							UI::Property("Break Force", breakForce);
							UI::Property("Break Torque", breakTorque);
						}

						bool isCollisionEnabled = joint->IsCollisionEnabled();
						UI::Property("Is Collision Enabled", isCollisionEnabled);

						bool isPreProcessingEnabled = joint->IsPreProcessingEnabled();
						UI::Property("Is Preprocessing Enabled", isPreProcessingEnabled);

						UI::EndDisabled();
						UI::EndPropertyGrid();

						ImGui::TreePop();
					}
				}

				ImGui::TreePop();
			}
#endif
		}

		ImGui::End();*/
	}

}
