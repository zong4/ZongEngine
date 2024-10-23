#pragma once

#include "EditorPanel.h"

#include "Engine/Scene/Scene.h"
#include "Engine/Renderer/Mesh.h"
#include "Engine/Editor/SelectionManager.h"
#include "Engine/Editor/EditorResources.h"
#include "Engine/ImGui/UICore.h"
#include "Engine/ImGui/ImGui.h"
#include "Engine/ImGui/CustomTreeNode.h"


namespace Hazel {

	// NOTE(Peter): Stolen from imgui.h since IM_COL32 isn't available in this header
	#define COLOR32(R,G,B,A)    (((ImU32)(A)<<24) | ((ImU32)(B)<<16) | ((ImU32)(G)<<8) | ((ImU32)(R)<<0))

	enum class VectorAxis
	{
		X = BIT(0),
		Y = BIT(1),
		Z = BIT(2),
		W = BIT(3)
	};

	namespace AnimationGraph {
		const std::string& GetName(Identifier id);
	}

	class SceneHierarchyPanel : public EditorPanel
	{
	public:
		SceneHierarchyPanel() = default;
		SceneHierarchyPanel(const Ref<Scene>& scene, SelectionContext selectionContext, bool isWindow = true);

		virtual void SetSceneContext(const Ref<Scene>& scene) override;
		Ref<Scene> GetSceneContext() const { return m_Context; }

		void SetEntityDeletedCallback(const std::function<void(Entity)>& func) { m_EntityDeletedCallback = func; }
		void SetMeshAssetConvertCallback(const std::function<void(Entity, Ref<MeshSource>)>& func) { m_MeshAssetConvertCallback = func; }
		void SetInvalidMetadataCallback(const std::function<void(Entity, AssetHandle)>& func) { m_InvalidMetadataCallback = func; }

		void AddEntityContextMenuPlugin(const std::function<void(Entity)>& func) { m_EntityContextMenuPlugins.push_back(func); }

		virtual void OnImGuiRender(bool& isOpen) override;
		virtual void OnEvent(Event& event) override;

		static SelectionContext GetActiveSelectionContext() { return s_ActiveSelectionContext; }

	public:
		template<typename TVectorType, typename TComponent, typename GetOtherFunc>
		uint32_t GetInconsistentVectorAxis(GetOtherFunc func)
		{
			uint32_t axes = 0;

			const auto& entities = SelectionManager::GetSelections(m_SelectionContext);

			if (entities.size() < 2)
				return 0;

			Entity firstEntity = m_Context->GetEntityWithUUID(entities[0]);
			const TVectorType& first = func(firstEntity.GetComponent<TComponent>());

			for (size_t i = 1; i < entities.size(); i++)
			{
				Entity entity = m_Context->GetEntityWithUUID(entities[i]);
				const TVectorType& otherVector = func(entity.GetComponent<TComponent>());

				if (glm::epsilonNotEqual(otherVector.x, first.x, FLT_EPSILON))
					axes |= (uint32_t)VectorAxis::X;

				if (glm::epsilonNotEqual(otherVector.y, first.y, FLT_EPSILON))
					axes |= (uint32_t)VectorAxis::Y;

				if constexpr (std::is_same_v<TVectorType, glm::vec3> || std::is_same_v<TVectorType, glm::vec4>)
				{
					if (glm::epsilonNotEqual(otherVector.z, first.z, FLT_EPSILON))
						axes |= (uint32_t)VectorAxis::Z;
				}

				if constexpr (std::is_same_v<TVectorType, glm::vec4>)
				{
					if (glm::epsilonNotEqual(otherVector.w, first.w, FLT_EPSILON))
						axes |= (uint32_t)VectorAxis::W;
				}
			}

			return axes;
		}

		template<typename TVectorType>
		uint32_t GetInconsistentVectorAxis(const TVectorType& first, const TVectorType& other)
		{
			uint32_t axes = 0;

			if (glm::epsilonNotEqual(other.x, first.x, FLT_EPSILON))
				axes |= (uint32_t)VectorAxis::X;

			if (glm::epsilonNotEqual(other.y, first.y, FLT_EPSILON))
				axes |= (uint32_t)VectorAxis::Y;

			if constexpr (std::is_same_v<TVectorType, glm::vec3> || std::is_same_v<TVectorType, glm::vec4>)
			{
				if (glm::epsilonNotEqual(other.z, first.z, FLT_EPSILON))
					axes |= (uint32_t)VectorAxis::Z;
			}

			if constexpr (std::is_same_v<TVectorType, glm::vec4>)
			{
				if (glm::epsilonNotEqual(other.w, first.w, FLT_EPSILON))
					axes |= (uint32_t)VectorAxis::W;
			}

			return axes;
		}

		template<typename TRefType, typename TComponent, typename GetOtherFunc>
		bool IsInconsistentRef(GetOtherFunc func)
		{
			const auto& entities = SelectionManager::GetSelections(m_SelectionContext);

			if (entities.size() < 2)
				return false;

			Entity firstEntity = m_Context->GetEntityWithUUID(entities[0]);
			const Ref<TRefType>& first = func(firstEntity.GetComponent<TComponent>());

			for (size_t i = 1; i < entities.size(); i++)
			{
				Entity entity = m_Context->GetEntityWithUUID(entities[i]);
				const Ref<TRefType>& otherValue = func(entity.GetComponent<TComponent>());

				if (otherValue != first)
					return true;
			}

			return false;
		}

		template<typename TPrimitive, typename TComponent, typename GetOtherFunc>
		bool IsInconsistentPrimitive(GetOtherFunc func)
		{
			const auto& entities = SelectionManager::GetSelections(m_SelectionContext);

			if (entities.size() < 2)
				return false;

			Entity firstEntity = m_Context->GetEntityWithUUID(entities[0]);
			const TPrimitive& first = func(firstEntity.GetComponent<TComponent>());

			for (size_t i = 1; i < entities.size(); i++)
			{
				Entity entity = m_Context->GetEntityWithUUID(entities[i]);

				if (!entity.HasComponent<TComponent>())
					continue;

				const auto& otherValue = func(entity.GetComponent<TComponent>());
				if (otherValue != first)
					return true;
			}

			return false;
		}

		template<typename TComponent, typename GetOtherFunc>
		bool IsInconsistentString(GetOtherFunc func)
		{
			return IsInconsistentPrimitive<std::string, TComponent, GetOtherFunc>(func);
		}

		template<typename TComponent, typename UIFunction>
		void DrawComponent(const std::string& name, UIFunction uiFunction, Ref<Texture2D> icon = nullptr)
		{
			bool shouldDraw = true;

			auto& entities = SelectionManager::GetSelections(s_ActiveSelectionContext);
			for (const auto& entityID : entities)
			{
				Entity entity = m_Context->GetEntityWithUUID(entityID);
				if (!entity.HasComponent<TComponent>())
				{
					shouldDraw = false;
					break;
				}
			}

			if (!shouldDraw || entities.size() == 0)
				return;

			if (icon == nullptr)
				icon = EditorResources::AssetIcon;

			// NOTE(Peter):
			//	This fixes an issue where the first "+" button would display the "Remove" buttons for ALL components on an Entity.
			//	This is due to ImGui::TreeNodeEx only pushing the id for it's children if it's actually open
			ImGui::PushID((void*)typeid(TComponent).hash_code());
			ImVec2 contentRegionAvailable = ImGui::GetContentRegionAvail();

			bool open = UI::TreeNodeWithIcon(name, icon, { 14.0f, 14.0f });
			bool right_clicked = ImGui::IsItemClicked(ImGuiMouseButton_Right);
			float lineHeight = ImGui::GetItemRectMax().y - ImGui::GetItemRectMin().y;
			float itemPadding = 4.0f;

			bool resetValues = false;
			bool removeComponent = false;

			ImGui::SameLine(contentRegionAvailable.x - lineHeight - 5.0f);
			UI::ShiftCursorY(lineHeight / 4.0f);
			if (ImGui::InvisibleButton("##options", ImVec2{ lineHeight, lineHeight }) || right_clicked)
			{
				ImGui::OpenPopup("ComponentSettings");
			}
			UI::DrawButtonImage(EditorResources::GearIcon, COLOR32(160, 160, 160, 200),
								COLOR32(160, 160, 160, 255),
								COLOR32(160, 160, 160, 150),
								UI::RectExpanded(UI::GetItemRect(), 0.0f, 0.0f));

			if (UI::BeginPopup("ComponentSettings"))
			{
				UI::ShiftCursorX(itemPadding);

				Entity entity = m_Context->GetEntityWithUUID(entities[0]);
				auto& component = entity.GetComponent<TComponent>();

				if (ImGui::MenuItem("Copy"))
					Scene::CopyComponentFromScene<TComponent>(m_ComponentCopyEntity, m_ComponentCopyScene, entity, m_Context);

				UI::ShiftCursorX(itemPadding);
				if (ImGui::MenuItem("Paste"))
				{
					for (auto entityID : SelectionManager::GetSelections(s_ActiveSelectionContext))
					{
						Entity selectedEntity = m_Context->GetEntityWithUUID(entityID);
						Scene::CopyComponentFromScene<TComponent>(selectedEntity, m_Context, m_ComponentCopyEntity, m_ComponentCopyScene);
					}
				}

				UI::ShiftCursorX(itemPadding);
				if (ImGui::MenuItem("Reset"))
					resetValues = true;

				UI::ShiftCursorX(itemPadding);
				if constexpr (!std::is_same<TComponent, TransformComponent>::value)
				{
					if (ImGui::MenuItem("Remove component"))
						removeComponent = true;
				}

				UI::EndPopup();
			}

			if (open)
			{
				Entity entity = m_Context->GetEntityWithUUID(entities[0]);
				TComponent& firstComponent = entity.GetComponent<TComponent>();
				const bool isMultiEdit = entities.size() > 1;
				uiFunction(firstComponent, entities, isMultiEdit);
				ImGui::TreePop();
			}

			if (removeComponent)
			{
				for (auto& entityID : entities)
				{
					Entity entity = m_Context->GetEntityWithUUID(entityID);
					if (entity.HasComponent<TComponent>())
						entity.RemoveComponent<TComponent>();
				}
			}

			if (resetValues)
			{
				for (auto& entityID : entities)
				{
					Entity entity = m_Context->GetEntityWithUUID(entityID);
					if (entity.HasComponent<TComponent>())
					{
						entity.RemoveComponent<TComponent>();
						entity.AddComponent<TComponent>();
					}
				}
			}

			if (!open)
				UI::ShiftCursorY(-(ImGui::GetStyle().ItemSpacing.y + 1.0f));

			ImGui::PopID();
		}

	private:

		bool IsMeshSet(AssetHandle& outHandle);
		bool IsStaticMeshSet(AssetHandle& outHandle);

		void DrawEntityCreateMenu(Entity parent = {});
		void DrawEntityNode(Entity entity, const std::string& searchFilter = {});
		void DrawComponents(const std::vector<UUID>& entities);

		template<typename T>
		void EditGraphInput(Identifier id, choc::value::ValueView& value, const std::vector<UUID>& entities)
		{
			auto t = value.get<T>();
			if (UI::Property(AnimationGraph::GetName(id).c_str(), t))
			{
				for (auto entityID : entities)
				{
					Entity entity = m_Context->GetEntityWithUUID(entityID);
					auto& anim = entity.GetComponent<AnimationComponent>();
					if (anim.AnimationGraph)
					{
						auto instanceValue = anim.AnimationGraph->InValue(id);
						instanceValue.set(t);
					}
				}
			}
		}


		void OnExternalEntityDestroyed(Entity entity);

		bool TagSearchRecursive(Entity entity, std::string_view searchFilter, uint32_t maxSearchDepth, uint32_t currentDepth = 1);

	private:
		Ref<Scene> m_Context;
		bool m_IsWindow;
		bool m_IsWindowFocused = false;
		bool m_IsHierarchyOrPropertiesFocused = false;
		bool m_ShiftSelectionRunning = false;

		std::function<void(Entity)> m_EntityDeletedCallback;
		std::function<void(Entity, Ref<MeshSource>)> m_MeshAssetConvertCallback;
		std::function<void(Entity, AssetHandle)> m_InvalidMetadataCallback;
		std::vector<std::function<void(Entity)>> m_EntityContextMenuPlugins;

		SelectionContext m_SelectionContext;

		int32_t m_FirstSelectedRow = -1;
		int32_t m_LastSelectedRow = -1;

		Ref<Scene> m_ComponentCopyScene;
		Entity m_ComponentCopyEntity;

		static SelectionContext s_ActiveSelectionContext;
	};

}
