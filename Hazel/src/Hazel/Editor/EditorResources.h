#pragma once

#include "Hazel/Renderer/Texture.h"
#include "Hazel/Utilities/FileSystem.h"

namespace Hazel {

	class EditorResources
	{
	public:
		// Generic
		inline static Ref<Texture2D> GearIcon = nullptr;
		inline static Ref<Texture2D> PlusIcon = nullptr;
		inline static Ref<Texture2D> PencilIcon = nullptr;
		inline static Ref<Texture2D> ForwardIcon = nullptr;
		inline static Ref<Texture2D> BackIcon = nullptr;
		inline static Ref<Texture2D> PointerIcon = nullptr;
		inline static Ref<Texture2D> SearchIcon = nullptr;
		inline static Ref<Texture2D> ClearIcon = nullptr;
		inline static Ref<Texture2D> SaveIcon = nullptr;
		inline static Ref<Texture2D> ReticuleIcon = nullptr;
		inline static Ref<Texture2D> LinkedIcon = nullptr;
		inline static Ref<Texture2D> UnlinkedIcon = nullptr;

		// Icons
		inline static Ref<Texture2D> AnimationIcon = nullptr;
		inline static Ref<Texture2D> AssetIcon = nullptr;
		inline static Ref<Texture2D> AudioIcon = nullptr;
		inline static Ref<Texture2D> AudioListenerIcon = nullptr;
		inline static Ref<Texture2D> BoxColliderIcon = nullptr;
		inline static Ref<Texture2D> BoxCollider2DIcon = nullptr;
		inline static Ref<Texture2D> CameraIcon = nullptr;
		inline static Ref<Texture2D> CapsuleColliderIcon = nullptr;
		inline static Ref<Texture2D> CharacterControllerIcon = nullptr;
		inline static Ref<Texture2D> CircleCollider2DIcon = nullptr;
		inline static Ref<Texture2D> DirectionalLightIcon = nullptr;
		inline static Ref<Texture2D> FixedJointIcon = nullptr;
		inline static Ref<Texture2D> MeshIcon = nullptr;
		inline static Ref<Texture2D> MeshColliderIcon = nullptr;
		inline static Ref<Texture2D> PointLightIcon = nullptr;
		inline static Ref<Texture2D> RigidBodyIcon = nullptr;
		inline static Ref<Texture2D> RigidBody2DIcon = nullptr;
		inline static Ref<Texture2D> ScriptIcon = nullptr;
		inline static Ref<Texture2D> SkeletonIcon = nullptr;
		inline static Ref<Texture2D> SpriteIcon = nullptr;
		inline static Ref<Texture2D> SkyLightIcon = nullptr;
		inline static Ref<Texture2D> CompoundColliderIcon = nullptr;
		inline static Ref<Texture2D> SphereColliderIcon = nullptr;
		inline static Ref<Texture2D> SpotLightIcon = nullptr;
		inline static Ref<Texture2D> StaticMeshIcon = nullptr;
		inline static Ref<Texture2D> TextIcon = nullptr;
		inline static Ref<Texture2D> TransformIcon = nullptr;
		inline static Ref<Texture2D> RefreshIcon = nullptr;

		// Viewport
		inline static Ref<Texture2D> PlayIcon = nullptr;
		inline static Ref<Texture2D> PauseIcon = nullptr;
		inline static Ref<Texture2D> StopIcon = nullptr;
		inline static Ref<Texture2D> SimulateIcon = nullptr;
		inline static Ref<Texture2D> MoveIcon = nullptr;
		inline static Ref<Texture2D> RotateIcon = nullptr;
		inline static Ref<Texture2D> ScaleIcon = nullptr;

		// Window
		inline static Ref<Texture2D> MinimizeIcon = nullptr;
		inline static Ref<Texture2D> MaximizeIcon = nullptr;
		inline static Ref<Texture2D> RestoreIcon = nullptr;
		inline static Ref<Texture2D> CloseIcon = nullptr;

		// Content Browser
		inline static Ref<Texture2D> FolderIcon = nullptr;
		inline static Ref<Texture2D> FileIcon = nullptr;
		inline static Ref<Texture2D> FBXFileIcon = nullptr;
		inline static Ref<Texture2D> OBJFileIcon = nullptr;
		inline static Ref<Texture2D> GLTFFileIcon = nullptr;
		inline static Ref<Texture2D> GLBFileIcon = nullptr;
		inline static Ref<Texture2D> WAVFileIcon = nullptr;
		inline static Ref<Texture2D> MP3FileIcon = nullptr;
		inline static Ref<Texture2D> OGGFileIcon = nullptr;
		inline static Ref<Texture2D> CSFileIcon = nullptr;
		inline static Ref<Texture2D> PNGFileIcon = nullptr;
		inline static Ref<Texture2D> JPGFileIcon = nullptr;
		inline static Ref<Texture2D> MaterialFileIcon = nullptr;
		inline static Ref<Texture2D> SceneFileIcon = nullptr;
		inline static Ref<Texture2D> PrefabFileIcon = nullptr;
		inline static Ref<Texture2D> FontFileIcon = nullptr;
		inline static Ref<Texture2D> AnimationFileIcon = nullptr;
		inline static Ref<Texture2D> AnimationGraphFileIcon = nullptr;
		inline static Ref<Texture2D> MeshFileIcon = nullptr;
		inline static Ref<Texture2D> StaticMeshFileIcon = nullptr;
		inline static Ref<Texture2D> MeshColliderFileIcon = nullptr;
		inline static Ref<Texture2D> PhysicsMaterialFileIcon = nullptr;
		inline static Ref<Texture2D> SkeletonFileIcon = nullptr;
		inline static Ref<Texture2D> SoundConfigFileIcon = nullptr;
		inline static Ref<Texture2D> SoundGraphFileIcon = nullptr;

		// Node Graph Editor
		inline static Ref<Texture2D> CompileIcon = nullptr;
		inline static Ref<Texture2D> PinValueConnectIcon = nullptr;
		inline static Ref<Texture2D> PinValueDisconnectIcon = nullptr;
		inline static Ref<Texture2D> PinFlowConnectIcon = nullptr;
		inline static Ref<Texture2D> PinFlowDisconnectIcon = nullptr;
		inline static Ref<Texture2D> PinAudioConnectIcon = nullptr;
		inline static Ref<Texture2D> PinAudioDisconnectIcon = nullptr;

		// Textures
		inline static Ref<Texture2D> HazelLogoTexture = nullptr;
		inline static Ref<Texture2D> CheckerboardTexture = nullptr;
		inline static Ref<Texture2D> ShadowTexture = nullptr;
		inline static Ref<Texture2D> TranslucentTexture = nullptr;
		inline static Ref<Texture2D> TranslucentGradientTexture = nullptr;

		static void Init()
		{
			TextureSpecification spec;
			spec.SamplerWrap = TextureWrap::Clamp;

			// Generic (dont forget to .Reset() these in EditorResources::Shutdown())
			GearIcon = LoadTexture("Generic/Gear.png", "GearIcon", spec);
			PlusIcon = LoadTexture("Generic/Plus.png", "PlusIcon", spec);
			PencilIcon = LoadTexture("Generic/Pencil.png", "PencilIcon", spec);
			ForwardIcon = LoadTexture("Generic/Forward.png", "ForwardIcon", spec);
			BackIcon = LoadTexture("Generic/Back.png", "BackIcon", spec);
			PointerIcon = LoadTexture("Generic/Pointer.png", "PointerIcon", spec);
			SearchIcon = LoadTexture("Generic/Search.png", "SearchIcon", spec);
			ClearIcon = LoadTexture("Generic/Clear.png", "ClearIcon", spec);
			SaveIcon = LoadTexture("Generic/Save.png", "ClearIcon", spec);
			ReticuleIcon = LoadTexture("Generic/Reticule.png", "Reticule", spec);
			LinkedIcon = LoadTexture("Generic/Linked.png", "LinkedIcon", spec);
			UnlinkedIcon = LoadTexture("Generic/Unlinked.png", "UnlinkedIcon", spec);

			// Icons (dont forget to .Reset() these in EditorResources::Shutdown())
			AnimationIcon = LoadTexture("Icons/Animation.png", "AnimationIcon", spec);
			AssetIcon = LoadTexture("Icons/Generic.png", "GenericIcon", spec);
			AudioIcon = LoadTexture("Icons/Audio.png", "AudioIcon", spec);
			AudioListenerIcon = LoadTexture("Icons/AudioListener.png", "AudioListenerIcon", spec);
			BoxColliderIcon = LoadTexture("Icons/BoxCollider.png", "BoxColliderIcon", spec);
			BoxCollider2DIcon = LoadTexture("Icons/BoxCollider2D.png", "BoxCollider2DIcon", spec);
			CameraIcon = LoadTexture("Icons/Camera.png", "CameraIcon", spec);
			CapsuleColliderIcon = LoadTexture("Icons/CapsuleCollider.png", "CapsuleColliderIcon", spec);
			CharacterControllerIcon = LoadTexture("Icons/CharacterController.png", "CharacterControllerIcon", spec);
			CircleCollider2DIcon = LoadTexture("Icons/CircleCollider2D.png", "CircleCollider2DIcon", spec);
			DirectionalLightIcon = LoadTexture("Icons/DirectionalLight.png", "DirectionalLightIcon", spec);
			FixedJointIcon = LoadTexture("Icons/FixedJoint.png", "FixedJointIcon", spec);
			MeshIcon = LoadTexture("Icons/Mesh.png", "MeshIcon", spec);
			MeshColliderIcon = LoadTexture("Icons/MeshCollider.png", "MeshColliderIcon", spec);
			PointLightIcon = LoadTexture("Icons/PointLight.png", "PointLightIcon", spec);
			RigidBodyIcon = LoadTexture("Icons/RigidBody.png", "RigidBodyIcon", spec);
			RigidBody2DIcon = LoadTexture("Icons/RigidBody2D.png", "RigidBody2DIcon", spec);
			ScriptIcon = LoadTexture("Icons/Script.png", "ScriptIcon", spec);
			SkeletonIcon = LoadTexture("Icons/Skeleton.png", "SkeletonIcon", spec);
			SpriteIcon = LoadTexture("Icons/SpriteRenderer.png", "SpriteIcon", spec);
			SkyLightIcon = LoadTexture("Icons/SkyLight.png", "SkyLightIcon", spec);
			CompoundColliderIcon = LoadTexture("Icons/CompoundCollider.png", "CompoundColliderIcon", spec);
			SphereColliderIcon = LoadTexture("Icons/SphereCollider.png", "SphereColliderIcon", spec);
			SpotLightIcon = LoadTexture("Icons/SpotLight.png", "SpotLightIcon", spec);
			StaticMeshIcon = LoadTexture("Icons/StaticMesh.png", "StaticMeshIcon", spec);
			TextIcon = LoadTexture("Icons/Text.png", "TextIcon", spec);
			TransformIcon = LoadTexture("Icons/Transform.png", "TransformIcon", spec);
			RefreshIcon = LoadTexture("Viewport/RotateTool.png", "RefreshIcon", spec);

			// Viewport (dont forget to .Reset() these in EditorResources::Shutdown())
			PlayIcon = LoadTexture("Viewport/Play.png", "PlayIcon", spec);
			PauseIcon = LoadTexture("Viewport/Pause.png", "PauseIcon", spec);
			StopIcon = LoadTexture("Viewport/Stop.png", "StopIcon", spec);
			SimulateIcon = LoadTexture("Viewport/Simulate.png", "SimulateIcon", spec);
			MoveIcon = LoadTexture("Viewport/MoveTool.png", "MoveIcon", spec);
			RotateIcon = LoadTexture("Viewport/RotateTool.png", "RotateIcon", spec);
			ScaleIcon = LoadTexture("Viewport/ScaleTool.png", "ScaleIcon", spec);

			// Window (dont forget to .Reset() these in EditorResources::Shutdown())
			MinimizeIcon = LoadTexture("Window/Minimize.png", "MinimizeIcon", spec);
			MaximizeIcon = LoadTexture("Window/Maximize.png", "MaximizeIcon", spec);
			RestoreIcon = LoadTexture("Window/Restore.png", "RestoreIcon", spec);
			CloseIcon = LoadTexture("Window/Close.png", "CloseIcon", spec);

			// Content Browser (dont forget to .Reset() these in EditorResources::Shutdown())
			FolderIcon = LoadTexture("ContentBrowser/Folder.png", "FolderIcon", spec);
			FileIcon = LoadTexture("ContentBrowser/File.png", "FileIcon", spec);
			FBXFileIcon = LoadTexture("ContentBrowser/FBX.png", "FBXFileIcon", spec);
			OBJFileIcon = LoadTexture("ContentBrowser/OBJ.png", "OBJFileIcon", spec);
			GLTFFileIcon = LoadTexture("ContentBrowser/GLTF.png", "GLTFFileIcon", spec);
			GLBFileIcon = LoadTexture("ContentBrowser/GLB.png", "GLBFileIcon", spec);
			WAVFileIcon = LoadTexture("ContentBrowser/WAV.png", "WAVFileIcon", spec);
			MP3FileIcon = LoadTexture("ContentBrowser/MP3.png", "MP3FileIcon", spec);
			OGGFileIcon = LoadTexture("ContentBrowser/OGG.png", "OGGFileIcon", spec);
			CSFileIcon = LoadTexture("ContentBrowser/CS.png", "CSFileIcon", spec);
			PNGFileIcon = LoadTexture("ContentBrowser/PNG.png", "PNGFileIcon", spec);
			JPGFileIcon = LoadTexture("ContentBrowser/JPG.png", "JPGFileIcon", spec);
			MaterialFileIcon = LoadTexture("ContentBrowser/Material.png", "MaterialFileIcon", spec);
			SceneFileIcon = LoadTexture("Hazel-IconLogo-2023.png", "SceneFileIcon", spec);
			PrefabFileIcon = LoadTexture("ContentBrowser/Prefab.png", "PrefabFileIcon", spec);
			FontFileIcon = LoadTexture("ContentBrowser/Font.png", "FontFileIcon", spec);
			AnimationFileIcon = LoadTexture("ContentBrowser/Animation.png", "AnimationFileIcon", spec);
			AnimationGraphFileIcon = LoadTexture("ContentBrowser/AnimationGraph.png", "AnimationGraphFileIcon", spec);
			MeshFileIcon = LoadTexture("ContentBrowser/Mesh.png", "MeshFileIcon", spec);
			StaticMeshFileIcon = LoadTexture("ContentBrowser/StaticMesh.png", "StaticMeshFileIcon", spec);
			MeshColliderFileIcon = LoadTexture("ContentBrowser/MeshCollider.png", "MeshColliderFileIcon", spec);
			PhysicsMaterialFileIcon = LoadTexture("ContentBrowser/PhysicsMaterial.png", "PhysicsMaterialFileIcon", spec);
			SkeletonFileIcon = LoadTexture("ContentBrowser/Skeleton.png", "SkeletonFileIcon", spec);
			SoundConfigFileIcon = LoadTexture("ContentBrowser/SoundConfig.png", "SoundConfigFileIcon", spec);
			SoundGraphFileIcon = LoadTexture("ContentBrowser/SoundGraph.png", "SoundGraphFileIcon", spec);

			// Node Graph (dont forget to .Reset() these in EditorResources::Shutdown())
			CompileIcon = LoadTexture("NodeGraph/Compile.png", "Compile", spec);
			PinValueConnectIcon = LoadTexture("NodeGraph/Pins/ValueConnect.png", "ValueConnect", spec);
			PinValueDisconnectIcon = LoadTexture("NodeGraph/Pins/ValueDisconnect.png", "ValueDisconnect", spec);
			PinFlowConnectIcon = LoadTexture("NodeGraph/Pins/FlowConnect.png", "FlowConnect", spec);
			PinFlowDisconnectIcon = LoadTexture("NodeGraph/Pins/FlowDisconnect.png", "FlowDisconnect", spec);
			PinAudioConnectIcon = LoadTexture("NodeGraph/Pins/AudioConnect.png", "AudioConnect", spec);
			PinAudioDisconnectIcon = LoadTexture("NodeGraph/Pins/AudioDisconnect.png", "AudioDisconnect", spec);

			// Textures (dont forget to .Reset() these in EditorResources::Shutdown())
			HazelLogoTexture = LoadTexture("HazelLogo_Light.png");
			CheckerboardTexture = LoadTexture("Checkerboard.tga");
			ShadowTexture = LoadTexture("Panels/Shadow.png", "ShadowTexture", spec);
			TranslucentTexture = LoadTexture("Panels/Translucent.png", "TranslucentTexture", spec);
			TranslucentGradientTexture = LoadTexture("Panels/TranslucentGradient.png", "TranslucentGradientTexture", spec);
		}

		static void Shutdown()
		{
			// Generic
			GearIcon.Reset();
			PlusIcon.Reset();
			PencilIcon.Reset();
			ForwardIcon.Reset();
			BackIcon.Reset();
			PointerIcon.Reset();
			SearchIcon.Reset();
			ClearIcon.Reset();
			SaveIcon.Reset();
			ReticuleIcon.Reset();
			LinkedIcon.Reset();
			UnlinkedIcon.Reset();

			// Icons
			AnimationIcon.Reset();
			AssetIcon.Reset();
			AudioIcon.Reset();
			AudioListenerIcon.Reset();
			BoxColliderIcon.Reset();
			BoxCollider2DIcon.Reset();
			CameraIcon.Reset();
			CapsuleColliderIcon.Reset();
			CharacterControllerIcon.Reset();
			CircleCollider2DIcon.Reset();
			DirectionalLightIcon.Reset();
			FixedJointIcon.Reset();
			MeshIcon.Reset();
			MeshColliderIcon.Reset();
			PointLightIcon.Reset();
			RigidBodyIcon.Reset();
			RigidBody2DIcon.Reset();
			ScriptIcon.Reset();
			SkeletonIcon.Reset();
			SpriteIcon.Reset();
			SkyLightIcon.Reset();
			CompoundColliderIcon.Reset();
			SphereColliderIcon.Reset();
			SpotLightIcon.Reset();
			StaticMeshIcon.Reset();
			TextIcon.Reset();
			TransformIcon.Reset();
			RefreshIcon.Reset();

			// Viewport
			PlayIcon.Reset();
			PauseIcon.Reset();
			StopIcon.Reset();
			SimulateIcon.Reset();
			MoveIcon.Reset();
			RotateIcon.Reset();
			ScaleIcon.Reset();

			// Window
			MinimizeIcon.Reset();
			MaximizeIcon.Reset();
			RestoreIcon.Reset();
			CloseIcon.Reset();

			// Content Browser
			FolderIcon.Reset();
			FileIcon.Reset();
			FBXFileIcon.Reset();
			OBJFileIcon.Reset();
			GLTFFileIcon.Reset();
			GLBFileIcon.Reset();
			WAVFileIcon.Reset();
			MP3FileIcon.Reset();
			OGGFileIcon.Reset();
			CSFileIcon.Reset();
			PNGFileIcon.Reset();
			JPGFileIcon.Reset();
			MaterialFileIcon.Reset();
			SceneFileIcon.Reset();
			PrefabFileIcon.Reset();
			FontFileIcon.Reset();
			AnimationFileIcon.Reset();
			AnimationGraphFileIcon.Reset();
			MeshFileIcon.Reset();
			StaticMeshFileIcon.Reset();
			MeshColliderFileIcon.Reset();
			PhysicsMaterialFileIcon.Reset();
			SkeletonFileIcon.Reset();
			SoundConfigFileIcon.Reset();
			SoundGraphFileIcon.Reset();

			// Node Graph
			CompileIcon.Reset();
			PinValueConnectIcon.Reset();
			PinValueDisconnectIcon.Reset();
			PinFlowConnectIcon.Reset();
			PinFlowDisconnectIcon.Reset();
			PinAudioConnectIcon.Reset();
			PinAudioDisconnectIcon.Reset();

			// Textures
			HazelLogoTexture.Reset();
			CheckerboardTexture.Reset();
			ShadowTexture.Reset();
			TranslucentTexture.Reset();
			TranslucentGradientTexture.Reset();
		}

	private:
		static Ref<Texture2D> LoadTexture(const std::filesystem::path& relativePath, TextureSpecification specification = TextureSpecification())
		{
			return LoadTexture(relativePath, "", specification);
		}

		static Ref<Texture2D> LoadTexture(const std::filesystem::path& relativePath, const std::string& name, TextureSpecification specification)
		{
			specification.DebugName = name;

			std::filesystem::path path = std::filesystem::path("Resources") / "Editor" / relativePath;
			
			if (!FileSystem::Exists(path))
			{
				HZ_CORE_FATAL("Failed to load icon {0}! The file doesn't exist.", path.string());
				HZ_CORE_VERIFY(false);
				return nullptr;
			}

			return Texture2D::Create(specification, path);
		}
	};

}
