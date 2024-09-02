using Hazel;
using System;

// These scripts are intended to be used on a "third person" character, set up as follows:
// 
// "Player" entity:
//    - Transform component                  <-- the position, rotation, and scale of the player character
//    - Animation component                  <-- the animation graph for the player character
//    - Character controller component       <-- a physics "actor" governing interaction between the player character and the physics of the game world
//    - Capsule collider component           <-- a physics collider approximating the players shape
//    - Script component: PlayerController   <-- Contains game logic for moving the player character
//       - holds a reference to the Camera entity
//    - Child entities:
//       - Mesh components                   <-- The actual model (aka "mesh(es)") that make up the player character
//
// "Camera" entity:
//    - Transform component                  <-- the position, rotation and scale of the camera
//    - Script component:                    <-- Contains game logic for moving the camera
//       - holds a reference to the Player entity
//
// The player and camera controllers work in concert to provide the following functionality:
// PlayerController (and PlayerControllerAnimated):
//    - WASD keys move the player forwards, left, right, backwards relative to the direction the camera is facing.
//    - Left shift key to run
//    - Spacebar to jump
//
// OverShoulderCamera:
//    - Camera moves with, and orbits around, the player character
//    - Mouse movement (when right button held) changes the pitch and yaw of the camera
//    - Parameters control the distance from the player, the height of the camera, and the pitch and yaw offsets

namespace ThirdPersonExample
{
	public class OverShoulderCamera : Entity
	{
		public Entity? Player;
		public float DistanceFromPlayer = 3.5f;
		public float OrbitVerticalOffset = 2.0f;
		public float YawOffsetDegrees = 25.0f;
		public float MinPitchDegrees = -27.0f;
		public float MaxPitchDegrees = 30.0f;
		public float MouseSensitivity = 1.0f;

		private Vector2 m_LastMousePosition;
		private Vector3 LocalRotation = Vector3.Zero;
		private Vector3 LocalTranslation = Vector3.Zero;
		private CharacterControllerComponent? m_CharacterController;

		protected override void OnCreate()
		{
			m_CharacterController = GetComponent<CharacterControllerComponent>();
		}

		protected override void OnUpdate(float ts)
		{
			if (Player is null)
			{
				return;
			}

			Vector2 currentMousePosition = Input.GetMousePosition();

			// sync m_Transform to the current camera rotation
			// (this allows us to change it in editor during runtime)
			LocalRotation = Rotation;

			// Rotate camera (only if the right mouse button is down)
			if (Input.IsMouseButtonDown(MouseButton.Right))
			{
				Vector2 delta = currentMousePosition - m_LastMousePosition;
				LocalRotation.Y = ((Mathf.PI + LocalRotation.Y - delta.X * MouseSensitivity / 10.0f * ts) % Mathf.TwoPI) - Mathf.PI; // ensure Rotation.Y is in range -PI to PI
				LocalRotation.X = Mathf.Clamp(LocalRotation.X - delta.Y * MouseSensitivity / 10.0f * ts, Mathf.Clamp(MinPitchDegrees, -85, 85) * Mathf.Deg2Rad, Mathf.Clamp(MaxPitchDegrees, -85.0f, 85.0f) * Mathf.Deg2Rad);
			}

			float yawOffsetRadians = YawOffsetDegrees * Mathf.Deg2Rad;
			float rFactor = DistanceFromPlayer * Mathf.Cos(LocalRotation.X);
			LocalTranslation.X = Mathf.Sin(LocalRotation.Y + yawOffsetRadians) * rFactor;
			LocalTranslation.Y = OrbitVerticalOffset - (DistanceFromPlayer * Mathf.Sin(LocalRotation.X));
			LocalTranslation.Z = Mathf.Cos(LocalRotation.Y + yawOffsetRadians) * rFactor;

			m_CharacterController?.Move(Player.Transform.WorldTransform.Position + LocalTranslation - Translation);
			m_CharacterController?.Rotate(RotationQuat.Conjugate * new Quaternion(LocalRotation));

			m_LastMousePosition = currentMousePosition;
		}
	}


	public class PlayerController : Entity
	{
		public Entity? Camera;
		public float WalkSpeed = 1.5f;
		public float RunSpeed = 3.0f;
		public float RotationSpeed = 10.0f;
		public float JumpPower = 3.0f;

		private CharacterControllerComponent? m_CharacterController;

		protected override void OnCreate()
		{
			m_CharacterController = GetComponent<CharacterControllerComponent>();

			if (Camera is null)
			{
				Log.Error("Camera entity is not set!");
			}

			if (m_CharacterController is null)
			{
				Log.Error("Could not find Player model's transform.");
			}
		}

		protected override void OnUpdate(float ts)
		{
			if (Camera is null || m_CharacterController is null)
			{
				return;
			}
			float X = 0.0f;
			float Z = 0.0f;
			bool keyDown = false;

			Vector3 desiredRotation = Rotation;

			if (Input.IsKeyDown(KeyCode.W))
			{
				Z = 1.0f;
				keyDown = true;
				desiredRotation.Y = Camera.Rotation.Y - Mathf.PI;
			}
			else if (Input.IsKeyDown(KeyCode.S))
			{
				Z = -1.0f;
				keyDown = true;
				desiredRotation.Y = Camera.Rotation.Y;
			}

			if (Input.IsKeyDown(KeyCode.A))
			{
				X = -1.0f;
				keyDown = true;
				desiredRotation.Y = Camera.Rotation.Y - (Mathf.PI / 2.0f);
			}
			else if (Input.IsKeyDown(KeyCode.D))
			{
				X = 1.0f;
				keyDown = true;
				desiredRotation.Y = Camera.Rotation.Y + (Mathf.PI / 2.0f);
			}

			bool isGrounded = m_CharacterController.IsGrounded;
			if (keyDown)
			{
				// rotate to face camera direction
				Vector3 currentRotation = Rotation;

				float diff1 = Mathf.Modulo(desiredRotation.Y - currentRotation.Y, Mathf.TwoPI);
				float diff2 = Mathf.Modulo(currentRotation.Y - desiredRotation.Y, Mathf.TwoPI);
				bool useDiff2 = diff2 < diff1;

				float delta = useDiff2 ? Mathf.Max(-diff2, -Mathf.Abs(RotationSpeed * ts)) : Mathf.Min(diff1, Mathf.Abs(RotationSpeed * ts));

				currentRotation.Y += delta;
				m_CharacterController.SetRotation(new Quaternion(currentRotation));

				if (Math.Abs(delta) < 0.001f)
				{
					Vector3 movement = (Camera.Transform.LocalTransform.Right * X) + (Camera.Transform.LocalTransform.Forward * Z);
					movement.Normalize();
					bool shiftDown = Input.IsKeyDown(KeyCode.LeftShift);
					float speed = shiftDown ? RunSpeed : WalkSpeed;
					movement *= speed * ts;

					// Note: Properties on the character controller can be used to control whether or movement will be applied
					// if the character is not grounded
					m_CharacterController.Move(movement);
				}
			}

			if (isGrounded && Input.IsKeyDown(KeyCode.Space))
			{
				m_CharacterController.Jump(JumpPower);
			}
		}
	}


	public class PlayerControllerAnimated : Entity
	{
		public Entity? Camera;
		public float JumpPower = 3.0f;
		public float RotationSpeed = 10.0f;
		public Entity? HandSlot;
		public Prefab? InteractPrefab;
		public Prefab? BulletPrefab;
		public int Ammo = 0;

		private AnimationComponent? m_Animation;
		private CharacterControllerComponent? m_CharacterController;

		private static Identifier ForwardsInputID = new Identifier("Forwards");
		private static Identifier IsGroundedInputID = new Identifier("IsGrounded");
		private static Identifier IsJumpingInputID = new Identifier("IsJumping");
		private static Identifier IsShootingInputID = new Identifier("IsShooting");

		private static Identifier JumpedEventID = new Identifier("Jumped");
		private static Identifier InteractedEventID = new Identifier("Interacted");
		private static Identifier ShootEventID = new Identifier("Shoot");

		private Identifier m_InteractTriggerID;
		private Entity? m_InteractEntity;
		private Entity? m_InteractionTextEntity;

		protected override void OnCreate()
		{
			m_Animation = GetComponent<AnimationComponent>();

			m_CharacterController = GetComponent<CharacterControllerComponent>();

			if (Camera is null)
			{
				Log.Error("Camera entity is not set!");
			}

			if (m_Animation is null)
			{
				Log.Error("Could not find animation component.");
			}

			if (m_CharacterController is null)
			{
				Log.Error("Could not find character controller component.");
			}

			AnimationEvent += OnAnimationEvent;

		}

		protected override void OnUpdate(float ts)
		{
			if (Camera is null || m_Animation is null || m_CharacterController is null)
			{
				return;
			}
			float forwards = 0;
			bool isKeyDown = false;
			bool isShiftDown = false;
			bool isShooting = false;

			Vector3 desiredRotation = Rotation;

			if (Input.IsKeyDown(KeyCode.W))
			{
				isKeyDown = true;
				desiredRotation.Y = Camera.Rotation.Y - Mathf.PI;
			}
			else if (Input.IsKeyDown(KeyCode.S))
			{
				isKeyDown = true;
				desiredRotation.Y = Camera.Rotation.Y;
			}

			if (Input.IsKeyDown(KeyCode.A))
			{
				isKeyDown = true;
				desiredRotation.Y = Camera.Rotation.Y - (Mathf.PI / 2.0f);
			}
			else if (Input.IsKeyDown(KeyCode.D))
			{
				isKeyDown = true;
				desiredRotation.Y = Camera.Rotation.Y + (Mathf.PI / 2.0f);
			}

			if (isKeyDown)
			{
				isShiftDown = Input.IsKeyDown(KeyCode.LeftShift);
				forwards = isShiftDown ? 2.0f : 1.0f;
			}

			if ((forwards != 0.0f) && Mathf.Abs(Rotation.Y - desiredRotation.Y) > 0.0001f)
			{
				// rotate to face camera direction first, then move
				Vector3 currentRotation = Rotation;

				float diff1 = Mathf.Modulo(desiredRotation.Y - currentRotation.Y, Mathf.TwoPI);
				float diff2 = Mathf.Modulo(currentRotation.Y - desiredRotation.Y, Mathf.TwoPI);
				bool useDiff2 = diff2 < diff1;

				float delta = useDiff2 ? Mathf.Max(-diff2, -Mathf.Abs(RotationSpeed * ts)) : Mathf.Min(diff1, Mathf.Abs(RotationSpeed * ts));

				currentRotation.Y += delta;

				// Note: Rotate() here rather than SetRotation() as the latter "teleports" the character to the new rotation
				// Rotate() on the other hand, moves the character with "physics" (so will, for example, pay attention to whether or not the character is grounded)
				m_CharacterController.Rotate(RotationQuat.Conjugate * new Quaternion(currentRotation));
			}

			if (m_CharacterController.IsGrounded)
			{
				if (Input.IsKeyPressed(KeyCode.Space))
				{
					// Note: When we get an event back from the animation graph that the jump start animation is complete, then
					// we tell the character controller to apply a jump force.
					m_Animation.SetInputBool(IsJumpingInputID, true);
				}

				// If the player presses "E" and there there is an entity to interact with, then trigger interaction animation
				// (we get an event when the animation is done, and that's when we actually interact with the entity)
				if(Input.IsKeyPressed(KeyCode.E) && (m_InteractEntity is not null))
				{
					m_Animation.SetInputTrigger(m_InteractTriggerID);
				}

				// If the player presses "R" and there is an item in the HandSlot, then drop the item
				if (Input.IsKeyPressed(KeyCode.R) && (HandSlot is not null) && (HandSlot.Children.Length > 0))
				{
					Entity item = HandSlot.Children[0];
					RigidBodyComponent? rb = item.GetComponent<RigidBodyComponent>();
					if (rb is not null)
					{
						rb.BodyType = EBodyType.Dynamic;
					}
					item.Parent = null;
					if (InteractPrefab is not null)
					{
						item.InstantiateChild(InteractPrefab);
					}
				}

				// While the player is holding down left mouse button and there is a pistol in the HandSlot, then fire the pistol
				if (Input.IsMouseButtonDown(MouseButton.Left) && (HandSlot is not null) && (HandSlot.Children.Length > 0) && (HandSlot.Children[0].Tag == "Pistol") && BulletPrefab is not null)
				{
					isShooting = true;
				}
			}

			m_Animation.SetInputFloat(ForwardsInputID, forwards);
			m_Animation.SetInputBool(IsGroundedInputID, m_CharacterController.IsGrounded);
			m_Animation.SetInputBool(IsShootingInputID, isShooting);

			if(m_InteractionTextEntity is not null)
			{
				if (Camera is not null)
				{
					m_InteractionTextEntity.Transform.Rotation = Camera.Transform.WorldTransform.Rotation;
				}
			}
		}


		void OnAnimationEvent(Identifier eventID)
		{
			if(eventID == JumpedEventID)
			{
				m_CharacterController?.Jump(JumpPower);
				m_Animation?.SetInputBool(IsJumpingInputID, false);
				return;
			}

			if (eventID == InteractedEventID)
			{
				// "interact" with the entity.
				// In this case, pick it up
				// Note that m_InteractEntity is the interaction trigger.
				// The parent of m_InteractEntity is the actual entity we are interested in interacting with.
				if (m_InteractEntity is not null && m_InteractEntity.Parent is not null)
				{
					Entity item = m_InteractEntity.Parent;

					// Change parent's rigid body from dynamic to kinematic
					// (since the player is now carrying that entity, its movement is controlled by player movement
					// rather than by physics);
					RigidBodyComponent? rb = item.GetComponent<RigidBodyComponent>();
					if (rb is not null)
					{
						rb.BodyType = EBodyType.Kinematic;
					}

					// Take appropriate action depending on which entity we are interacting with
					if (item.Tag == "Pistol")
					{
						// Pickup the pistol by parenting it to HandSlot
						item.Parent = HandSlot;
						item.Translation = new Vector3(0.0f, 0.1f, 0.0f);
						item.Rotation = new Vector3(0.0f, 90.0f * Mathf.Deg2Rad, 0.0f);
					}
					else if(item.Tag == "Ammo Box")
					{
						// Add to ammo count, and destroy the ammo box.
						item.Destroy();
						Ammo += 12;
					}
					else
					{
						Log.Info("Unrecognized entity type for interaction!");
					}

					m_InteractEntity.Destroy();
				}

				DisableInteraction();
				return;
			}

			if(eventID == ShootEventID)
			{
				if (Ammo > 0)
				{
					--Ammo;
					Entity weapon = HandSlot!.Children[0];
					Transform transform = new();
					foreach (Entity child in weapon.Children)
					{
						if (child.Tag == "Muzzle")
						{
							transform = child.Transform.WorldTransform;
							break;
						}
					}

					RigidBodyComponent? rb = Scene.InstantiatePrefab(BulletPrefab!, transform)?.GetComponent<RigidBodyComponent>();
					if (rb is not null)
					{
						rb.AngularVelocity = Vector3.Zero;
						rb.LinearVelocity = transform.Forward * -40.0f;  // Note: Hazel's "Forward" vector is -Z, but all our assets face forwards in +Z
					}
				}
				return;
			}

			Log.Info("Unrecognized animation event received!");
		}


		public void EnableInteraction(Identifier triggerID, Entity other)
		{
			if(m_InteractEntity is not null)
			{
				return;
			}

			m_InteractTriggerID = triggerID;
			m_InteractEntity = other;
			Transform transform = new Transform();
			transform.Position = other.Transform.WorldTransform.Position + new Vector3(0, 0.3f, 0);
			if (Camera is not null)
			{
				transform.Rotation = Camera.Transform.WorldTransform.Rotation;
			}
			transform.Scale = new Vector3(0.1f, 0.1f, 0.1f);

			m_InteractionTextEntity = Scene.CreateEntity("InteractionText");
			m_InteractionTextEntity.Translation = transform.Position;
			m_InteractionTextEntity.Rotation = transform.Rotation;
			m_InteractionTextEntity.Scale = transform.Scale;
			TextComponent? tc = m_InteractionTextEntity.CreateComponent<TextComponent>();
			if (tc is not null)
			{
				// Here, you could check which entity m_InteractEntity is, and set the text accordingly
				tc.Text = "E - Pickup";
			}
		}


		public void DisableInteraction()
		{
			m_InteractTriggerID = Identifier.Invalid;
			m_InteractEntity = null;
			m_InteractionTextEntity?.Destroy();
			m_InteractionTextEntity = null;
		}
	}


	class Trigger : Entity
	{
		public Entity? Player;
		public String TriggerTag = "Interact";  // This is the name of the trigger that is sent to the player animation graph if/when the player presses "E"

		private Identifier m_TriggerID;

		protected override void OnCreate()
		{
			m_TriggerID = new Identifier(TriggerTag);

			TriggerBeginEvent += OnTriggerBegin;
			TriggerEndEvent += OnTriggerEnd;
		}

		void OnTriggerBegin(Entity other)
		{
			Player?.As<PlayerControllerAnimated>()?.EnableInteraction(m_TriggerID, other);
		}

		void OnTriggerEnd(Entity other)
		{
			Player?.As<PlayerControllerAnimated>()?.DisableInteraction();
		}
	}
}
