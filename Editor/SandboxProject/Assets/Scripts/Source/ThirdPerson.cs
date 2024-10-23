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
// Player Controller (and PlayerControllerAnimated):
//    - WASD keys move the player forwards, left, right, backwards relative to the direction the camera is facing.
//    - Left shift key to run
//    - Spacebar to jump
//
// Camera Controller:
//    - Mouse changes direction that the camera is facing.
//    - Escape key: stops camera movement so that you can (for example) fiddle around with settings in Editor.
//    - F5 key: resumes camera movement
//
namespace ThirdPersonExample
{
	public class CameraController : Entity
	{
		public Entity Player;
		public float DistanceFromPlayer = 8.0f;
		public float MouseSensitivity = 1.0f;

		private Vector2 m_LastMousePosition;

		private bool m_CameraMovementEnabled = true;
		private Transform m_Transform = new Transform(new Vector3(0.0f, 2.0f, 8.0f), new Vector3(177.8f * Mathf.Deg2Rad, 0.0f, 180.0f * Mathf.Deg2Rad), Vector3.One);

		protected override void OnUpdate(float ts)
		{
			if (Player == null)
			{
				return;
			}

			if (Input.IsKeyPressed(KeyCode.Escape) && m_CameraMovementEnabled)
			{
				m_CameraMovementEnabled = false;
			}

			if (Input.IsKeyPressed(KeyCode.F5) && !m_CameraMovementEnabled)
			{
				m_CameraMovementEnabled = true;
			}

			Vector2 currentMousePosition = Input.GetMousePosition();
			if (m_CameraMovementEnabled)
			{
				Vector2 delta = m_LastMousePosition - currentMousePosition;
				m_Transform.Rotation.Y = (m_Transform.Rotation.Y - delta.X * MouseSensitivity * ts) % Mathf.TwoPI;

				m_Transform.Position.X = DistanceFromPlayer * Mathf.Sin(m_Transform.Rotation.Y);
				m_Transform.Position.Z = DistanceFromPlayer * -Mathf.Cos(m_Transform.Rotation.Y);
			}

			//Hazel.Transform transform = Player.Transform.WorldTransform * m_Transform;
			Transform.Translation = m_Transform.Position + Player.Transform.WorldTransform.Position;
			Transform.Rotation = m_Transform.Rotation;
			Transform.Scale = m_Transform.Scale;

			m_LastMousePosition = currentMousePosition;

		}
	}


	public class PlayerController : Entity
	{
		public Entity Camera;
		public float WalkSpeed = 1.5f;
		public float RunSpeed = 3.0f;
		public float RotationSpeed = 10.0f;
		public float JumpPower = 3.0f;

		private CharacterControllerComponent m_CharacterController;
		private TransformComponent m_CameraTransform;
		private Vector3 m_LastMovement;

		protected override void OnCreate()
		{
			m_CharacterController = GetComponent<CharacterControllerComponent>();

			if (Camera == null)
			{
				Log.Error("Camera entity is not set!");
			}
			else
			{
				m_CameraTransform = Camera.GetComponent<TransformComponent>();
			}

			if (m_CameraTransform == null)
			{
				Log.Error("Could not find camera transform.");
			}

			if (m_CharacterController == null)
			{
				Log.Error("Could not find Player model's transform.");
			}
		}

		protected override void OnUpdate(float ts)
		{
			if (m_CameraTransform == null || m_CharacterController == null)
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
				desiredRotation.Y = -m_CameraTransform.Rotation.Y;
			}
			else if (Input.IsKeyDown(KeyCode.S))
			{
				Z = -1.0f;
				keyDown = true;
				desiredRotation.Y = -m_CameraTransform.Rotation.Y + Mathf.PI;
			}

			if (Input.IsKeyDown(KeyCode.A))
			{
				X = -1.0f;
				keyDown = true;
				desiredRotation.Y = -m_CameraTransform.Rotation.Y + (Mathf.PI / 2.0f);
			}
			else if (Input.IsKeyDown(KeyCode.D))
			{
				X = 1.0f;
				keyDown = true;
				desiredRotation.Y = -m_CameraTransform.Rotation.Y - (Mathf.PI / 2.0f);
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
					Vector3 movement = (m_CameraTransform.LocalTransform.Right * X) + (m_CameraTransform.LocalTransform.Forward * Z);
					movement.Normalize();
					bool shiftDown = Input.IsKeyDown(KeyCode.LeftShift);
					float speed = shiftDown ? RunSpeed : WalkSpeed;
					movement *= speed * ts;

					// We could simply move the character according to player's key presses and leave it at that.
					// Like this:
					//    m_CharacterController.Move(movement);
					//
					// However, that means the player can still control movement of the character even if it is
					// in mid-air.
					//
					// If you don't want the character to be able to change direction mid-air, then you
					// can use the IsGrounded property to check whether or not to move player, like the following code:
					if (isGrounded)
					{
						m_CharacterController.Move(movement);
						m_LastMovement = movement / ts;
					}
					else
					{
						// If not grounded, carry on moving per last grounded movement.
						// This gives the effect of momentum, rather than just falling straight down.
						// Note adjustment by ts is in case frame times are not exactly the same.
						m_CharacterController.Move(m_LastMovement * ts);
					}
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
		public Entity Camera;
		public float JumpPower = 3.0f; 
		public float RotationSpeed = 10.0f;

		private AnimationComponent m_Animation;
		private CharacterControllerComponent m_CharacterController;
		private TransformComponent m_CameraTransform;
		private Vector3 m_LastMovement = Vector3.Zero;
		private static Identifier ForwardsStateInputID = new Identifier("ForwardsState");
		private static Identifier IsGroundedInputID = new Identifier("IsGrounded");

		private static Identifier WalkingEventID = new Identifier("Walking");
		private static Identifier RunningEventID = new Identifier("Running");
		private static Identifier FallingEventID = new Identifier("Falling");

		protected override void OnCreate()
		{
			m_Animation = GetComponent<AnimationComponent>();

			m_CharacterController = GetComponent<CharacterControllerComponent>();

			if (Camera == null)
			{
				Log.Error("Camera entity is not set!");
			}
			else
			{
				m_CameraTransform = Camera.GetComponent<TransformComponent>();
			}

			if (m_CameraTransform == null)
			{
				Log.Error("Could not find camera transform.");
			}

			if (m_Animation == null)
			{
				Log.Error("Could not find animation component.");
			}

			if (m_CharacterController == null)
			{
				Log.Error("Could not find character controller component.");
			}

			AnimationEvent += OnAnimationEvent;

		}

		protected override void OnUpdate(float ts)
		{
			if (m_CameraTransform == null || m_Animation == null || m_CharacterController == null)
			{
				return;
			}
			int forwardsState = 0;
			bool isKeyDown = false;
			bool isShiftDown = false;

			Vector3 desiredRotation = Rotation;

			if (Input.IsKeyDown(KeyCode.W))
			{
				isKeyDown = true;
				desiredRotation.Y = -m_CameraTransform.Rotation.Y;
			}
			else if (Input.IsKeyDown(KeyCode.S))
			{
				isKeyDown = true;
				desiredRotation.Y = -m_CameraTransform.Rotation.Y + Mathf.PI;
			}

			if (Input.IsKeyDown(KeyCode.A))
			{
				isKeyDown = true;
				desiredRotation.Y = -m_CameraTransform.Rotation.Y + (Mathf.PI / 2.0f);
			}
			else if (Input.IsKeyDown(KeyCode.D))
			{
				isKeyDown = true;
				desiredRotation.Y = -m_CameraTransform.Rotation.Y - (Mathf.PI / 2.0f);
			}

			if (isKeyDown)
			{
				isShiftDown = Input.IsKeyDown(KeyCode.LeftShift);
				forwardsState = isShiftDown ? 2 : 1;
			}

			if ((forwardsState != 0) && Mathf.Abs(Rotation.Y - desiredRotation.Y) > 0.0001)
			{
				// rotate to face camera direction first, then move
				Vector3 currentRotation = Rotation;

				float diff1 = Mathf.Modulo(desiredRotation.Y - currentRotation.Y, Mathf.TwoPI);
				float diff2 = Mathf.Modulo(currentRotation.Y - desiredRotation.Y, Mathf.TwoPI);
				bool useDiff2 = diff2 < diff1;

				float delta = useDiff2 ? Mathf.Max(-diff2, -Mathf.Abs(RotationSpeed * ts)) : Mathf.Min(diff1, Mathf.Abs(RotationSpeed * ts));

				currentRotation.Y += delta;
				m_CharacterController.SetRotation(new Quaternion(currentRotation));
			}

			if (m_CharacterController.IsGrounded)
			{
				if (Input.IsKeyDown(KeyCode.Space))
				{
					m_CharacterController.Jump(JumpPower);
				}

				// Store the animation's current root motion as "movement.
				// If the character leaves the ground, we will apply that movement to give the illusion of momentum.
				// (as opposed to instantly falling straight down, which would look weird)
				// The thing is, the animation root motion is in coordinate system relative to the model entity.
				// We need to transform to coordinate system of the player entity (which is the entity we wish to move).
				//
				// Strictly speaking, it should be something like this:   m_LastMovement = Matrix3(Inverse(ParentEntity.WorldTransform) * Matrix3(ModelEntity.WorldTransform) * m_Animation.RootMotion.Position
				// But we do not yet have:
				//    construction of Matrix4 from a Transform
				//    Inverse function for Matrix4
				//    A Matrix3 class
				//    Multiplication of Vector3 by Matrix3
				//
				// So, in the meantime, this shortcut will have to do:
				// 1) Assume that there is no Parent (i.e. player entity is at top level of scene)
				// 2) Assume that the model is not scaled (i.e. so the only thing we need to account for is rotation of the model entity)
				// If 1) and 2) hold, then we have enough stuff implemented to be able to do the transformation
				Quaternion quat = new Quaternion(Rotation);
				m_LastMovement = quat * m_Animation.RootMotion.Position / ts;
			}
			else
			{
				// If not grounded, carry on moving per last grounded movement.
				// This gives the effect of momentum, rather than just falling straight down.
				// Note adjustment by ts is in case frame times are not exactly the same.
				m_CharacterController.Move(m_LastMovement * ts);
			}

			m_Animation.SetInputInt(ForwardsStateInputID, forwardsState);
			m_Animation.SetInputBool(IsGroundedInputID, m_CharacterController.IsGrounded);
		}


		// Example handler for events received from animation graph
		void OnAnimationEvent(Identifier eventID)
		{
			if (eventID == WalkingEventID)
			{
				Log.Info("Walking");
			} else if (eventID == RunningEventID)
			{
				Log.Info("Running");
			}
			else if (eventID == FallingEventID)
			{
				Log.Info("Falling");
			}
			else
			{
				Log.Info("Unrecognized animation event received!");
			}
		}

	}


	class PhysicsTestTrigger : Entity
	{
		private SkyLightComponent m_Lights;

		protected override void OnCreate()
		{
			Entity skylight = Scene.FindEntityByTag("Sky Light");
			if (skylight != null)
			{
				m_Lights = skylight.GetComponent<SkyLightComponent>();
			}
			else
			{
				Log.Error("'Sky Light' entity was not found, or it did not have a SkyLightComponent!");
			}

			TriggerBeginEvent += OnTriggerBegin;
			TriggerEndEvent += OnTriggerEnd;
		}

		void OnTriggerBegin(Entity other)
		{
			Log.Trace("Trigger begin");
			m_Lights.Intensity = 0.0f;
		}

		void OnTriggerEnd(Entity other)
		{
			Log.Trace("Trigger end");
			m_Lights.Intensity = 1.0f;
		}

	}
}
