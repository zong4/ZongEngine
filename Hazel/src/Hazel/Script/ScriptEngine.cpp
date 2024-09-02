#include "hzpch.h"
#include "ScriptEngine.h"
#include "ScriptGlue.h"
#include "ScriptAsset.h"

#include "Hazel/Core/Application.h"
#include "Hazel/Project/Project.h"
#include "Hazel/Asset/AssetManager.h"

#include <Coral/HostInstance.hpp>
#include <Coral/StringHelper.hpp>
#include <Coral/Attribute.hpp>
#include <Coral/TypeCache.hpp>

namespace Hazel {

	static std::unordered_map<std::string, DataType> s_DataTypeLookup = {
		{ "System.SByte", DataType::SByte },
		{ "System.Byte", DataType::Byte },
		{ "System.Int16", DataType::Short },
		{ "System.UInt16", DataType::UShort },
		{ "System.Int32", DataType::Int },
		{ "System.UInt32", DataType::UInt },
		{ "System.Int64", DataType::Long },
		{ "System.UInt64", DataType::ULong },
		{ "System.Single", DataType::Float },
		{ "System.Double", DataType::Double },
		{ "Hazel.Vector2", DataType::Vector2 },
		{ "Hazel.Vector3", DataType::Vector3 },
		{ "Hazel.Vector4", DataType::Vector4 },
		{ "System.Boolean", DataType::Bool },
		{ "Hazel.Entity", DataType::Entity },
		{ "Hazel.Prefab", DataType::Prefab },
		{ "Hazel.Mesh", DataType::Mesh },
		{ "Hazel.StaticMesh", DataType::StaticMesh },
		{ "Hazel.Material", DataType::Material },
		{ "Hazel.Texture2D", DataType::Texture2D },
		{ "Hazel.Scene", DataType::Scene },
	};

	void OnCSharpException(std::string_view message)
	{
		HZ_CONSOLE_LOG_ERROR("C# Exception: {}", message);
	}

	void ScriptEngine::LoadProjectAssembly()
	{
		m_AppAssemblyData.reset();

		auto filepath = std::filesystem::absolute(Project::GetScriptModuleFilePath());

		m_AppAssemblyData = CreateScope<AssemblyData>();
		m_AppAssemblyData->Assembly = &m_LoadContext->LoadAssembly(filepath.string());
		
		if (m_AppAssemblyData->Assembly->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
		{
			return;
		}

		BuildAssemblyCache(m_AppAssemblyData.get());
	}

	void ScriptEngine::LoadProjectAssemblyRuntime(Buffer data)
	{
		m_AppAssemblyData.reset();

		m_AppAssemblyData = CreateScope<AssemblyData>();
		m_AppAssemblyData->Assembly = &m_LoadContext->LoadAssemblyFromMemory(reinterpret_cast<const std::byte*>(data.Data), data.Size);

		if (m_AppAssemblyData->Assembly->GetLoadStatus() != Coral::AssemblyLoadStatus::Success)
		{
			return;
		}

		BuildAssemblyCache(m_AppAssemblyData.get());

	}

	bool ScriptEngine::IsValidScript(UUID scriptID) const
	{
		if (!m_AppAssemblyData)
			return false;

		return m_AppAssemblyData->CachedTypes.contains(scriptID) && m_ScriptMetadata.contains(scriptID);
	}

	void OnCoralMessage(std::string_view message, Coral::MessageLevel level)
	{
		switch (level)
		{
			case Coral::MessageLevel::Info:
				HZ_CORE_INFO_TAG("Scripting", "{}", std::string(message));
				break;
			case Coral::MessageLevel::Warning:
				HZ_CORE_WARN_TAG("Scripting", "{}", std::string(message));
				break;
			case Coral::MessageLevel::Error:
				HZ_CORE_ERROR_TAG("Scripting", "{}", std::string(message));
				break;
		}
	}

	const Hazel::ScriptMetadata& ScriptEngine::GetScriptMetadata(UUID scriptID) const
	{
		HZ_CORE_VERIFY(m_ScriptMetadata.contains(scriptID));
		return m_ScriptMetadata.at(scriptID);
	}

	const Coral::Type* ScriptEngine::GetTypeByName(std::string_view name) const
	{
		for (const auto&[id, type] : m_CoreAssemblyData->CachedTypes)
		{
			Coral::String typeName = type->GetFullName();

			if (typeName == name)
			{
				Coral::String::Free(typeName);
				return type;
			}

			Coral::String::Free(typeName);
		}

		for (const auto& [id, type] : m_AppAssemblyData->CachedTypes)
		{
			Coral::String typeName = type->GetFullName();

			if (typeName == name)
			{
				Coral::String::Free(typeName);
				return type;
			}

			Coral::String::Free(typeName);
		}

		return nullptr;
	}

	const ScriptEngine& ScriptEngine::GetInstance() { return GetMutable(); }

	void ScriptEngine::InitializeHost()
	{
		m_Host = std::make_unique<Coral::HostInstance>();

		Coral::HostSettings settings =
		{
			.CoralDirectory = (std::filesystem::current_path() / "DotNet").string(),
			.MessageCallback = OnCoralMessage,
			.ExceptionCallback = OnCSharpException
		};

		Coral::CoralInitStatus initStatus = m_Host->Initialize(settings);

		if (initStatus == Coral::CoralInitStatus::Success)
		{
			return;
		}

		switch (initStatus)
		{
			case Coral::CoralInitStatus::CoralManagedNotFound:
			{
				auto message = std::format("Could not find Coral.Managed.dll in directory {}", settings.CoralDirectory);

#if defined(HZ_PLATFORM_WINDOWS)
				int response = MessageBoxA(nullptr,
					message.c_str(),
					"Hazel C# Scripting Engine Initialization Failure", MB_OK | MB_ICONERROR
				);
#else
				HZ_CORE_ERROR("Hazel C# Scripting Engine Initialization Failure: {}", message);
#endif
				break;
			}
			case Coral::CoralInitStatus::CoralManagedInitError:
			{
#if defined(HZ_PLATFORM_WINDOWS)
				int response = MessageBoxA(nullptr,
					"Failed to initialize Coral.Managed",
					"Hazel C# Scripting Engine Initialization Failure", MB_OK | MB_ICONERROR
				);
#else
				HZ_CORE_ERROR("Hazel C# Scripting Engine Initialization Failure: Failed to initialize Coral.Managed");
#endif
				break;
			}
			case Coral::CoralInitStatus::DotNetNotFound:
			{
#if defined(HZ_PLATFORM_WINDOWS)
				int response = MessageBoxA(nullptr,
					"Hazel requires .NET 8 or higher!\n\n"
					"Please make sure you have the appropriate .NET Runtime installed. Installers for all platforms can be found here: https://dotnet.microsoft.com/en-us/download/dotnet\n\n"
					"Would you like to download the latest .NET 8 Runtime installer?",
					"Hazel C# Scripting Engine Initialization Failure",
					MB_YESNO | MB_ICONERROR
				);

				if (response == IDYES)
				{
					system("start https://aka.ms/dotnet/8.0/windowsdesktop-runtime-win-x64.exe");
				}
#else
				HZ_CORE_ERROR(
					"Hazel requires .NET 8 or higher!\n\n"
					"Please make sure you have the appropriate .NET Runtime installed. Installers for all platforms can be found here: https://dotnet.microsoft.com/en-us/download/dotnet\n\n"				"You can download the .NET installer here: https://dotnet.microsoft.com/en-us/download/dotnet."
				);

				system("open https://dotnet.microsoft.com/en-us/download/dotnet");
#endif
				break;
			}
			default:
				break;
		}

		// All of the above errors are fatal
		std::exit(-1);
	}

	void ScriptEngine::ShutdownHost()
	{
		Coral::TypeCache::Get().Clear();

		m_Host->Shutdown();
		m_Host.reset();
	}

	void ScriptEngine::Initialize(Ref<Project> project)
	{
		m_LoadContext = std::make_unique<Coral::AssemblyLoadContext>(std::move(m_Host->CreateAssemblyLoadContext("HazelLoadContext")));

		auto scriptCorePath = (std::filesystem::current_path() / "Resources" / "Scripts" / "Hazel-ScriptCore.dll").string();
		m_CoreAssemblyData = CreateScope<AssemblyData>();

		m_CoreAssemblyData->Assembly = &m_LoadContext->LoadAssembly(scriptCorePath);
		BuildAssemblyCache(m_CoreAssemblyData.get());

		ScriptGlue::RegisterGlue(*m_CoreAssemblyData->Assembly);
	}

	void ScriptEngine::Shutdown()
	{
		for (auto& [scriptID, scriptMetadata] : m_ScriptMetadata)
		{
			for (auto& [fieldID, fieldMetadata] : scriptMetadata.Fields)
			{
				fieldMetadata.DefaultValue.Release();
			}
		}
		m_ScriptMetadata.clear();

		m_AppAssemblyData.reset();
		m_CoreAssemblyData.reset();
		m_Host->UnloadAssemblyLoadContext(*m_LoadContext);

		Coral::TypeCache::Get().Clear();
	}	

	void ScriptEngine::BuildAssemblyCache(AssemblyData* assemblyData)
	{
		auto& types = assemblyData->Assembly->GetTypes();
		auto& entityType = assemblyData->Assembly->GetType("Hazel.Entity");

		for (auto& type : types)
		{
			std::string fullName = type->GetFullName();
			auto scriptID = Hash::GenerateFNVHash(fullName);

			assemblyData->CachedTypes[scriptID] = type;

			// TODO(Peter): Talk to Jay about moving AudioEntity into a separate project
			//				that provides out of the box gameplay functionality
			if (type->IsSubclassOf(entityType))
			{
				auto& metadata = m_ScriptMetadata[scriptID];
				metadata.FullName = fullName;

				auto temp = type->CreateInstance();

				for (auto& fieldInfo : type->GetFields())
				{
					Coral::ScopedString fieldName = fieldInfo.GetName();
					std::string fieldNameStr = fieldName;

					if (fieldNameStr.find("k__BackingField") != std::string::npos)
						continue;

					auto* fieldType = &fieldInfo.GetType();

					if (fieldType->IsSZArray())
					{
						fieldType = &fieldType->GetElementType();
					}

					Coral::ScopedString typeName = fieldType->GetFullName();

					if (!s_DataTypeLookup.contains(typeName))
						continue;
					
					if (fieldInfo.GetAccessibility() != Coral::TypeAccessibility::Public)
					{
						// TODO(Peter): fieldInfo.HasAttribute()
						auto attributes = fieldInfo.GetAttributes();

						auto found = std::ranges::find_if(attributes, [](Coral::Attribute& attribute)
						{
							Coral::ScopedString name = attribute.GetType().GetFullName();
							return name == "Hazel.ShowInEditorAttribute";
						});

						// Only ignore this field if it's not public AND it doesn't have the ShowInEditor attribute attached to it
						if (found == attributes.end())
						{
							continue;
						}
					}

					// NOTE(Peter): Entity.ID bleeds through to the inheriting scripts, annoying, but we can deal
					if (fieldNameStr == "ID")
						continue;

					auto fullFieldName = std::format("{}.{}", fullName, fieldNameStr);
					uint32_t fieldID = Hash::GenerateFNVHash(fullFieldName);

					auto& fieldMetadata = metadata.Fields[fieldID];
					fieldMetadata.Name = fieldName;
					fieldMetadata.Type = s_DataTypeLookup.at(typeName);
					fieldMetadata.ManagedType = &fieldInfo.GetType();

					switch (fieldMetadata.Type)
					{
						case DataType::SByte:
							fieldMetadata.SetDefaultValue<int8_t>(temp);
							break;
						case DataType::Byte:
							fieldMetadata.SetDefaultValue<uint8_t>(temp);
							break;
						case DataType::Short:
							fieldMetadata.SetDefaultValue<int16_t>(temp);
							break;
						case DataType::UShort:
							fieldMetadata.SetDefaultValue<uint16_t>(temp);
							break;
						case DataType::Int:
							fieldMetadata.SetDefaultValue<int32_t>(temp);
							break;
						case DataType::UInt:
							fieldMetadata.SetDefaultValue<uint32_t>(temp);
							break;
						case DataType::Long:
							fieldMetadata.SetDefaultValue<int64_t>(temp);
							break;
						case DataType::ULong:
							fieldMetadata.SetDefaultValue<uint64_t>(temp);
							break;
						case DataType::Float:
							fieldMetadata.SetDefaultValue<float>(temp);
							break;
						case DataType::Double:
							fieldMetadata.SetDefaultValue<double>(temp);
							break;
						case DataType::Vector2:
							fieldMetadata.SetDefaultValue<glm::vec2>(temp);
							break;
						case DataType::Vector3:
							fieldMetadata.SetDefaultValue<glm::vec3>(temp);
							break;
						case DataType::Vector4:
							fieldMetadata.SetDefaultValue<glm::vec4>(temp);
							break;
						case DataType::Bool:
							break;
						case DataType::Entity:
						case DataType::Prefab:
						case DataType::Mesh:
						case DataType::StaticMesh:
						case DataType::Material:
						case DataType::Texture2D:
						case DataType::Scene:
							break;
						default:
							break;
					}
				}

				temp.Destroy();
			}
		}
	}

	ScriptEngine& ScriptEngine::GetMutable()
	{
		static ScriptEngine s_Instance;
		return s_Instance;
	}

}
