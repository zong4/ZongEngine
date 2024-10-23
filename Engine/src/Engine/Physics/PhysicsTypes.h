#pragma once

namespace Hazel {

	enum class ECookingResult
	{
		Success,
		ZeroAreaTestFailed,
		PolygonLimitReached,
		LargeTriangle,
		InvalidMesh,
		Failure,
		None
	};

	enum class EForceMode : uint8_t
	{
		Force = 0,
		Impulse,
		VelocityChange,
		Acceleration
	};

	enum class EActorAxis : uint32_t
	{
		None = 0,
		TranslationX = BIT(0), TranslationY = BIT(1), TranslationZ = BIT(2), Translation = TranslationX | TranslationY | TranslationZ,
		RotationX = BIT(3), RotationY = BIT(4), RotationZ = BIT(5), Rotation = RotationX | RotationY | RotationZ
	};

	enum class ECollisionDetectionType : uint32_t
	{
		Discrete,
		Continuous
	};

	enum class ECollisionFlags : uint8_t
	{
		None,
		Sides = BIT(0),
		Above = BIT(1),
		Below = BIT(2),
	};

	enum class EBodyType { Static, Dynamic, Kinematic };

	enum class EFalloffMode { Constant, Linear };

}
