#pragma once

#include "Hazel/Core/UUID.h"
#include "Hazel/Asset/AssetTypes.h"

namespace Hazel {

	using AssetHandle = UUID;

	class Asset : public RefCounted
	{
	public:
		AssetHandle Handle = 0;
		uint16_t Flags = (uint16_t)AssetFlag::None;

		virtual ~Asset() {}

		static AssetType GetStaticType() { return AssetType::None; }
		virtual AssetType GetAssetType() const { return AssetType::None; }

		virtual void OnDependencyUpdated(AssetHandle handle) {}

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}
		
		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

	private:
		// If you want to find out whether assets are valid or missing, use AssetManager::IsAssetValid(handle), IsAssetMissing(handle)
		// This cleans up and removes inconsistencies from rest of the code.
		// You simply go AssetManager::GetAsset<Whatever>(handle), and so long as you get a non-null pointer back, you're good to go.
		// No IsValid(), IsFlagSet(AssetFlag::Missing) etc. etc. all throughout the code.
		friend class EditorAssetManager;
		friend class RuntimeAssetManager;
		friend class AssimpMeshImporter;
		friend class TextureSerializer;

		bool IsValid() const { return ((Flags & (uint16_t)AssetFlag::Missing) | (Flags & (uint16_t)AssetFlag::Invalid)) == 0; }

		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlag flag, bool value = true)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}
	};


	// Note (0x): this does not belong here.
	class AudioFile : public Asset
	{
	public:
		double Duration;
		uint32_t SamplingRate;
		uint16_t BitDepth;
		uint16_t NumChannels;
		uint64_t FileSize;

		AudioFile() = default;
		AudioFile(double duration, uint32_t samplingRate, uint16_t bitDepth, uint16_t numChannels, uint64_t fileSize)
			: Duration(duration), SamplingRate(samplingRate), BitDepth(bitDepth), NumChannels(numChannels), FileSize(fileSize)
		{
		}

		static AssetType GetStaticType() { return AssetType::Audio; }
		virtual AssetType GetAssetType() const override { return GetStaticType(); }
	};

	template<typename T>
	struct AsyncAssetResult
	{
		Ref<T> Asset;
		bool IsReady = false;

		AsyncAssetResult() = default;
		AsyncAssetResult(const AsyncAssetResult<T>& other) = default;

		AsyncAssetResult(Ref<T> asset, bool isReady = false)
			: Asset(asset), IsReady(isReady) {}

		template<typename T2>
		AsyncAssetResult(const AsyncAssetResult<T2>& other)
			: Asset(other.Asset.template As<T>()), IsReady(other.IsReady) {}

		operator Ref<T>() const { return Asset; }
		operator bool() const { return IsReady; }
	};

}
