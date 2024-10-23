using System;

namespace Hazel
{
	public abstract class Component
	{
		public Entity Entity { get; internal set; }
	}

	public class TagComponent : Component
	{
		public string Tag
		{
			get => InternalCalls.TagComponent_GetTag(Entity.ID);
			set => InternalCalls.TagComponent_SetTag(Entity.ID, value);
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
				InternalCalls.TransformComponent_GetTransform(Entity.ID, out Transform result);
				return result;
			}

			set => InternalCalls.TransformComponent_SetTransform(Entity.ID, ref value);
		}

		/// <summary>
		/// Transform in world coordinate space (Does <b>NOT</b> account for RigidBody transform)
		/// </summary>
		public Transform WorldTransform
		{
			get
			{
				InternalCalls.TransformComponent_GetWorldSpaceTransform(Entity.ID, out Transform result);
				return result;
			}
		}

		public Vector3 Translation
		{
			get
			{
				InternalCalls.TransformComponent_GetTranslation(Entity.ID, out Vector3 result);
				return result;
			}

			set => InternalCalls.TransformComponent_SetTranslation(Entity.ID, ref value);
		}

		public Vector3 Rotation
		{
			get
			{
				InternalCalls.TransformComponent_GetRotation(Entity.ID, out Vector3 result);
				return result;
			}

			set => InternalCalls.TransformComponent_SetRotation(Entity.ID, ref value);
		}

		public Vector3 Scale
		{
			get
			{
				InternalCalls.TransformComponent_GetScale(Entity.ID, out Vector3 result);
				return result;
			}

			set => InternalCalls.TransformComponent_SetScale(Entity.ID, ref value);
		}

		public Matrix4 TransformMatrix
		{
			get
			{
				InternalCalls.TransformComponent_GetTransformMatrix(Entity.ID, out Matrix4 result);
				return result;
			}

			set => InternalCalls.TransformComponent_SetTransformMatrix(Entity.ID, ref value);
		}

		public void SetRotation(Quaternion rotation)
		{
			InternalCalls.TransformComponent_SetRotationQuat(Entity.ID, ref rotation);
		}

	}

	public class MeshComponent : Component, IEquatable<MeshComponent>
	{
		public Mesh Mesh
		{
			get
			{
				if (!InternalCalls.MeshComponent_GetMesh(Entity.ID, out AssetHandle meshHandle))
					return null;

				return new Mesh(meshHandle);
			}

			set => InternalCalls.MeshComponent_SetMesh(Entity.ID, ref value.m_Handle);

		}

		public bool HasMaterial(int index) => InternalCalls.MeshComponent_HasMaterial(Entity.ID, index);

		public Material GetMaterial(int index)
		{
			if (!HasMaterial(index) || !InternalCalls.MeshComponent_GetMaterial(Entity.ID, index, out AssetHandle materialHandle))
				return null;

			return new Material(Mesh.Handle, materialHandle, this);
		}
		public bool IsRigged => InternalCalls.MeshComponent_GetIsRigged(Entity.ID);


		public override int GetHashCode() => (Mesh.m_Handle).GetHashCode();
		
		public override bool Equals(object obj) => obj is MeshComponent other && Equals(other);
		
		public bool Equals(MeshComponent right)
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
				InternalCalls.StaticMeshComponent_GetMesh(Entity.ID, out AssetHandle outHandle);
				return new StaticMesh(outHandle);
			}
			set => InternalCalls.StaticMeshComponent_SetMesh(Entity.ID, ref value.m_Handle);
		}

		public bool HasMaterial(int index) => InternalCalls.StaticMeshComponent_HasMaterial(Entity.ID, index);

		public Material GetMaterial(int index)
		{
			if (!HasMaterial(index) || !InternalCalls.StaticMeshComponent_GetMaterial(Entity.ID, index, out AssetHandle materialHandle))
				return null;

			return new Material(Mesh.Handle, materialHandle, this);
		}

		public void SetMaterial(int index, Material material)
		{
			InternalCalls.StaticMeshComponent_SetMaterial(Entity.ID, index, material.Handle.m_Handle);
		}

		public bool Visible
		{
			get => InternalCalls.StaticMeshComponent_IsVisible(Entity.ID);
			set => InternalCalls.StaticMeshComponent_SetVisible(Entity.ID, value);
		}

	}

	public readonly struct Identifier
	{
		public readonly uint ID;

		public Identifier(string name)
		{
			ID = InternalCalls.Identifier_Get(name);
		}

		public static implicit operator uint(Identifier id) => id.ID;
		public override string ToString() => ID.ToString();
	}

	public class AnimationComponent : Component
	{
		public bool GetInputBool(uint inputID)
		{
			return InternalCalls.AnimationComponent_GetInputBool(Entity.ID, inputID);
		}

		public void SetInputBool(uint inputID, bool value)
		{
			InternalCalls.AnimationComponent_SetInputBool(Entity.ID, inputID, value);
		}

		public int GetInputInt(uint inputID)
		{
			return InternalCalls.AnimationComponent_GetInputInt(Entity.ID, inputID);
		}
		
		public void SetInputInt(uint inputID, int value)
		{
			InternalCalls.AnimationComponent_SetInputInt(Entity.ID, inputID, value);
		}

		public float GetInputFloat(uint inputID)
		{
			return InternalCalls.AnimationComponent_GetInputFloat(Entity.ID, inputID);
		}

		public void SetInputFloat(uint inputID, float value)
		{
			InternalCalls.AnimationComponent_SetInputFloat(Entity.ID, inputID, value);
		}

		public Transform RootMotion
		{
			get
			{
				InternalCalls.AnimationComponent_GetRootMotion(Entity.ID, out Transform result);
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
		public void SetPerspective(float verticalFov, float nearClip, float farClip) => InternalCalls.CameraComponent_SetPerspective(Entity.ID, verticalFov, nearClip, farClip);
		public void SetOrthographic(float size, float nearClip, float farClip) => InternalCalls.CameraComponent_SetOrthographic(Entity.ID, size, nearClip, farClip);

		public float VerticalFOV
		{ 
			get => InternalCalls.CameraComponent_GetVerticalFOV(Entity.ID);
			set => InternalCalls.CameraComponent_SetVerticalFOV(Entity.ID, value);
		}

		public float PerspectiveNearClip
		{
			get => InternalCalls.CameraComponent_GetPerspectiveNearClip(Entity.ID);
			set => InternalCalls.CameraComponent_SetPerspectiveNearClip(Entity.ID, value);
		}

		public float PerspectiveFarClip
		{
			get => InternalCalls.CameraComponent_GetPerspectiveFarClip(Entity.ID);
			set => InternalCalls.CameraComponent_SetPerspectiveFarClip(Entity.ID, value);
		}

		public float OrthographicSize
		{
			get => InternalCalls.CameraComponent_GetOrthographicSize(Entity.ID);
			set => InternalCalls.CameraComponent_SetOrthographicSize(Entity.ID, value);
		}

		public float OrthographicNearClip
		{
			get => InternalCalls.CameraComponent_GetOrthographicNearClip(Entity.ID);
			set => InternalCalls.CameraComponent_SetOrthographicNearClip(Entity.ID, value);
		}

		public float OrthographicFarClip
		{
			get => InternalCalls.CameraComponent_GetOrthographicFarClip(Entity.ID);
			set => InternalCalls.CameraComponent_SetOrthographicFarClip(Entity.ID, value);
		}

		public CameraComponentType ProjectionType
		{
			get => InternalCalls.CameraComponent_GetProjectionType(Entity.ID);
			set => InternalCalls.CameraComponent_SetProjectionType(Entity.ID, value);
		}

		public bool Primary
		{
			get => InternalCalls.CameraComponent_GetPrimary(Entity.ID);
			set => InternalCalls.CameraComponent_SetPrimary(Entity.ID, value);
		}
	}

	public class DirectionalLightComponent : Component
	{
		public Vector3 Radiance
		{
			get
			{
				InternalCalls.DirectionalLightComponent_GetRadiance(Entity.ID, out Vector3 result);
				return result;
			}

			set => InternalCalls.DirectionalLightComponent_SetRadiance(Entity.ID, ref value);
		}

		public float Intensity
		{
			get => InternalCalls.DirectionalLightComponent_GetIntensity(Entity.ID);
			set => InternalCalls.DirectionalLightComponent_SetIntensity(Entity.ID, value);
		}

		public bool CastShadows
		{
			get => InternalCalls.DirectionalLightComponent_GetCastShadows(Entity.ID);
			set => InternalCalls.DirectionalLightComponent_SetCastShadows(Entity.ID, value);
		}

		public bool SoftShadows
		{
			get => InternalCalls.DirectionalLightComponent_GetSoftShadows(Entity.ID);
			set => InternalCalls.DirectionalLightComponent_SetSoftShadows(Entity.ID, value);
		}

		public float LightSize
		{
			get => InternalCalls.DirectionalLightComponent_GetLightSize(Entity.ID);
			set => InternalCalls.DirectionalLightComponent_SetLightSize(Entity.ID, value);
		}
	}

	public class PointLightComponent : Component
	{
		public Vector3 Radiance
		{
			get
			{
				InternalCalls.PointLightComponent_GetRadiance(Entity.ID, out Vector3 result);
				return result;
			}

			set => InternalCalls.PointLightComponent_SetRadiance(Entity.ID, ref value);
		}

		public float Intensity
		{
			get => InternalCalls.PointLightComponent_GetIntensity(Entity.ID);
			set => InternalCalls.PointLightComponent_SetIntensity(Entity.ID, value);
		}

		public float Radius
		{
			get => InternalCalls.PointLightComponent_GetRadius(Entity.ID);
			set => InternalCalls.PointLightComponent_SetRadius(Entity.ID, value);
		}

		public float Falloff
		{
			get => InternalCalls.PointLightComponent_GetFalloff(Entity.ID);
			set => InternalCalls.PointLightComponent_SetFalloff(Entity.ID, value);
		}
	}

	public class SpotLightComponent : Component
	{
		public Vector3 Radiance
		{
			get
			{
				InternalCalls.SpotLightComponent_GetRadiance(Entity.ID, out Vector3 result);
				return result;
			}

			set => InternalCalls.SpotLightComponent_SetRadiance(Entity.ID, ref value);
		}

		public float Intensity
		{
			get => InternalCalls.SpotLightComponent_GetIntensity(Entity.ID);
			set => InternalCalls.SpotLightComponent_SetIntensity(Entity.ID, value);
		}

		public float Range
		{
			get => InternalCalls.SpotLightComponent_GetRange(Entity.ID);
			set => InternalCalls.SpotLightComponent_SetRange(Entity.ID, value);
		}

		public float Angle
		{
			get => InternalCalls.SpotLightComponent_GetAngle(Entity.ID);
			set => InternalCalls.SpotLightComponent_SetAngle(Entity.ID, value);
		}
		public float AngleAttenuation
		{
			get => InternalCalls.SpotLightComponent_GetAngleAttenuation(Entity.ID);
			set => InternalCalls.SpotLightComponent_SetAngleAttenuation(Entity.ID, value);
		}

		public float Falloff
		{
			get => InternalCalls.SpotLightComponent_GetFalloff(Entity.ID);
			set => InternalCalls.SpotLightComponent_SetFalloff(Entity.ID, value);
		}

		public bool CastsShadows
		{
			get => InternalCalls.SpotLightComponent_GetCastsShadows(Entity.ID);
			set => InternalCalls.SpotLightComponent_SetCastsShadows(Entity.ID, value);
		}

		public bool SoftShadows
		{
			get => InternalCalls.SpotLightComponent_GetSoftShadows(Entity.ID);
			set => InternalCalls.SpotLightComponent_SetSoftShadows(Entity.ID, value);
		}
	}
	
	public class SkyLightComponent : Component
	{
		public float Intensity
		{
			get => InternalCalls.SkyLightComponent_GetIntensity(Entity.ID);
			set => InternalCalls.SkyLightComponent_SetIntensity(Entity.ID, value);
		}

		public float Turbidity
		{
			get => InternalCalls.SkyLightComponent_GetTurbidity(Entity.ID);
			set => InternalCalls.SkyLightComponent_SetTurbidity(Entity.ID, value);
		}

		public float Azimuth
		{
			get => InternalCalls.SkyLightComponent_GetAzimuth(Entity.ID);
			set => InternalCalls.SkyLightComponent_SetAzimuth(Entity.ID, value);
		}

		public float Inclination
		{
			get => InternalCalls.SkyLightComponent_GetInclination(Entity.ID);
			private set => InternalCalls.SkyLightComponent_SetInclination(Entity.ID, value);
		}
	}

	public class ScriptComponent : Component
	{
		public object Instance => InternalCalls.ScriptComponent_GetInstance(Entity.ID);
	}

	public class SpriteRendererComponent : Component
	{
		public Vector4 Color
		{
			get
			{
				InternalCalls.SpriteRendererComponent_GetColor(Entity.ID, out Vector4 color);
				return color;
			}
			set => InternalCalls.SpriteRendererComponent_SetColor(Entity.ID, ref value);
		}
		public float TilingFactor
		{
			get => InternalCalls.SpriteRendererComponent_GetTilingFactor(Entity.ID);
			set => InternalCalls.SpriteRendererComponent_SetTilingFactor(Entity.ID, value);
		}
		public Vector2 UVStart
		{ 
			get
			{
				InternalCalls.SpriteRendererComponent_GetUVStart(Entity.ID, out Vector2 uvStart);
				return uvStart;
			}
			set => InternalCalls.SpriteRendererComponent_SetUVStart(Entity.ID, ref value);
		}
		public Vector2 UVEnd
		{ 
			get
			{
				InternalCalls.SpriteRendererComponent_GetUVEnd(Entity.ID, out Vector2 uvEnd);
				return uvEnd;
			}
			set => InternalCalls.SpriteRendererComponent_SetUVEnd(Entity.ID, ref value);
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
			get => InternalCalls.RigidBody2DComponent_GetBodyType(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetBodyType(Entity.ID, value);
		}

		public Vector2 Translation
		{
			get
			{
				InternalCalls.RigidBody2DComponent_GetTranslation(Entity.ID, out Vector2 translation);
				return translation;
			}

			set => InternalCalls.RigidBody2DComponent_SetTranslation(Entity.ID, ref value);
		}

		public float Rotation
		{
			get => InternalCalls.RigidBody2DComponent_GetRotation(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetRotation(Entity.ID, value);
		}

		public float Mass
		{
			get => InternalCalls.RigidBody2DComponent_GetMass(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetMass(Entity.ID, value);
		}

		public Vector2 LinearVelocity
		{
			get
			{
				InternalCalls.RigidBody2DComponent_GetLinearVelocity(Entity.ID, out Vector2 velocity);
				return velocity;
			}

			set => InternalCalls.RigidBody2DComponent_SetLinearVelocity(Entity.ID, ref value);
		}

		public float GravityScale
		{
			get => InternalCalls.RigidBody2DComponent_GetGravityScale(Entity.ID);
			set => InternalCalls.RigidBody2DComponent_SetGravityScale(Entity.ID, value);
		}

		public void ApplyLinearImpulse(Vector2 impulse, Vector2 offset, bool wake)
			=> InternalCalls.RigidBody2DComponent_ApplyLinearImpulse(Entity.ID, ref impulse, ref offset, wake);

		public void ApplyAngularImpulse(float impulse, bool wake)
			=> InternalCalls.RigidBody2DComponent_ApplyAngularImpulse(Entity.ID, impulse, wake);

		public void AddForce(Vector2 force, Vector2 offset, bool wake)
			=> InternalCalls.RigidBody2DComponent_AddForce(Entity.ID, ref force, ref offset, wake);

		public void AddTorque(float torque, bool wake)
			=> InternalCalls.RigidBody2DComponent_AddTorque(Entity.ID, torque, wake);
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
			get => InternalCalls.RigidBodyComponent_GetBodyType(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetBodyType(Entity.ID, value);
		}
		
		public float Mass
		{
			get => InternalCalls.RigidBodyComponent_GetMass(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetMass(Entity.ID, value);
		}

		public Vector3 LinearVelocity
		{
			get
			{
				InternalCalls.RigidBodyComponent_GetLinearVelocity(Entity.ID, out Vector3 velocity);
				return velocity;
			}

			set => InternalCalls.RigidBodyComponent_SetLinearVelocity(Entity.ID, ref value);
		}

		public Vector3 AngularVelocity
		{
			get
			{
				InternalCalls.RigidBodyComponent_GetAngularVelocity(Entity.ID, out Vector3 velocity);
				return velocity;
			}

			set => InternalCalls.RigidBodyComponent_SetAngularVelocity(Entity.ID, ref value);
		}

		public bool IsTrigger
		{
			get => InternalCalls.RigidBodyComponent_IsTrigger(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetTrigger(Entity.ID, value);
		}

		public float MaxLinearVelocity
		{
			get => InternalCalls.RigidBodyComponent_GetMaxLinearVelocity(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetMaxLinearVelocity(Entity.ID, value);
		}

		public float MaxAngularVelocity
		{
			get => InternalCalls.RigidBodyComponent_GetMaxAngularVelocity(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetMaxAngularVelocity(Entity.ID, value);
		}

		public float LinearDrag
		{
			get { return InternalCalls.RigidBodyComponent_GetLinearDrag(Entity.ID); }
			set { InternalCalls.RigidBodyComponent_SetLinearDrag(Entity.ID, value); }
		}

		public float AngularDrag
		{
			get => InternalCalls.RigidBodyComponent_GetAngularDrag(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetAngularDrag(Entity.ID, value);
		}

		public uint Layer
		{
			get => InternalCalls.RigidBodyComponent_GetLayer(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetLayer(Entity.ID, value);
		}

		public string LayerName
		{
			get => InternalCalls.RigidBodyComponent_GetLayerName(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetLayerByName(Entity.ID, value);
		}

		public bool IsSleeping
		{
			get => InternalCalls.RigidBodyComponent_IsSleeping(Entity.ID);
			set => InternalCalls.RigidBodyComponent_SetIsSleeping(Entity.ID, value);
		}

		public void MoveKinematic(Vector3 targetPosition, Vector3 targetRotation, float deltaSeconds)
			=> InternalCalls.RigidBodyComponent_MoveKinematic(Entity.ID, ref targetPosition, ref targetRotation, deltaSeconds);

		public void AddForce(Vector3 force, EForceMode forceMode = EForceMode.Force)
			=> InternalCalls.RigidBodyComponent_AddForce(Entity.ID, ref force, forceMode);

		public void AddForceAtLocation(Vector3 force, Vector3 location, EForceMode forceMode = EForceMode.Force)
		{
			InternalCalls.RigidBodyComponent_AddForceAtLocation(Entity.ID, ref force, ref location, forceMode);
		}

		public void AddTorque(Vector3 torque, EForceMode forceMode = EForceMode.Force)
			=> InternalCalls.RigidBodyComponent_AddTorque(Entity.ID, ref torque, forceMode);

		// Rotation should be in radians
		public void Rotate(Vector3 rotation)
			=> InternalCalls.RigidBodyComponent_Rotate(Entity.ID, ref rotation);

		public void SetAxisLock(EActorAxis axis, bool value, bool forceWake = false)
			=> InternalCalls.RigidBodyComponent_SetAxisLock(Entity.ID, axis, value, forceWake);
		public bool IsAxisLocked(EActorAxis axis)
			=> InternalCalls.RigidBodyComponent_IsAxisLocked(Entity.ID, axis);

		public uint GetLockedAxes() => InternalCalls.RigidBodyComponent_GetLockedAxes(Entity.ID);

		public void Teleport(Vector3 targetPosition, bool force = false)
		{
			Teleport(targetPosition, Entity.Rotation, force);
		}

		public void Teleport(Vector3 targetPosition, Vector3 targetRotation, bool force = false)
		{
			InternalCalls.RigidBodyComponent_Teleport(Entity.ID, ref targetPosition, ref targetRotation, force);
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
			get => InternalCalls.CharacterControllerComponent_GetIsGravityEnabled(Entity.ID);
			set => InternalCalls.CharacterControllerComponent_SetIsGravityEnabled(Entity.ID, value);
		}

		public float SlopeLimit
		{
			get => InternalCalls.CharacterControllerComponent_GetSlopeLimit(Entity.ID);
			set => InternalCalls.CharacterControllerComponent_SetSlopeLimit(Entity.ID, value);
		}

		public float StepOffset
		{
			get => InternalCalls.CharacterControllerComponent_GetStepOffset(Entity.ID);
			set => InternalCalls.CharacterControllerComponent_SetStepOffset(Entity.ID, value);
		}

		public Vector3 LinearVelocity
		{
			get
			{
				InternalCalls.CharacterControllerComponent_GetLinearVelocity(Entity.ID, out Vector3 velocity);
				return velocity;
			}

			set => InternalCalls.CharacterControllerComponent_SetLinearVelocity(Entity.ID, ref value);
		}
		public bool IsGrounded => InternalCalls.CharacterControllerComponent_IsGrounded(Entity.ID);
		public ECollisionFlags CollisionFlags => InternalCalls.CharacterControllerComponent_GetCollisionFlags(Entity.ID);

		public void SetTranslation(Vector3 translation) => InternalCalls.CharacterControllerComponent_SetTranslation(Entity.ID, ref translation);
		public void SetRotation(Quaternion rotation) => InternalCalls.CharacterControllerComponent_SetRotation(Entity.ID, ref rotation);

		public void Move(Vector3 displacement) => InternalCalls.CharacterControllerComponent_Move(Entity.ID, ref displacement);
		public void Jump(float jumpPower) => InternalCalls.CharacterControllerComponent_Jump(Entity.ID, jumpPower);
	}

	public class FixedJointComponent : Component
	{

		public Entity ConnectedEntity
		{
			get => new Entity(InternalCalls.FixedJointComponent_GetConnectedEntity(Entity.ID));
			set => InternalCalls.FixedJointComponent_SetConnectedEntity(Entity.ID, value.ID);
		}

		public bool IsBreakable
		{
			get => InternalCalls.FixedJointComponent_IsBreakable(Entity.ID);
			set => InternalCalls.FixedJointComponent_SetIsBreakable(Entity.ID, value);
		}

		public bool IsBroken => InternalCalls.FixedJointComponent_IsBroken(Entity.ID);

		public float BreakForce
		{
			get => InternalCalls.FixedJointComponent_GetBreakForce(Entity.ID);
			set => InternalCalls.FixedJointComponent_SetBreakForce(Entity.ID, value);
		}

		public float BreakTorque
		{
			get => InternalCalls.FixedJointComponent_GetBreakTorque(Entity.ID);
			set => InternalCalls.FixedJointComponent_SetBreakTorque(Entity.ID, value);
		}

		public bool IsCollisionEnabled
		{
			get => InternalCalls.FixedJointComponent_IsCollisionEnabled(Entity.ID);
			set => InternalCalls.FixedJointComponent_SetCollisionEnabled(Entity.ID, value);
		}

		public bool IsPreProcessingEnabled
		{
			get => InternalCalls.FixedJointComponent_IsPreProcessingEnabled(Entity.ID);
			set => InternalCalls.FixedJointComponent_SetPreProcessingEnabled(Entity.ID, value);
		}

		public void Break() => InternalCalls.FixedJointComponent_Break(Entity.ID);
	}

	public class BoxColliderComponent : Component
	{
		public Vector3 HalfSize
		{
			get
			{
				InternalCalls.BoxColliderComponent_GetHalfSize(Entity.ID, out Vector3 halfSize);
				return halfSize;
			}
		}

		public Vector3 Offset
		{
			get
			{
				InternalCalls.BoxColliderComponent_GetOffset(Entity.ID, out Vector3 offset);
				return offset;
			}
		}

		public PhysicsMaterial Material
		{
			get
			{
				InternalCalls.BoxColliderComponent_GetMaterial(Entity.ID, out PhysicsMaterial material);
				return material;
			}

			set => InternalCalls.BoxColliderComponent_SetMaterial(Entity.ID, ref value);
		}
	}

	public class SphereColliderComponent : Component
	{

		public float Radius => InternalCalls.SphereColliderComponent_GetRadius(Entity.ID);

		public Vector3 Offset
		{
			get
			{
				InternalCalls.SphereColliderComponent_GetOffset(Entity.ID, out Vector3 offset);
				return offset;
			}
		}

		public PhysicsMaterial Material
		{
			get
			{
				InternalCalls.SphereColliderComponent_GetMaterial(Entity.ID, out PhysicsMaterial material);
				return material;
			}

			set => InternalCalls.SphereColliderComponent_SetMaterial(Entity.ID, ref value);
		}
	}

	public class CapsuleColliderComponent : Component
	{

		public float Radius => InternalCalls.CapsuleColliderComponent_GetRadius(Entity.ID);
		public float HalfHeight => InternalCalls.CapsuleColliderComponent_GetHeight(Entity.ID);

		public Vector3 Offset
		{
			get
			{
				InternalCalls.CapsuleColliderComponent_GetOffset(Entity.ID, out Vector3 offset);
				return offset;
			}
		}

		public PhysicsMaterial Material
		{
			get
			{
				InternalCalls.CapsuleColliderComponent_GetMaterial(Entity.ID, out PhysicsMaterial material);
				return material;
			}

			set => InternalCalls.CapsuleColliderComponent_SetMaterial(Entity.ID, ref value);
		}
	}

	public class MeshColliderComponent : Component
	{
		public bool IsStaticMesh => InternalCalls.MeshColliderComponent_IsMeshStatic(Entity.ID);

		public AssetHandle ColliderMeshHandle
		{
			get => InternalCalls.MeshColliderComponent_GetColliderMesh(Entity.ID, out AssetHandle colliderHandle)
				? colliderHandle : AssetHandle.Invalid;
		}

		public PhysicsMaterial Material
		{
			get
			{
				InternalCalls.MeshColliderComponent_GetMaterial(Entity.ID, out PhysicsMaterial material);
				return material;
			}

			set => InternalCalls.MeshColliderComponent_SetMaterial(Entity.ID, ref value);
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
				m_ColliderMesh = new Mesh(ColliderMeshHandle);

			return m_ColliderMesh;
		}

		public StaticMesh GetStaticColliderMesh()
		{
			if (!IsStaticMesh)
				throw new InvalidOperationException("Tried to access a static Mesh in MeshColliderComponent when the collision mesh is a dynamic Mesh. Use MeshColliderComponent.GetColliderMesh() instead.");

			if (!ColliderMeshHandle.IsValid())
				return null;

			if (m_ColliderMesh == null || m_StaticColliderMesh.Handle != ColliderMeshHandle)
				m_StaticColliderMesh = new StaticMesh(ColliderMeshHandle);

			return m_StaticColliderMesh;
		}
	}

	public class AudioListenerComponent : Component
	{
	}

	public class AudioComponent : Component
	{
		public bool IsPlaying() => InternalCalls.AudioComponent_IsPlaying(Entity.ID);
		public bool Play(float startTime = 0.0f) => InternalCalls.AudioComponent_Play(Entity.ID, startTime);
		public bool Stop() => InternalCalls.AudioComponent_Stop(Entity.ID);
		public bool Pause() => InternalCalls.AudioComponent_Pause(Entity.ID);
		public bool Resume() => InternalCalls.AudioComponent_Resume(Entity.ID);

		public float VolumeMultiplier
		{
			get => InternalCalls.AudioComponent_GetVolumeMult(Entity.ID);
			set => InternalCalls.AudioComponent_SetVolumeMult(Entity.ID, value);
		}

		public float PitchMultiplier
		{
			get => InternalCalls.AudioComponent_GetPitchMult(Entity.ID);
			set => InternalCalls.AudioComponent_SetPitchMult(Entity.ID, value);
		}

		public void SetEvent(AudioCommandID eventID) => InternalCalls.AudioComponent_SetEvent(Entity.ID, eventID);
		public void SetEvent(string eventName) => InternalCalls.AudioComponent_SetEvent(Entity.ID, new AudioCommandID(eventName));
	}

	public class TextComponent : Component
	{
		private string m_Text;
		private ulong m_Hash = 0;

		public string Text
		{
			get
			{
				ulong internalHash = InternalCalls.TextComponent_GetHash(Entity.ID);
				if (m_Hash != internalHash)
				{
					m_Text = InternalCalls.TextComponent_GetText(Entity.ID);
					m_Hash = internalHash;
				}

				return m_Text;
			}

			set => InternalCalls.TextComponent_SetText(Entity.ID, value);
		}

		public Vector4 Color
		{
			get
			{
				InternalCalls.TextComponent_GetColor(Entity.ID, out Vector4 color);
				return color;
			}

			set => InternalCalls.TextComponent_SetColor(Entity.ID, ref value);
		}
	}

}
