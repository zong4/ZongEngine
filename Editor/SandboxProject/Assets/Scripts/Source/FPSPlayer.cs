using System;
using Engine;

namespace FPSExample
{
	public class FPSPlayer : Entity
	{
		public float WalkingSpeed = 10.0f;
		public float RunSpeed = 20.0f;
		public float JumpForce = 50.0f;

		public Prefab ProjectilePrefab;

		public Entity[] EntityArray;

		[NonSerialized]
		public float MouseSensitivity = 10.0f;

		private bool m_Colliding = false;
		public float CurrentSpeed { get; private set; }

		private RigidBodyComponent m_RigidBody;
		private TransformComponent m_Transform;
		private TransformComponent m_CameraTransform;

		private Entity m_CameraEntity;

		private Vector2 m_LastMousePosition;

		private float m_CurrentYMovement = 0.0f;
		private float m_RotationY = 0.0f;

		private Vector2 m_MovementDirection = new Vector2(0.0F);
		private bool m_ShouldJump = false;

		private Vector3 m_PreviousPosition;
		float m_DistanceTraveled = 0.0f;
		public int PlayFootstepSounds = 0;
		public float FootstepLength_m = 1.4f;
		public float FootstepsVolume = 0.22f;

		private float m_ShootingCooldown = 0.15f;
		private float m_ShootingCooldownTimer = 0.0f;

		private Entity m_ProjectileParent;

		private readonly AudioCommandID m_PlayShotSoundGraphSoundID = new AudioCommandID("play_soundgraph");
		private readonly AudioCommandID m_PlayShotSoundID = new AudioCommandID("play_nonsoundgraph");
		private readonly AudioCommandID m_PlayFootstepID = new AudioCommandID("play_footstep");

		private void PlayFootstepSound()
		{
			Audio.PostEventAtLocation(m_PlayFootstepID, Transform.WorldTransform.Position, Transform.WorldTransform.Rotation);
			m_DistanceTraveled = 0.0f;
		}

		protected override void OnCreate()
		{
			Audio.PreloadEventSources(m_PlayShotSoundGraphSoundID);
			Audio.PreloadEventSources(m_PlayShotSoundID);

			m_Transform = GetComponent<TransformComponent>();
			m_RigidBody = GetComponent<RigidBodyComponent>();

			CurrentSpeed = WalkingSpeed;

			CollisionBeginEvent += (n) => { m_Colliding = true; };
			CollisionEndEvent += (n) => { m_Colliding = false; };

			m_CameraEntity = Scene.FindEntityByTag("Camera");
			m_CameraTransform = m_CameraEntity.GetComponent<TransformComponent>();

			m_LastMousePosition = Input.GetMousePosition();

			Input.SetCursorMode(CursorMode.Locked);

			// Setup footsteps
			m_PreviousPosition = Transform.Translation;
			FootstepLength_m = Math.Max(0.0f, FootstepLength_m);
			if (FootstepsVolume < 0.0f)
				FootstepsVolume = 0.22f;
			else
				FootstepsVolume = Math.Min(1.0f, FootstepsVolume);

			m_ProjectileParent = Scene.FindEntityByTag("ProjectileHolder");
		}

		protected override void OnUpdate(float ts)
		{
			if (Input.IsKeyPressed(KeyCode.Escape) && Input.GetCursorMode() == CursorMode.Locked)
				Input.SetCursorMode(CursorMode.Normal);

			if (Input.IsMouseButtonPressed(MouseButton.Left) && Input.GetCursorMode() == CursorMode.Normal)
            {
				Input.SetCursorMode(CursorMode.Locked);
				m_LastMousePosition = Input.GetMousePosition();
            }

			CurrentSpeed = Input.IsKeyPressed(KeyCode.LeftControl) ? RunSpeed : WalkingSpeed;

			UpdateMovementInput();
			UpdateRotation(ts);
			UpdateShooting(ts);

			// Play footstep sounds
			m_DistanceTraveled += Transform.Translation.Distance(m_PreviousPosition);
			m_PreviousPosition = Transform.Translation;
			if (m_DistanceTraveled >= FootstepLength_m && m_Colliding && PlayFootstepSounds == 1)
			{
				PlayFootstepSound();
			}
		}

        private void SpawnProjectileContainer()
        {
            if (!ProjectilePrefab)
                return;

            Entity projectile = Scene.InstantiatePrefab(ProjectilePrefab, Transform.Translation + m_CameraTransform.WorldTransform.Forward);
            var rb = projectile.GetComponent<RigidBodyComponent>();
			projectile.Translation = Transform.Translation + m_CameraTransform.WorldTransform.Forward;

            if (m_ProjectileParent != null)
                projectile.Parent = m_ProjectileParent;

            Vector3 force = m_CameraTransform.WorldTransform.Forward * 20.0f;
            rb.AddForce(force, EForceMode.Impulse);
        }

        private void UpdateShooting(float ts)
        {
			m_ShootingCooldownTimer -= ts;
			bool trigger = GetTriggerAxis() > 0.0f;

			if ((trigger || Input.IsMouseButtonPressed(MouseButton.Left)) && m_ShootingCooldownTimer < 0.0f)
			{
				m_ShootingCooldownTimer = m_ShootingCooldown;
				SpawnProjectileContainer();

				Audio.PostEventAtLocation(m_PlayShotSoundGraphSoundID, m_Transform.WorldTransform.Position, m_Transform.WorldTransform.Rotation);
			}
			else if (Input.IsMouseButtonPressed(MouseButton.Right) && m_ShootingCooldownTimer < 0.0f)
			{
				m_ShootingCooldownTimer = m_ShootingCooldown;
				SpawnProjectileContainer();
				
				Audio.PostEventAtLocation(m_PlayShotSoundID, m_Transform.WorldTransform.Position, m_Transform.WorldTransform.Rotation);
			}
		}

		private float GetMovementAxis(int axis, float deadzone = 0.2f)
        {
            float value = Input.GetControllerAxis(0, axis);
            return Mathf.Abs(value) < deadzone ? 0.0f : value;
        }

		private float GetHorizontalMovementAxis(float deadzone = 0.2f)
        {
            // Dpad
            byte hat = Input.GetControllerHat(0, 0);
			if ((hat & 2) != 0)
				return 1.0f;
            if ((hat & 8) != 0)
                return -1.0f;

			// Analogue stick
			return GetMovementAxis(0, deadzone);
		}

        private float GetVerticalMovementAxis(float deadzone = 0.2f)
        {
			// Dpad
            byte hat = Input.GetControllerHat(0, 0);
            if ((hat & 4) != 0)
                return 1.0f;
            if ((hat & 1) != 0)
                return -1.0f;

            // Analogue stick
            return GetMovementAxis(1, deadzone);
		}

        private float GetTriggerAxis(float deadzone = 0.2f)
        {
            return GetMovementAxis(5, deadzone);
        }

        private void UpdateMovementInput()
		{
			m_MovementDirection.X = 0.0f;
			m_MovementDirection.Y = 0.0f;

			m_MovementDirection.X = GetHorizontalMovementAxis();
			m_MovementDirection.Y = -GetVerticalMovementAxis();

			if (Input.IsKeyDown(KeyCode.W))
				m_MovementDirection.Y = 1.0f;
			else if (Input.IsKeyDown(KeyCode.S))
				m_MovementDirection.Y = -1.0f;

			if (Input.IsKeyDown(KeyCode.A))
				m_MovementDirection.X = -1.0f;
			else if (Input.IsKeyDown(KeyCode.D))
				m_MovementDirection.X = 1.0f;

			m_ShouldJump = (Input.IsKeyPressed(KeyCode.Space) || Input.IsControllerButtonPressed(0, 0)) && !m_ShouldJump;
		}

		protected override void OnPhysicsUpdate(float fixedTimeStep)
		{
			UpdateMovement();
		}

		private void UpdateRotation(float ts)
		{
			if (Input.GetCursorMode() != CursorMode.Locked)
				return;

			{
				float sensitivity = 3.0f;

				float hAxis = -GetMovementAxis(2);
				float vAxis = -GetMovementAxis(3);
				Vector2 delta = new Vector2(hAxis * hAxis, vAxis * vAxis);
				delta *= new Vector2(Math.Sign(hAxis), Math.Sign(vAxis));
				m_CurrentYMovement = delta.X * sensitivity;
				float xRotation = delta.Y * sensitivity * ts;

				if (xRotation != 0.0f)
					m_CameraTransform.Rotation += new Vector3(xRotation, 0.0f, 0.0f);

				m_CameraTransform.Rotation = new Vector3(Mathf.Clamp(m_CameraTransform.Rotation.X * Mathf.Rad2Deg, -80.0f, 80.0f), 0.0f, 0.0f) * Mathf.Deg2Rad;
			}

            // Mouse
            {
                // TODO: Mouse position should be relative to the viewport
                Vector2 currentMousePosition = Input.GetMousePosition();
                Vector2 delta = m_LastMousePosition - currentMousePosition;
				if (delta.X != 0.0f)
					m_CurrentYMovement += delta.X * MouseSensitivity * ts;
				
				float xRotation = delta.Y * (MouseSensitivity * 0.05f) * ts;

				if (xRotation != 0.0f)
					m_CameraTransform.Rotation += new Vector3(xRotation, 0.0f, 0.0f);

				m_LastMousePosition = currentMousePosition;
			}
		}

		private void UpdateMovement()
		{
            if (Input.GetCursorMode() != CursorMode.Locked)
                return;

			m_RotationY += m_CurrentYMovement * 0.05f;
			var movement = Translation + (m_CameraTransform.WorldTransform.Right * m_MovementDirection.X + m_CameraTransform.WorldTransform.Forward * m_MovementDirection.Y);
			m_RigidBody.MoveKinematic(movement, m_RotationY * Vector3.Up, 0.1f);
			m_CurrentYMovement = 0.0f;

			/*Rotation += m_CurrentYMovement * Vector3.Up * 0.05f;

			movement.Y = 0.0f;

			if (movement.Length() > 0.0f)
			{
				movement.Normalize();
				Vector3 velocity = movement * CurrentSpeed;
				velocity.Y = m_RigidBody.LinearVelocity.Y;
				m_RigidBody.LinearVelocity = velocity;
			}

			if (m_ShouldJump && m_Colliding)
			{
				m_RigidBody.AddForce(Vector3.Up * JumpForce, EForceMode.Impulse);
				m_ShouldJump = false;
			}*/
		}
	}
}
