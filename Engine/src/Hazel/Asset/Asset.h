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

		bool IsValid() const { return ((Flags & (uint16_t)AssetFlag::Missing) | (Flags & (uint16_t)AssetFlag::Invalid)) == 0; }

		virtual bool operator==(const Asset& other) const
		{
			return Handle == other.Handle;
		}
		
		virtual bool operator!=(const Asset& other) const
		{
			return !(*this == other);
		}

		bool IsFlagSet(AssetFlag flag) const { return (uint16_t)flag & Flags; }
		void SetFlag(AssetFlag flag, bool value = true)
		{
			if (value)
				Flags |= (uint16_t)flag;
			else
				Flags &= ~(uint16_t)flag;
		}
	};

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
}
