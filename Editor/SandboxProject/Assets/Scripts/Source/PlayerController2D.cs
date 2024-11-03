using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using Engine;

namespace PlayerController2D
{
    class PlayerController2D : Entity
    {
        private RigidBody2DComponent m_RigidBody;
        public float Speed = 15.0f;
        public float WallJumpSpeed = 20.0f;
        public float JumpForce = 35.0f;
        public float JumpMultiplier = 1.0f;
        public float FallMultiplier = 2.1f;
        public float DashForce = 100.0f;
        public float TimeToDash = 1.0f;
        public float WallClimbSpeed = 3.0f; // temp public
        public int Jumps = 2; 
        public int MaximumDashSteps = 25; // temp public Variable
        public int MaximumDashWaitSteps = 3; // temp public Variable

        private Vector2 m_MovementInput = Vector2.Zero;

        private int m_DefaultJumps = 0;
        private int m_Jumps = 0;
        private int m_JumpSteps = 0;
        private static int m_MaxJumpSteps = 12;
        private int m_WallJumpSteps = 0;
        private static int m_MaxWallJumpSteps = 20;
        private int m_WallLockSteps = 0;
        private int m_MaxWallLockSteps = 4;
        private int m_DashWaitSteps = 0;
        private int m_DashSteps = 0;

		private float m_JumpTimer = 0.0f;
		private float m_MaximumJumpTime = 0.0833f;
		private float m_WallJumpTimer = 0.0f;
		private float m_MaximumWallJumpTime = 0.1388f;
		private float m_WallLockTimer = 0.0f;
		private float m_MaxWallLockTime = 0.0277f;
		private float m_DashWaitTimer = 0.0f;
		private float m_MaximumDashWaitTimer = 0.02083f;
		private float m_MaxDashTimer = 0.1736f;


        private int m_GemsCollected = 0;

        private int m_ControllerID = 0; // Change depending on the controller

        private bool m_DoJump = false;
        private bool m_CanJump = true;
        private bool m_CanSecondJump = true;
        private bool m_CanWallJump = true;
        private bool m_BeginSecondJump = false;
        private bool m_StartWaitForDash = false;
        private bool m_DoDash = false;
        private bool m_ShouldDash = false;
        private bool m_IsGrounded = false;
        private bool m_IsWallJumping = false;
        private bool m_FinishedWallJumping = false;
        private bool m_DoWallJump = false;
        private bool m_IsOnWallLeft = false;
        private bool m_IsOnWallRight = false;
        private bool m_IsOnWall = false;
        private bool m_IsJumping = false;
        private bool m_IsDashing = false;
        private bool m_DoAttack = false;
        
        //Timers in seconds
        float m_TimeSinceDash = 5.0f;
        float m_TimeToDash = 5.0f;
        float m_TimeSinceAttack = 1.0f;
        float m_TimeToAttack = 1.0f;

        private float m_Speed;
        private float m_WallJumpSpeed;
        private float m_WallJumpDirection;
        private float m_JumpForce;
        private float m_JumpMultiplier;
        private float m_FallMultiplier;
        private float m_DashForce;
        private float m_WallClimbSpeed;
        

        protected override void OnCreate()
        {
            m_Speed = Speed;
            m_WallJumpSpeed = WallJumpSpeed;
            m_DefaultJumps = Jumps;
            m_JumpForce = JumpForce;
            m_DashForce = DashForce;
            m_JumpMultiplier = JumpMultiplier;
            m_FallMultiplier = FallMultiplier;
            m_TimeToDash = TimeToDash;
            m_RigidBody = GetComponent<RigidBody2DComponent>();
            Collision2DBeginEvent += OnCollisionBegin;
        }

        void OnCollisionBegin(Entity other)
        {
            //if(other.Tag == "Gem")
            //{
            //    m_GemsCollected++;
            //    Destroy(other); 
            //}
        }

        float m_PreviousFrameTime = 0.0f;
        float m_CurrentFrameTime;

        protected override void OnUpdate(float ts) 
        {   
            // dt is the difference in time between the last frame and the current frame
            m_CurrentFrameTime = ts;
            float dt = m_CurrentFrameTime - m_PreviousFrameTime;
            m_PreviousFrameTime = ts;

            UpdateMovementInput();
            UpdateMovementKeys();
            UpdateMovement(ts, dt);
            m_IsOnWall = (m_IsOnWallRight || m_IsOnWallLeft); //&& m_MovementInput.X != 0.0f;
            if(Input.IsKeyPressed(KeyCode.Tab))
            {
                Log.Warn("IS ON WALL : {0}", m_IsOnWall);
                Log.Warn("IS ON GROUND : {0}", m_IsGrounded);
            }

            Vector2 groundRaycastOffset = new Vector2(0.5f, 0.0f); // change these when you have a new character model
            RaycastHit2D[] groundCheck0 = Physics2D.Raycast2D(Translation.XY - groundRaycastOffset, Vector2.Down, 0.52f); // change these when you have a new character model
            RaycastHit2D[] groundCheck1 = Physics2D.Raycast2D(Translation.XY + groundRaycastOffset, Vector2.Down, 0.52f); // change these when you have a new character model
            RaycastHit2D[] wallCheckLeft = Physics2D.Raycast2D(Translation.XY, Vector2.Left, 0.52f); // change these when you have a new character model
            RaycastHit2D[] wallCheckRight = Physics2D.Raycast2D(Translation.XY, Vector2.Right, 0.52f); // change these when you have a new character model
            
            if (groundCheck0.Length > 0 || groundCheck1.Length > 0)
                m_IsGrounded = true;
            else
                m_IsGrounded = false;

            if(wallCheckLeft.Length > 0)
            {
                //if(wallCheckLeft[0].Entity.Tag == "Platform2D")
                m_IsOnWallLeft = true;
                //Log.Warn(m_IsOnWallLeft);
            }
            else
                m_IsOnWallLeft = false;

            if (wallCheckRight.Length > 0)
            { 
                //if(wallCheckRight[0].Entity.Tag == "Platform2D")
                m_IsOnWallRight = true;
            }
            else
                m_IsOnWallRight = false;
        }

        bool m_SpaceKeyPressed = false;
        bool m_SpaceKeyState = false;
        bool m_SpaceKeyReleased = false;

        bool m_JumpButtonPressed = false;
        bool m_JumpButtonState = false;
        bool m_JumpButtonReleased = false;

        bool m_SideMovementInputPressed = false;
        bool m_SideMovementInputState = false;
        bool m_SideMovementInputReleased = false;

        void UpdateMovementKeys()
        {
            bool spaceKeyState = Input.IsKeyPressed(KeyCode.Space);
            m_SpaceKeyPressed = spaceKeyState && !m_SpaceKeyState;
            m_SpaceKeyReleased = !spaceKeyState && m_SpaceKeyState;
            m_SpaceKeyState = spaceKeyState;
            
            bool jumpButtonState = Input.IsControllerButtonPressed(m_ControllerID, 0);
            m_JumpButtonPressed = jumpButtonState && !m_JumpButtonState;
            m_JumpButtonReleased = !jumpButtonState && m_JumpButtonState;
            m_JumpButtonState = jumpButtonState;

            bool sideMovementState = m_MovementInput.X != 0.0f;
            m_SideMovementInputPressed = sideMovementState  && !m_SideMovementInputState;
            m_SideMovementInputReleased = !m_SideMovementInputState && m_SideMovementInputState;
            m_SideMovementInputState = sideMovementState;
        }

        void UpdateMovement(float ts, float dt)
        {
            m_TimeSinceDash += ts;
            m_TimeSinceAttack += ts;
            
            m_MovementInput.Normalize();
            m_WallJumpDirection = GetWallJumpDirection();

            if(m_IsGrounded)
            {
                m_JumpTimer = 0.0f;
                m_BeginSecondJump = false;
                m_Jumps = m_DefaultJumps;
                m_TimeSinceDash = m_TimeToDash;
                if(m_IsGrounded && m_IsOnWall)
                    m_IsOnWall = false;
            }

            if(m_IsOnWall)
            {
                m_JumpTimer = 0.0f;
                m_BeginSecondJump = false;
                m_Jumps = m_DefaultJumps;
            }

            //WallClimb Doesnt work yet
            //if(m_IsOnWallLeft && m_MovementInput.X < 0.0f || m_MovementInput.Y > 0)
            //{
            //    Log.Warn("Should be moving up!");
            //    //m_RigidBody.ApplyLinearImpulse(new Vector2(0.0f, m_WallClimbSpeed), Vector2.Zero, true);
            //    m_RigidBody.Translation = m_RigidBody.Translation + Vector2.Up * m_WallClimbSpeed;
            //}
            //else if(m_IsOnWallRight && m_MovementInput.X > 0.0f || m_MovementInput.Y > 0)
            //{
            //    Log.Warn("Should be moving up!");
            //    //m_RigidBody.ApplyLinearImpulse(new Vector2(0.0f, m_WallClimbSpeed), Vector2.Zero, true);
            //    m_RigidBody.Translation = m_RigidBody.Translation + Vector2.Up * m_WallClimbSpeed;
            //}
            //else
            //{ 
            //    Log.Warn("Should be moving down!");
            //    //m_RigidBody.ApplyLinearImpulse(new Vector2(0.0f, -m_WallClimbSpeed), Vector2.Zero, true);
            //    m_RigidBody.Translation = m_RigidBody.Translation + Vector2.Down * m_WallClimbSpeed;
            //}

            if (m_IsOnWall)
            {
                m_WallJumpTimer = 0.0f;
                if(m_MovementInput.X == 0)
                    m_WallJumpTimer += ts;
                else
                    m_WallLockTimer = 0.0f;

                if(m_WallLockTimer < m_MaxWallLockTime && !m_IsWallJumping)
                {
                    m_RigidBody.LinearVelocity = Vector2.Zero;
                    m_RigidBody.GravityScale = 0.0f;
                }
                else
                {
                    //This is necessary because the friction between the two bodies is too great and the speed of the fall would be too slow as a result.
                    m_RigidBody.GravityScale = 1.0f;
                    if(m_IsOnWallLeft)
                        m_RigidBody.Translation = new Vector2(m_RigidBody.Translation.X + 0.02f, m_RigidBody.Translation.Y);
                    else
                        m_RigidBody.Translation = new Vector2(m_RigidBody.Translation.X - 0.02f, m_RigidBody.Translation.Y);
                }
            }
            else
                m_RigidBody.GravityScale = 1.0f;

            m_CanJump = m_JumpTimer < m_MaximumJumpTime && m_Jumps > 0 && !m_BeginSecondJump && !m_DoWallJump && !m_IsWallJumping && ! m_FinishedWallJumping;
            m_CanSecondJump = !m_CanJump && m_Jumps > 0 && !m_DoWallJump && !m_IsWallJumping;

            if(m_DoJump)
            {
                //Log.Warn("Told to jump!");
                //Log.Warn("Finished wall jump : {0}", m_FinishedWallJumping);
                //Log.Warn("isWall jumping : {0}", m_IsWallJumping);
                //Log.Warn("Do Wall jump: {0}", m_DoWallJump);
                if(m_CanJump)
                {
                    //Log.Warn("JumpSteps FIRST : {0}", m_JumpTimer);
                    m_IsJumping = true;
                    m_RigidBody.LinearVelocity = new Vector2(m_Speed * m_MovementInput.X, m_RigidBody.LinearVelocity.Y);
                    Jump();
                }

                if(m_BeginSecondJump)
                {
                    //Log.Warn("JumpSteps SECOND : {0}", m_JumpTimer);
                    m_RigidBody.LinearVelocity = new Vector2(m_Speed * m_MovementInput.X, m_RigidBody.LinearVelocity.Y);
                    m_IsJumping = true;
                    Jump();
                }
            }

            if(m_DoWallJump)
            {
                float wallJumpSpeed = m_WallJumpSpeed;
                if (m_WallJumpDirection  != 0)
                    wallJumpSpeed = m_WallJumpSpeed * m_WallJumpDirection ;

                if (m_MovementInput.X == m_WallJumpDirection)
                {
                    //Log.Warn("Input {0} == direction {1}", m_MovementInput.X, m_WallJumpDirection);
                    m_RigidBody.LinearVelocity = new Vector2(wallJumpSpeed * m_WallJumpDirection, m_RigidBody.LinearVelocity.Y);
                    m_IsWallJumping = true;
                    WallJump();
                }
                else if(m_MovementInput.X != m_WallJumpDirection && m_MovementInput.X != 0.0f && m_WallJumpDirection != 0.0f)
                { 
                    //Log.Warn("Input {0} != direction {1}", m_MovementInput.X, m_WallJumpDirection);
                    m_RigidBody.LinearVelocity = new Vector2((wallJumpSpeed * m_WallJumpDirection) / 3 , m_RigidBody.LinearVelocity.Y);
                    m_IsWallJumping = true;
                    SmallWallJump();
                }
            }

            if(m_SpaceKeyReleased || m_JumpButtonReleased)
            {
                m_JumpTimer = 0.0f;
                m_IsJumping = false;
                m_DoJump = false;
                m_FinishedWallJumping = false;
                if(m_CanSecondJump)
                    m_BeginSecondJump = m_Jumps > 0;
            }

            if(m_ShouldDash)
            {
                m_TimeSinceDash = 0.0f;
                Dash();
                m_IsDashing = true;
                m_ShouldDash = false;
            }

            if(m_StartWaitForDash)
            {
                m_DashWaitTimer += ts;
                if(m_DashWaitTimer > m_MaximumDashWaitTimer)
                {
                    m_ShouldDash = true;
                    m_DashWaitSteps = 0;
                    m_StartWaitForDash = false;
                }
            }

            if(m_DoDash)
            {
                if(m_TimeSinceDash >= m_TimeToDash)
                {
                    //m_RigidBody.LinearVelocity = Vector2.Zero;
                    m_RigidBody.GravityScale = 0.0f;
                    m_StartWaitForDash = true;
                }
            }

            if(m_DoAttack)
            {
                if(m_TimeSinceAttack >= m_TimeToAttack)
                {
                    //TODO: Raycast to check if the enemy is within the attack range if it is DoAttack and Jump
                    m_TimeSinceAttack = 0.0f;
                    Jump();
                    m_IsJumping = true;
                }
            }

            if(m_IsJumping)
            {
                m_JumpTimer += ts;
                if (m_JumpTimer >= m_MaximumJumpTime)
                {
                    m_Jumps--;
                    m_IsJumping = false;
                    if(m_BeginSecondJump)
                    {
                        m_BeginSecondJump = false;
                        m_CanSecondJump = false;
                    }
                }
            }

            if(m_IsWallJumping)
            {
                //Log.Warn("WALL JUMPING");
                m_DoWallJump = false;
                m_WallJumpTimer += ts;
                if(m_WallJumpTimer >= m_MaximumWallJumpTime && !m_FinishedWallJumping)
                {
                    m_IsWallJumping = false;
                    m_CanWallJump = false;
                    m_FinishedWallJumping = true;
                }
            }

            if(m_IsDashing)
            {
                m_DashWaitTimer += ts;
                if(m_DashWaitTimer >= m_MaxDashTimer)
                {
                    m_RigidBody.GravityScale = 1.0f;
                    m_DashWaitTimer = 0.0f;
                    m_IsDashing = false;
                    Vector2 currentVelocity = m_RigidBody.LinearVelocity;
                    currentVelocity.Y = 0.0f;
                    m_RigidBody.LinearVelocity = Vector2.Zero;
                }
            }
            bool isInAir = !m_IsGrounded && !m_IsDashing && !m_StartWaitForDash && !m_IsOnWall && !m_IsWallJumping;
            //cancel out any velocity that you might have if you are not pressing a movement key
            if(isInAir)
            {
                if(m_MovementInput.X == 0.0f && !m_CanWallJump)
                    m_RigidBody.LinearVelocity = new Vector2(0.0f, m_RigidBody.LinearVelocity.Y);
            }

            //Initial Part of the Jump (Positive Y velocity and isJumping = true)
            if(m_RigidBody.LinearVelocity.Y > 0.0f && isInAir)
                m_RigidBody.AddForce(Vector2.Down * m_JumpMultiplier, Vector2.Zero, true);

            //Second Part of the Jump (Positive Y velocity but the isJumping = fasle)
            if(!m_IsJumping && m_RigidBody.LinearVelocity.Y > 0.0f && isInAir)
                m_RigidBody.AddForce(Vector2.Down * m_FallMultiplier, Vector2.Zero, true);
            
            //falling gravity, falls heavier when the player is not holding a movement key
            if(m_RigidBody.LinearVelocity.Y < 0.0f && isInAir)
            { 
                if(m_MovementInput.X == 0.0f)
                    m_RigidBody.AddForce(Vector2.Down * m_FallMultiplier * 2, Vector2.Zero, true);
                else
                    m_RigidBody.AddForce(Vector2.Down * m_FallMultiplier, Vector2.Zero, true);
            }
            //movement left right
            if(m_IsGrounded || m_RigidBody.LinearVelocity.Y < 0.0f && isInAir && !m_IsWallJumping)
               m_RigidBody.LinearVelocity = new Vector2(m_MovementInput.X * m_Speed, m_RigidBody.LinearVelocity.Y);
        }

        private void Jump()
        {
            m_RigidBody.LinearVelocity = new Vector2(m_RigidBody.LinearVelocity.X, m_JumpForce);
            m_DoJump = false;
        }

        private void Dash()
        {
            Vector2 dashForce = new Vector2(m_DashForce * m_MovementInput.X, m_DashForce * m_MovementInput.Y);
            m_RigidBody.ApplyLinearImpulse(dashForce, Vector2.Zero, true);
        }

        private void WallJump()
        { 
            m_RigidBody.LinearVelocity = new Vector2(m_Speed * m_WallJumpDirection * 2, m_JumpForce / 2);
            m_DoWallJump = false;
        }

        private void SmallWallJump()
        { 
            m_RigidBody.LinearVelocity = new Vector2(m_WallJumpDirection * m_Speed, m_JumpForce);
            m_DoWallJump = false;
        }

        private float GetWallJumpDirection()
        {
            //if(m_IsOnWallLeft)
            //    return 1.0f;
            //else if(m_IsOnWallRight)
            //    return -1.0f;
            //else
            //    return 0.0f;
            return Convert.ToSingle(m_IsOnWallLeft) - Convert.ToSingle(m_IsOnWallRight);
        }

        private void UpdateMovementInput()
        {
            m_DoWallJump = (Input.IsKeyPressed(KeyCode.Space) || Input.IsControllerButtonPressed(m_ControllerID, 0)) && m_IsOnWall;
            m_DoJump = (Input.IsKeyPressed(KeyCode.Space) || Input.IsControllerButtonPressed(m_ControllerID, 0)) && !m_DoJump && !m_IsOnWall;
            m_DoDash = Input.IsKeyPressed(KeyCode.C) || GetLeftTriggerAxis() != -1.0f || GetRightTriggerAxis() != -1.0f && !m_DoDash && !m_IsGrounded && m_MovementInput.Length() != 0.0f;
            m_DoAttack = (Input.IsKeyPressed(KeyCode.X) || Input.IsControllerButtonPressed(m_ControllerID, 1)) && !m_DoAttack;

            m_MovementInput = new Vector2(GetInputH(), GetInputV());
        }

        private float GetMovementAxis(int axis, float deadzone = 0.2f)
        {
            float value = Input.GetControllerAxis(m_ControllerID, axis);
            return Mathf.Abs(value) < deadzone ? 0.0f : value;
        }

        private float GetHorizontalMovementAxis(float deadzone = 0.2f)
        {
            // Dpad
            byte hat = Input.GetControllerHat(m_ControllerID, 0);
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
            byte hat = Input.GetControllerHat(m_ControllerID, 0);
            if ((hat & 4) != 0)
                return 1.0f;
            if ((hat & 1) != 0)
                return -1.0f;

            // Analogue stick
            return GetMovementAxis(1, deadzone);
		}

        private float GetLeftTriggerAxis(float deadzone = 0.2f)
        {
            return GetMovementAxis(4, deadzone);
        }

        private float GetRightTriggerAxis(float deadzone = 0.2f)
        {
            return GetMovementAxis(5, deadzone);
        }

        private float GetInputH()
        {
            float horizontalInput = GetHorizontalMovementAxis();

            if (Input.IsKeyHeld(KeyCode.A) || Input.IsKeyHeld(KeyCode.Left))
                horizontalInput = -1.0f;
            else if (Input.IsKeyHeld(KeyCode.D) || Input.IsKeyHeld(KeyCode.Right))
                horizontalInput = 1.0f;
            
            return horizontalInput;
        }

        private float GetInputV()
        {
            float verticalInput = -GetVerticalMovementAxis();

            if (Input.IsKeyHeld(KeyCode.W) || Input.IsKeyHeld(KeyCode.Up))
                verticalInput = 1.0f;
            else if (Input.IsKeyHeld(KeyCode.S) || Input.IsKeyHeld(KeyCode.Down))
                verticalInput = -1.0f;
            
            return verticalInput; 
        }
    }
}
