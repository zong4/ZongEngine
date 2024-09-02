using System;
using Coral.Managed.Interop;

namespace Hazel
{
	public abstract class Component
	{
		public Entity Entity { get; internal set; }
	}

	public class TagComponent : Component
	{
		public string? Tag
		{
			get { unsafe { return InternalCalls.TagComponent_GetTag(Entity.ID); } }
			set { unsafe { InternalCalls.TagComponent_SetTag(Entity.ID, value); } }
		}
	}

	public class TransformComponent : Component
	{
		/// <summary>
		/// Transform relative to parent entity (Does <b>NOT</b> account for RigidBody transform)
		/// </summary>
		public Transform LocalTransform
		{
			get
			{
				unsafe
				{
					Transform transform;
					InternalCalls.TransformComponent_GetTransform(Entity.ID, &transform);
					return transform;
				}
			}

			set
			{
				unsafe { InternalCalls.TransformComponent_SetTransform(Entity.ID, &value); }
			}
		}

		/// <summary>
		/// Transform in world coordinate space (Does <b>NOT</b> account for RigidBody transform)
		/// </summary>
		public Transform WorldTransform
		{
			get
			{
				Transform result;
				unsafe { InternalCalls.TransformComponent_GetWorldSpaceTransform(Entity.ID, &result); }
				return result;
			}
		}

		public Vector3 Translation
		{
			get
			{
				Vector3 result;
				unsafe { InternalCalls.TransformComponent_GetTranslation(Entity.ID, &result); }
				return result;
			}

			set
			{
				unsafe { InternalCalls.TransformComponent_SetTranslation(Entity.ID, &value); }
			}
		}

		public Vector3 Rotation
		{
			get
			{
				Vector3 result;
				unsafe { InternalCalls.TransformComponent_GetRotation(Entity.ID, &result); }
				return result;
			}

			set
			{
				unsafe { InternalCalls.TransformComponent_SetRotation(Entity.ID, &value); }
			}
		}

		public Quaternion RotationQuat
		{
			get
			{
				Quaternion result;
				unsafe { InternalCalls.TransformComponent_GetRotationQuat(Entity.ID, &result); }
				return result;
			}

			set
			{
				unsafe { InternalCalls.TransformComponent_SetRotationQuat(Entity.ID, &value); }
			}
		}

		public Vector3 Scale
		{
			get
			{
				Vector3 result;
				unsafe { InternalCalls.TransformComponent_GetScale(Entity.ID, &result); }
				return result;
			}

			set
			{
				unsafe { InternalCalls.TransformComponent_SetScale(Entity.ID, &value); }
			}
		}

		public Matrix4 TransformMatrix
		{
			get
			{
				Matrix4 result;
				unsafe { InternalCalls.TransformComponent_GetTransformMatrix(Entity.ID, &result); }
				return result;
			}

			set
			{
				unsafe { InternalCalls.TransformComponent_SetTransformMatrix(Entity.ID, &value); }
			}
		}

		public void SetRotation(Quaternion rotation)
		{
			unsafe { InternalCalls.TransformComponent_SetRotationQuat(Entity.ID, &rotation); }
		}
	}


	// Note: (0x) For backwards compatibilty, the C++ SubmeshComponent is still "MeshComponent" in C#
	//            This may change in future (which will break compatibility with old scripts)
	public class MeshComponent : Component, IEquatable<MeshComponent>
	{
		public Mesh? Mesh
		{
			get
			{
				AssetHandle meshHandle;
				unsafe
				{
					if (!InternalCalls.MeshComponent_GetMesh(Entity.ID, &meshHandle))
						return null;
				}

				return new Mesh(meshHandle.m_Handle);
			}

			set
			{
				unsafe { InternalCalls.MeshComponent_SetMesh(Entity.ID, value.Handle); }
			}
		}

		public bool Visible
		{
			get { unsafe { return InternalCalls.MeshComponent_GetVisible(Entity.ID); } }
			set { unsafe { InternalCalls.MeshComponent_SetVisible(Entity.ID, value); } }
		}

		public bool HasMaterial(int index)
		{
			unsafe { return InternalCalls.MeshComponent_HasMaterial(Entity.ID, index); }
		}

		public Material? GetMaterial(int index)
		{
			if (!HasMaterial(index))
				return null;

			AssetHandle materialHandle;
			unsafe
			{
				if (!InternalCalls.MeshComponent_GetMaterial(Entity.ID, index, &materialHandle))
					return null;
			}

			return new Material(Mesh.Handle, materialHandle, this);
		}

		public bool IsRigged
		{
			get
			{
				unsafe { return InternalCalls.MeshComponent_GetIsRigged(Entity.ID); }
			}
		}

		public override int GetHashCode() => (Mesh.Handle).GetHashCode();
		
		public override bool Equals(object? obj) => obj is MeshComponent other && Equals(other);
		
		public bool Equals(MeshComponent? right)
		{
			if (right is null)
				return false;

			if (ReferenceEquals(this, right))
				return true;

			return Mesh == right.Mesh;
		}
		
		public static bool operator==(MeshComponent left, MeshComponent right) => left is null ? right is null : left.Equals(right);
		public static bool operator!=(MeshComponent left, MeshComponent right) => !(left == right);
	}

	public class StaticMeshComponent : Component
	{
		public StaticMesh Mesh
		{
			get
			{
				AssetHandle outHandle;
				unsafe { InternalCalls.StaticMeshComponent_GetMesh(Entity.ID, &outHandle); }
				return new StaticMesh(outHandle.m_Handle);
			}

			set
			{
				unsafe { InternalCalls.StaticMeshComponent_SetMesh(Entity.ID, value.Handle); }
			}
		}

		public bool HasMaterial(int index)
		{
			unsafe { return InternalCalls.StaticMeshComponent_HasMaterial(Entity.ID, index); }
		}

		public Material? GetMaterial(int index)
		{
			if (!HasMaterial(index))
				return null;

			AssetHandle materialHandle;

			unsafe
			{
				if (!InternalCalls.StaticMeshComponent_GetMaterial(Entity.ID, index, &materialHandle))
					return null;
			}

			return new Material(Mesh.Handle, materialHandle, this);
		}

		public void SetMaterial(int index, Material material)
		{
			unsafe
			{
				InternalCalls.StaticMeshComponent_SetMaterial(Entity.ID, index, material.Handle);
			}
		}

		public bool Visible
		{
			get { unsafe { return InternalCalls.StaticMeshComponent_GetVisible(Entity.ID); } }
			set { unsafe { InternalCalls.StaticMeshComponent_SetVisible(Entity.ID, value); } }
		}
	}

	public readonly struct Identifier
	{
		public readonly uint ID;

		public static readonly Identifier Invalid = new Identifier(0);

		private Identifier(uint id)
		{
			ID = id;
		}

		public Identifier(string name)
		{
			unsafe { ID = InternalCalls.Identifier_Get(name); }
		}

		public static implicit operator uint(Identifier id) => id.ID;
		public override string ToString() => ID.ToString();
	}

	public class AnimationComponent : Component
	{
		public bool GetInputBool(uint inputID)
		{
			unsafe { return InternalCalls.AnimationComponent_GetInputBool(Entity.ID, inputID); }
		}

		public void SetInputBool(uint inputID, bool value)
		{
			unsafe { InternalCalls.AnimationComponent_SetInputBool(Entity.ID, inputID, value); }
		}

		public int GetInputInt(uint inputID)
		{
			unsafe { return InternalCalls.AnimationComponent_GetInputInt(Entity.ID, inputID); }
		}
		
		public void SetInputInt(uint inputID, int value)
		{
			unsafe { InternalCalls.AnimationComponent_SetInputInt(Entity.ID, inputID, value); }
		}

		public float GetInputFloat(uint inputID)
		{
			unsafe { return InternalCalls.AnimationComponent_GetInputFloat(Entity.ID, inputID); }
		}

		public void SetInputFloat(uint inputID, float value)
		{
			unsafe { InternalCalls.AnimationComponent_SetInputFloat(Entity.ID, inputID, value); }
		}

		public Vector3 GetInputVector3(uint inputID)
		{
			unsafe
			{
				Vector3 result;
				InternalCalls.AnimationComponent_GetInputVector3(Entity.ID, inputID, &result);
				return result;
			}
		}

		public void SetInputVector3(uint inputID, Vector3 value)
		{
			unsafe
			{
				InternalCalls.AnimationComponent_SetInputVector3(Entity.ID, inputID, &value);
			}
		}
		
		public void SetInputTrigger(uint inputID)
		{
			unsafe
			{
				InternalCalls.AnimationComponent_SetInputTrigger(Entity.ID, inputID);
			}
		}

		public Transform RootMotion
		{
			get
			{
				Transform result;
				unsafe { InternalCalls.AnimationComponent_GetRootMotion(Entity.ID, &result); }
				return result;
			}
		}
	}

	public enum CameraComponentType
	{
		None = -1,
		Perspective, 
		Orthographic
	}

	public class CameraComponent : Component
	{
		public void SetPerspective(float verticalFov, float nearClip, float farClip)
		{
			unsafe { InternalCalls.CameraComponent_SetPerspective(Entity.ID, verticalFov, nearClip, farClip); }
		}

		public void SetOrthographic(float size, float nearClip, float farClip)
		{
			unsafe { InternalCalls.CameraComponent_SetOrthographic(Entity.ID, size, nearClip, farClip); }
		}

		public float VerticalFOV
		{
			get
			{
				unsafe { return InternalCalls.CameraComponent_GetVerticalFOV(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CameraComponent_SetVerticalFOV(Entity.ID, value); }
			}
		}

		public float PerspectiveNearClip
		{
			get
			{
				unsafe { return InternalCalls.CameraComponent_GetPerspectiveNearClip(Entity.ID); }
			}

			set
			{
				unsafe {  InternalCalls.CameraComponent_SetPerspectiveNearClip(Entity.ID, value); }
			}
		}

		public float PerspectiveFarClip
		{
			get
			{
				unsafe { return InternalCalls.CameraComponent_GetPerspectiveFarClip(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CameraComponent_SetPerspectiveFarClip(Entity.ID, value); }
			}
		}

		public float OrthographicSize
		{
			get
			{
				unsafe { return InternalCalls.CameraComponent_GetOrthographicSize(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CameraComponent_SetOrthographicSize(Entity.ID, value); }
			}
		}

		public float OrthographicNearClip
		{
			get
			{
				unsafe { return InternalCalls.CameraComponent_GetOrthographicNearClip(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CameraComponent_SetOrthographicNearClip(Entity.ID, value); }
			}
		}

		public float OrthographicFarClip
		{
			get
			{
				unsafe { return InternalCalls.CameraComponent_GetOrthographicFarClip(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CameraComponent_SetOrthographicFarClip(Entity.ID, value); }
			}
		}

		public CameraComponentType ProjectionType
		{
			get
			{
				unsafe { return InternalCalls.CameraComponent_GetProjectionType(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CameraComponent_SetProjectionType(Entity.ID, value); }
			}
		}

		public bool Primary
		{
			get
			{
				unsafe { return InternalCalls.CameraComponent_GetPrimary(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CameraComponent_SetPrimary(Entity.ID, value); }
			}
		}

		public Vector2 ToScreenSpace(Vector3 worldTranslation)
		{
			Vector2 result;
			unsafe { InternalCalls.CameraComponent_ToScreenSpace(Entity.ID, &worldTranslation, &result); }
			return result;
		}
	}

	public class DirectionalLightComponent : Component
	{
		public Vector3 Radiance
		{
			get
			{
				Vector3 result;
				unsafe { InternalCalls.DirectionalLightComponent_GetRadiance(Entity.ID, &result); }
				return result;
			}

			set
			{
				unsafe { InternalCalls.DirectionalLightComponent_SetRadiance(Entity.ID, &value); }
			}
		}

		public float Intensity
		{
			get
			{
				unsafe { return InternalCalls.DirectionalLightComponent_GetIntensity(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.DirectionalLightComponent_SetIntensity(Entity.ID, value); }
			}
		}

		public bool CastShadows
		{
			get
			{
				unsafe { return InternalCalls.DirectionalLightComponent_GetCastShadows(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.DirectionalLightComponent_SetCastShadows(Entity.ID, value); }
			}
		}

		public bool SoftShadows
		{
			get
			{
				unsafe { return InternalCalls.DirectionalLightComponent_GetSoftShadows(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.DirectionalLightComponent_SetSoftShadows(Entity.ID, value); }
			}
		}

		public float LightSize
		{
			get
			{
				unsafe { return InternalCalls.DirectionalLightComponent_GetLightSize(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.DirectionalLightComponent_SetLightSize(Entity.ID, value); }
			}
		}
	}

	public class PointLightComponent : Component
	{
		public Vector3 Radiance
		{
			get
			{
				Vector3 result;
				unsafe { InternalCalls.PointLightComponent_GetRadiance(Entity.ID, &result); }
				return result;
			}

			set
			{
				unsafe { InternalCalls.PointLightComponent_SetRadiance(Entity.ID, &value); }
			}
		}

		public float Intensity
		{
			get
			{
				unsafe
				{
					return InternalCalls.PointLightComponent_GetIntensity(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.PointLightComponent_SetIntensity(Entity.ID, value);
				}
			}
		}

		public float Radius
		{
			get
			{
				unsafe
				{
					return InternalCalls.PointLightComponent_GetRadius(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.PointLightComponent_SetRadius(Entity.ID, value);
				}
			}
		}

		public float Falloff
		{
			get
			{
				unsafe
				{
					return InternalCalls.PointLightComponent_GetFalloff(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.PointLightComponent_SetFalloff(Entity.ID, value);
				}
			}
		}
	}

	public class SpotLightComponent : Component
	{
		public Vector3 Radiance
		{
			get
			{
				Vector3 result;
				unsafe { InternalCalls.SpotLightComponent_GetRadiance(Entity.ID, &result); }
				return result;
			}

			set
			{
				unsafe { InternalCalls.SpotLightComponent_SetRadiance(Entity.ID, &value); }
			}
		}

		public float Intensity
		{
			get
			{
				unsafe
				{
					return InternalCalls.SpotLightComponent_GetIntensity(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SpotLightComponent_SetIntensity(Entity.ID, value);
				}
			}
		}

		public float Range
		{
			get
			{
				unsafe
				{
					return InternalCalls.SpotLightComponent_GetRange(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SpotLightComponent_SetRange(Entity.ID, value);
				}
			}
		}

		public float Angle
		{
			get
			{
				unsafe
				{
					return InternalCalls.SpotLightComponent_GetAngle(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SpotLightComponent_SetAngle(Entity.ID, value);
				}
			}
		}
		public float AngleAttenuation
		{
			get
			{
				unsafe
				{
					return InternalCalls.SpotLightComponent_GetAngleAttenuation(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SpotLightComponent_SetAngleAttenuation(Entity.ID, value);
				}
			}
		}

		public float Falloff
		{
			get
			{
				unsafe
				{
					return InternalCalls.SpotLightComponent_GetFalloff(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SpotLightComponent_SetFalloff(Entity.ID, value);
				}
			}
		}

		public bool CastsShadows
		{
			get
			{
				unsafe
				{
					return InternalCalls.SpotLightComponent_GetCastsShadows(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SpotLightComponent_SetCastsShadows(Entity.ID, value);
				}
			}
		}

		public bool SoftShadows
		{
			get
			{
				unsafe
				{
					return InternalCalls.SpotLightComponent_GetSoftShadows(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SpotLightComponent_SetSoftShadows(Entity.ID, value);
				}
			}
		}
	}
	
	public class SkyLightComponent : Component
	{
		public float Intensity
		{
			get
			{
				unsafe
				{
					return InternalCalls.SkyLightComponent_GetIntensity(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SkyLightComponent_SetIntensity(Entity.ID, value);
				}
			}
		}

		public float Turbidity
		{
			get
			{
				unsafe
				{
					return InternalCalls.SkyLightComponent_GetTurbidity(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SkyLightComponent_SetTurbidity(Entity.ID, value);
				}
			}
		}

		public float Azimuth
		{
			get
			{
				unsafe
				{
					return InternalCalls.SkyLightComponent_GetAzimuth(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SkyLightComponent_SetAzimuth(Entity.ID, value);
				}
			}
		}

		public float Inclination
		{
			get
			{
				unsafe
				{
					return InternalCalls.SkyLightComponent_GetInclination(Entity.ID);
				}
			}
			set
			{
				unsafe
				{
					InternalCalls.SkyLightComponent_SetInclination(Entity.ID, value);
				}
			}
		}
	}

	public class ScriptComponent : Component
	{
		public NativeInstance<object> Instance
		{
			get
			{
				unsafe { return InternalCalls.ScriptComponent_GetInstance(Entity.ID); }
			}
		}
	}

	public class SpriteRendererComponent : Component
	{
		public Vector4 Color
		{
			get
			{
				Vector4 color;
				unsafe { InternalCalls.SpriteRendererComponent_GetColor(Entity.ID, &color); }
				return color;
			}

			set
			{
				unsafe
				{
					InternalCalls.SpriteRendererComponent_SetColor(Entity.ID, &value);
				}
			}
		}

		public float TilingFactor
		{
			get
			{
				unsafe
				{
					return InternalCalls.SpriteRendererComponent_GetTilingFactor(Entity.ID);
				}
			}

			set
			{
				unsafe
				{
					InternalCalls.SpriteRendererComponent_SetTilingFactor(Entity.ID, value);
				}
			}
		}

		public Vector2 UVStart
		{ 
			get
			{
				Vector2 uvStart;
				unsafe { InternalCalls.SpriteRendererComponent_GetUVStart(Entity.ID, &uvStart); }
				return uvStart;
			}

			set
			{
				unsafe { InternalCalls.SpriteRendererComponent_SetUVStart(Entity.ID, &value); }
			}
		}

		public Vector2 UVEnd
		{ 
			get
			{
				Vector2 uvEnd;
				unsafe { InternalCalls.SpriteRendererComponent_GetUVEnd(Entity.ID, &uvEnd); }
				return uvEnd;
			}

			set
			{
				unsafe { InternalCalls.SpriteRendererComponent_SetUVEnd(Entity.ID, &value); }
			}
		}
	}

	public enum RigidBody2DBodyType
	{
		None = -1, 
		Static, 
		Dynamic, 
		Kinematic
	}

	public class RigidBody2DComponent : Component
	{
		public RigidBody2DBodyType BodyType
		{
			get
			{
				unsafe { return InternalCalls.RigidBody2DComponent_GetBodyType(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetBodyType(Entity.ID, value); }
			}
		}

		public Vector2 Translation
		{
			get
			{
				Vector2 translation;
				unsafe { InternalCalls.RigidBody2DComponent_GetTranslation(Entity.ID, &translation); }
				return translation;
			}

			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetTranslation(Entity.ID, &value); }
			}
		}

		public float Rotation
		{
			get
			{
				unsafe { return InternalCalls.RigidBody2DComponent_GetRotation(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetRotation(Entity.ID, value); }
			}
		}

		public float Mass
		{
			get
			{
				unsafe { return InternalCalls.RigidBody2DComponent_GetMass(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetMass(Entity.ID, value); }
			}
		}

		public Vector2 LinearVelocity
		{
			get
			{
				Vector2 velocity;
				unsafe { InternalCalls.RigidBody2DComponent_GetLinearVelocity(Entity.ID, &velocity); }
				return velocity;
			}

			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetLinearVelocity(Entity.ID, &value); }
			}
		}

		public float GravityScale
		{
			get
			{
				unsafe { return InternalCalls.RigidBody2DComponent_GetGravityScale(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBody2DComponent_SetGravityScale(Entity.ID, value); }
			}
		}

		public void ApplyLinearImpulse(Vector2 impulse, Vector2 offset, bool wake)
		{
			unsafe
			{
				InternalCalls.RigidBody2DComponent_ApplyLinearImpulse(Entity.ID, &impulse, &offset, wake);
			}
		}

		public void ApplyAngularImpulse(float impulse, bool wake)
		{
			unsafe
			{
				InternalCalls.RigidBody2DComponent_ApplyAngularImpulse(Entity.ID, impulse, wake);
			}
		}

		public void AddForce(Vector2 force, Vector2 offset, bool wake)
		{
			unsafe
			{
				InternalCalls.RigidBody2DComponent_AddForce(Entity.ID, &force, &offset, wake);
			}
		}

		public void AddTorque(float torque, bool wake)
		{
			unsafe
			{
				InternalCalls.RigidBody2DComponent_AddTorque(Entity.ID, torque, wake);
			}
		}
	}

	public class BoxCollider2DComponent : Component
	{
	}

	public enum EBodyType
	{
		None = -1,
		Static,
		Dynamic,
		Kinematic
	}

	public class RigidBodyComponent : Component
	{

		public EBodyType BodyType
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_GetBodyType(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetBodyType(Entity.ID, value); }
			}
		}
		
		public float Mass
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_GetMass(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetMass(Entity.ID, value); }
			}
		}

		public Vector3 LinearVelocity
		{
			get
			{
				Vector3 velocity;
				unsafe { InternalCalls.RigidBodyComponent_GetLinearVelocity(Entity.ID, &velocity); }
				return velocity;
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetLinearVelocity(Entity.ID, &value); }
			}
		}

		public Vector3 AngularVelocity
		{
			get
			{
				Vector3 velocity;
				unsafe { InternalCalls.RigidBodyComponent_GetAngularVelocity(Entity.ID, &velocity); }
				return velocity;
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetAngularVelocity(Entity.ID, &value); }
			}
		}

		public bool IsTrigger
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_IsTrigger(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetTrigger(Entity.ID, value); }
			}
		}

		public float MaxLinearVelocity
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_GetMaxLinearVelocity(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetMaxLinearVelocity(Entity.ID, value); }
			}
		}

		public float MaxAngularVelocity
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_GetMaxAngularVelocity(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetMaxAngularVelocity(Entity.ID, value); }
			}
		}

		public float LinearDrag
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_GetLinearDrag(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetLinearDrag(Entity.ID, value); }
			}
		}

		public float AngularDrag
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_GetAngularDrag(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetAngularDrag(Entity.ID, value); }
			}
		}

		public uint Layer
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_GetLayer(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetLayer(Entity.ID, value); }
			}
		}

		public string LayerName
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_GetLayerName(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetLayerByName(Entity.ID, value); }
			}
		}

		public bool IsSleeping
		{
			get
			{
				unsafe { return InternalCalls.RigidBodyComponent_IsSleeping(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.RigidBodyComponent_SetIsSleeping(Entity.ID, value); }
			}
		}

		public void MoveKinematic(Vector3 targetPosition, Vector3 targetRotation, float deltaSeconds)
		{
			unsafe { InternalCalls.RigidBodyComponent_MoveKinematic(Entity.ID, &targetPosition, &targetRotation, deltaSeconds); }
		}

		public void AddForce(Vector3 force, EForceMode forceMode = EForceMode.Force)
		{
			unsafe
			{
				InternalCalls.RigidBodyComponent_AddForce(Entity.ID, &force, forceMode);
			}
		}

		public void AddForceAtLocation(Vector3 force, Vector3 location, EForceMode forceMode = EForceMode.Force)
		{
			unsafe
			{
				InternalCalls.RigidBodyComponent_AddForceAtLocation(Entity.ID, &force, &location, forceMode);
			}
		}

		public void AddTorque(Vector3 torque, EForceMode forceMode = EForceMode.Force)
		{
			unsafe
			{
				InternalCalls.RigidBodyComponent_AddTorque(Entity.ID, &torque, forceMode);
			}
		}

		// Rotation should be in radians
		public void Rotate(Vector3 rotation)
		{
			unsafe { InternalCalls.RigidBodyComponent_Rotate(Entity.ID, &rotation); }
		}

		public void SetAxisLock(EActorAxis axis, bool value, bool forceWake = false)
		{
			unsafe { InternalCalls.RigidBodyComponent_SetAxisLock(Entity.ID, axis, value, forceWake); }
		}

		public bool IsAxisLocked(EActorAxis axis)
		{
			unsafe { return InternalCalls.RigidBodyComponent_IsAxisLocked(Entity.ID, axis); }
		}

		public uint GetLockedAxes()
		{
			unsafe { return InternalCalls.RigidBodyComponent_GetLockedAxes(Entity.ID); }
		}
	}

	public enum ECollisionFlags
	{
		None, Sides, Above, Below
	}

	public class CharacterControllerComponent : Component
	{
		public bool IsGravityEnabled
		{
			get
			{
				unsafe { return InternalCalls.CharacterControllerComponent_GetIsGravityEnabled(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CharacterControllerComponent_SetIsGravityEnabled(Entity.ID, value); }
			}
		}

		public float SlopeLimit
		{
			get
			{
				unsafe { return InternalCalls.CharacterControllerComponent_GetSlopeLimit(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CharacterControllerComponent_SetSlopeLimit(Entity.ID, value); }
			}
		}

		public float StepOffset
		{
			get
			{
				unsafe { return InternalCalls.CharacterControllerComponent_GetStepOffset(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.CharacterControllerComponent_SetStepOffset(Entity.ID, value); }
			}
		}

		public Vector3 LinearVelocity
		{
			get
			{
				Vector3 velocity;
				unsafe { InternalCalls.CharacterControllerComponent_GetLinearVelocity(Entity.ID, &velocity); }
				return velocity;
			}

			set
			{
				unsafe { InternalCalls.CharacterControllerComponent_SetLinearVelocity(Entity.ID, &value); }
			}
		}

		public bool IsGrounded
		{
			get
			{
				unsafe { return InternalCalls.CharacterControllerComponent_IsGrounded(Entity.ID); }
			}
		}

		public ECollisionFlags CollisionFlags
		{
			get
			{
				unsafe { return InternalCalls.CharacterControllerComponent_GetCollisionFlags(Entity.ID); }
			}
		}

		public void SetTranslation(Vector3 translation)
		{
			unsafe { InternalCalls.CharacterControllerComponent_SetTranslation(Entity.ID, &translation); }
		}

		public void SetRotation(Quaternion rotation)
		{
			unsafe { InternalCalls.CharacterControllerComponent_SetRotation(Entity.ID, &rotation); }
		}

		public void Move(Vector3 displacement)
		{
			unsafe { InternalCalls.CharacterControllerComponent_Move(Entity.ID, &displacement); }
		}
		public void Rotate(Quaternion rotation)
		{
			unsafe { InternalCalls.CharacterControllerComponent_Rotate(Entity.ID, &rotation); }
		}

		public void Jump(float jumpPower)
		{
			unsafe { InternalCalls.CharacterControllerComponent_Jump(Entity.ID, jumpPower); }
		}
	}

	public class BoxColliderComponent : Component
	{
		public Vector3 HalfSize
		{
			get
			{
				Vector3 halfSize;
				unsafe { InternalCalls.BoxColliderComponent_GetHalfSize(Entity.ID, &halfSize); }
				return halfSize;
			}
		}

		public Vector3 Offset
		{
			get
			{
				Vector3 offset;
				unsafe { InternalCalls.BoxColliderComponent_GetOffset(Entity.ID, &offset); }
				return offset;
			}
		}

		public PhysicsMaterial Material
		{
			get
			{
				PhysicsMaterial material;
				unsafe { InternalCalls.BoxColliderComponent_GetMaterial(Entity.ID, &material); }
				return material;
			}

			set
			{
				unsafe { InternalCalls.BoxColliderComponent_SetMaterial(Entity.ID, &value); }
			}
		}
	}

	public class SphereColliderComponent : Component
	{

		public float Radius
		{
			get
			{
				unsafe { return InternalCalls.SphereColliderComponent_GetRadius(Entity.ID); }
			}
		}

		public Vector3 Offset
		{
			get
			{
				Vector3 offset;
				unsafe { InternalCalls.SphereColliderComponent_GetOffset(Entity.ID, &offset); }
				return offset;
			}
		}

		public PhysicsMaterial Material
		{
			get
			{
				PhysicsMaterial material;
				unsafe { InternalCalls.SphereColliderComponent_GetMaterial(Entity.ID, &material); }
				return material;
			}

			set
			{
				unsafe { InternalCalls.SphereColliderComponent_SetMaterial(Entity.ID, &value); }
			}
		}
	}

	public class CapsuleColliderComponent : Component
	{

		public float Radius
		{
			get
			{
				unsafe { return InternalCalls.CapsuleColliderComponent_GetRadius(Entity.ID); }
			}
		}

		public float HalfHeight
		{
			get
			{
				unsafe { return InternalCalls.CapsuleColliderComponent_GetHeight(Entity.ID); }
			}
		}

		public Vector3 Offset
		{
			get
			{
				Vector3 offset;
				unsafe { InternalCalls.CapsuleColliderComponent_GetOffset(Entity.ID, &offset); }
				return offset;
			}
		}

		public PhysicsMaterial Material
		{
			get
			{
				PhysicsMaterial material;
				unsafe { InternalCalls.CapsuleColliderComponent_GetMaterial(Entity.ID, &material); }
				return material;
			}

			set
			{
				unsafe { InternalCalls.CapsuleColliderComponent_SetMaterial(Entity.ID, &value); }
			}
		}
	}

	public class MeshColliderComponent : Component
	{
		public bool IsStaticMesh
		{
			get
			{
				unsafe { return InternalCalls.MeshColliderComponent_IsMeshStatic(Entity.ID); }
			}
		}

		public AssetHandle ColliderMeshHandle
		{
			get
			{
				AssetHandle colliderHandle;
				unsafe
				{
					bool result = InternalCalls.MeshColliderComponent_GetColliderMesh(Entity.ID, &colliderHandle);
					return result ? colliderHandle : AssetHandle.Invalid;
				}
			}
		}

		public PhysicsMaterial Material
		{
			get
			{
				PhysicsMaterial material;
				unsafe { InternalCalls.MeshColliderComponent_GetMaterial(Entity.ID, &material); }
				return material;
			}

			set
			{
				unsafe { InternalCalls.MeshColliderComponent_SetMaterial(Entity.ID, &value); }
			}
		}

		private Mesh m_ColliderMesh;
		private StaticMesh m_StaticColliderMesh;

		public Mesh GetColliderMesh()
		{
			if (IsStaticMesh)
				throw new InvalidOperationException("Tried to access a dynamic Mesh in MeshColliderComponent when the collision mesh is a StaticMesh. Use MeshColliderComponent.GetStaticColliderMesh() instead.");

			if (!ColliderMeshHandle.IsValid())
				return null;

			if (m_ColliderMesh == null || m_ColliderMesh.Handle != ColliderMeshHandle)
				m_ColliderMesh = new Mesh(ColliderMeshHandle.m_Handle);

			return m_ColliderMesh;
		}

		public StaticMesh GetStaticColliderMesh()
		{
			if (!IsStaticMesh)
				throw new InvalidOperationException("Tried to access a static Mesh in MeshColliderComponent when the collision mesh is a dynamic Mesh. Use MeshColliderComponent.GetColliderMesh() instead.");

			if (!ColliderMeshHandle.IsValid())
				return null;

			if (m_ColliderMesh == null || m_StaticColliderMesh.Handle != ColliderMeshHandle)
				m_StaticColliderMesh = new StaticMesh(ColliderMeshHandle.m_Handle);

			return m_StaticColliderMesh;
		}
	}

	public class AudioListenerComponent : Component
	{
	}

	public class AudioComponent : Component
	{
		public bool IsPlaying()
		{
			unsafe { return InternalCalls.AudioComponent_IsPlaying(Entity.ID); }
		}

		public bool Play(float startTime = 0.0f)
		{
			unsafe { return InternalCalls.AudioComponent_Play(Entity.ID, startTime); }
		}
		
		public bool Stop()
		{
			unsafe { return InternalCalls.AudioComponent_Stop(Entity.ID); }
		}

		public bool Pause()
		{
			unsafe { return InternalCalls.AudioComponent_Pause(Entity.ID); }
		}

		public bool Resume()
		{
			unsafe { return InternalCalls.AudioComponent_Resume(Entity.ID); }
		}

		public float VolumeMultiplier
		{
			get
			{
				unsafe { return InternalCalls.AudioComponent_GetVolumeMult(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.AudioComponent_SetVolumeMult(Entity.ID, value); }
			}
		}

		public float PitchMultiplier
		{
			get
			{
				unsafe { return InternalCalls.AudioComponent_GetPitchMult(Entity.ID); }
			}

			set
			{
				unsafe { InternalCalls.AudioComponent_SetPitchMult(Entity.ID, value); }
			}
		}

		public void SetEvent(AudioCommandID eventID)
		{
			unsafe { InternalCalls.AudioComponent_SetEvent(Entity.ID, eventID); }
		}

		public void SetEvent(string eventName)
		{
			unsafe { InternalCalls.AudioComponent_SetEvent(Entity.ID, new AudioCommandID(eventName)); }
		}
	}

	public class TextComponent : Component
	{
		private string m_Text;
		private ulong m_Hash = 0;

		public string Text
		{
			get
			{
				unsafe
				{
					ulong internalHash = InternalCalls.TextComponent_GetHash(Entity.ID);
					if (m_Hash != internalHash)
					{
						m_Text = InternalCalls.TextComponent_GetText(Entity.ID);
						m_Hash = internalHash;
					}
				}

				return m_Text;
			}

			set
			{
				unsafe { InternalCalls.TextComponent_SetText(Entity.ID, value); }
			}
		}

		public Vector4 Color
		{
			get
			{
				Vector4 color;
				unsafe { InternalCalls.TextComponent_GetColor(Entity.ID, &color); }
				return color;
			}

			set
			{
				unsafe { InternalCalls.TextComponent_SetColor(Entity.ID, &value); }
			}
		}
	}

}
