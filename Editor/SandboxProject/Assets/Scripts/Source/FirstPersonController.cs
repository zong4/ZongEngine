using Hazel;
using System;

namespace Sandbox
{
	public class FirstPersonController : Entity
	{

		public float MovementSpeed = 3.0f;
		public float GamepadSensitivity = 3.0f;
		public float MouseSensitivity = 0.1f;
		public float JumpPower = 5.0f;
		public float ShootingCooldown = 0.15f;
		public float FootstepVolume = 0.22f;

		public Prefab ProjectilePrefab;

		[ShowInEditor("Camera")]
		private Entity m_CameraEntity;
		private TransformComponent m_CameraTransform;

		private CharacterControllerComponent m_CharacterController;

		private Vector2 m_MovementInput;
		private Vector2 m_RotationInput;

		private Vector2 m_LastMousePosition;

		private int m_GamepadID = -1;

		private Quaternion m_CurrentRotation = new Quaternion(Vector3.Zero, 1.0f);

		private Vector3 m_LastMovement;

		private float m_ShootingCooldownTimer = 0.0f;
		private readonly AudioCommandID m_PlayShotSoundGraphSoundID = new AudioCommandID("play_soundgraph");
		private readonly AudioCommandID m_PlayShotSoundID = new AudioCommandID("play_nonsoundgraph");

		private Vector3 m_PreviousPosition;
		private float m_DistanceTraveled = 0.0f;
		private float m_FootstepLength = 1.4f;
		private readonly AudioCommandID m_PlayFootstepID = new AudioCommandID("play_footstep");

		protected override void OnCreate()
		{
			DetectGamepad();
			SetupCharacterController();

			Input.SetCursorMode(CursorMode.Locked);

			m_CameraTransform = m_CameraEntity.GetComponent<TransformComponent>();

			m_PreviousPosition = Translation;
		}

		private void DetectGamepad()
		{
			// NOTE(Peter): Very hacky way to ensure we use a PlayStation or Xbox controller (otherwise it'd pick up my x56 H.O.T.A.S as the first controller)
			foreach (var controllerID in Input.GetConnectedControllerIDs())
			{
				var controllerName = Input.GetControllerName(controllerID);

				if (controllerName.Contains("Xbox") || controllerName.Contains("Sony"))
				{
					m_GamepadID = controllerID;
					break;
				}
			}
		}

		private void SetupCharacterController()
		{
			if (!HasComponent<CharacterControllerComponent>())
			{
				m_CharacterController = CreateComponent<CharacterControllerComponent>();
			}
			else
			{
				m_CharacterController = GetComponent<CharacterControllerComponent>();
			}
		}

		protected override void OnUpdate(float ts)
		{
			if (Input.IsKeyDown(KeyCode.Escape))
				Input.SetCursorMode(CursorMode.Normal);

			if (Input.IsMouseButtonPressed(MouseButton.Left))
				Input.SetCursorMode(CursorMode.Locked);

			UpdateInput();
			UpdateRotation(ts);
			UpdateMovement(ts);
			UpdateShooting(ts);

			// NOTE(Peter): Footsteps seem to break the audio engine currently. I'll have Jay look into this
			//UpdateFootstepSounds();
		}

		private void UpdateInput()
		{
			m_MovementInput.X = SandboxUtility.GetHorizontalMovement(m_GamepadID);
			m_MovementInput.Y = SandboxUtility.GetVerticalMovement(m_GamepadID);

			// Handle gamepad rotation
			m_RotationInput.X = SandboxUtility.GetGamepadAxisDelta(m_GamepadID, 2) * GamepadSensitivity;
			m_RotationInput.Y = SandboxUtility.GetGamepadAxisDelta(m_GamepadID, 3) * GamepadSensitivity;

			// Handle mouse rotation
			var mousePosition = Input.GetMousePosition();
			var mouseDelta = (m_LastMousePosition - mousePosition) * MouseSensitivity;

			if (mouseDelta.X != 0)
				m_RotationInput.X = mouseDelta.X;

			if (mouseDelta.Y != 0)
				m_RotationInput.Y = mouseDelta.Y;

			m_LastMousePosition = mousePosition;
		}

		private void UpdateRotation(float ts)
		{
			// Rotate camera
			var newRotation = m_CameraTransform.Rotation + Vector3.Right * m_RotationInput.Y * ts;
			newRotation.X = Mathf.Clamp(newRotation.X, -1.57079633f, 1.57079633f);
			m_CameraTransform.Rotation = newRotation;

			// Rotate Character Controller
			m_CurrentRotation = new Quaternion(Vector3.Up * m_RotationInput.X * ts) * m_CurrentRotation;
			m_CharacterController.SetRotation(m_CurrentRotation);
		}

		private void UpdateMovement(float ts)
		{
			if (m_CharacterController.IsGrounded)
			{
				var displacement = Transform.LocalTransform.Right * m_MovementInput.X + Transform.LocalTransform.Forward * m_MovementInput.Y;
				displacement.Normalize();

				var movement = displacement * MovementSpeed * ts;
				m_CharacterController.Move(movement);
				m_LastMovement = movement / ts;

				if (Input.IsKeyPressed(KeyCode.Space) || Input.IsControllerButtonPressed(m_GamepadID, GamepadButton.A))
				{
					m_CharacterController.Jump(JumpPower);
				}
			}
			else
			{
				m_CharacterController.Move(m_LastMovement * ts);
			}
		}

		private void UpdateShooting(float ts)
		{
			m_ShootingCooldownTimer -= ts;
			bool trigger = SandboxUtility.GetControllerAxis(m_GamepadID, 5) > 0.0f;

			if ((trigger || Input.IsMouseButtonPressed(MouseButton.Left)) && m_ShootingCooldownTimer < 0.0f)
			{
				m_ShootingCooldownTimer = ShootingCooldown;
				FireProjectile();
				Audio.PostEventAtLocation(m_PlayShotSoundGraphSoundID, Transform.WorldTransform.Position, Transform.WorldTransform.Rotation);
			}
			else if (Input.IsMouseButtonPressed(MouseButton.Right) && m_ShootingCooldownTimer < 0.0f)
			{
				m_ShootingCooldownTimer = ShootingCooldown;
				FireProjectile();
				Audio.PostEventAtLocation(m_PlayShotSoundID, Transform.WorldTransform.Position, Transform.WorldTransform.Rotation);
			}
		}

		private void FireProjectile()
		{
			if (!ProjectilePrefab)
				return;

			Entity projectile = Scene.InstantiatePrefab(ProjectilePrefab, m_CameraTransform.WorldTransform.Position + m_CameraTransform.WorldTransform.Forward);
			var rb = projectile.GetComponent<RigidBodyComponent>();
			projectile.Parent = Scene.FindEntityByTag("ProjectileHolder");

			Vector3 force = m_CameraTransform.WorldTransform.Forward * 20.0f;
			rb.AddForce(force, EForceMode.Impulse);
		}

		private void UpdateFootstepSounds()
		{
			m_DistanceTraveled += Translation.Distance(m_PreviousPosition);
			m_PreviousPosition = Translation;

			if (m_DistanceTraveled < m_FootstepLength || !m_CharacterController.IsGrounded)
			{
				return;
			}

			Audio.PostEventAtLocation(m_PlayFootstepID, Transform.WorldTransform.Position, Transform.WorldTransform.Rotation);
			m_DistanceTraveled = 0.0f;
		}

	}
}
