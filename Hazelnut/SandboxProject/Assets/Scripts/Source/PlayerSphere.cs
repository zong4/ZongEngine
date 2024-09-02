using System;

using Hazel;

namespace Example
{
    class PlayerSphere : Entity
    {
        public float HorizontalForce = 10.0f;
        public float JumpForce = 10.0f;

        private RigidBodyComponent m_PhysicsBody;
        private Material m_MeshMaterial;

        int m_CollisionCounter = 0;
        int m_TriggerCollisionCounter = 0;
        int m_2DCollisionCounter = 0;

        public Vector3 MaxSpeed = new Vector3();

        private bool Colliding => m_CollisionCounter > 0;
        private bool Triggering => m_TriggerCollisionCounter > 0;
        private bool TwoDColliding => m_2DCollisionCounter > 0;

        private TransformComponent m_Transform;

        protected override void OnCreate()
        {
            m_PhysicsBody = GetComponent<RigidBodyComponent>();
            m_Transform = GetComponent<TransformComponent>();

            MeshComponent meshComponent = GetComponent<MeshComponent>();
            m_MeshMaterial = meshComponent.Mesh.GetMaterial(0);
            m_MeshMaterial.Metalness = 0.0f;

            CollisionBeginEvent += OnPlayerCollisionBegin;
            CollisionEndEvent += OnPlayerCollisionEnd;
            TriggerBeginEvent += OnPlayerTriggerBegin;
            TriggerEndEvent += OnPlayerTriggerEnd;
            Collision2DBeginEvent += On2DCollisionBegin;
            Collision2DEndEvent += On2DCollisionEnd;
		}

        void OnPlayerCollisionBegin(Entity other)
        {
            m_CollisionCounter++;
        }

        void OnPlayerCollisionEnd(Entity other)
        {
            m_CollisionCounter--;
        }

        void OnPlayerTriggerBegin(Entity other)
		{
            m_TriggerCollisionCounter++;
        }

		void OnPlayerTriggerEnd(Entity other)
		{
            m_TriggerCollisionCounter--;
        }

        void On2DCollisionBegin(Entity entity)
        {
            m_2DCollisionCounter++;
        }

        void On2DCollisionEnd(Entity entity)
        {
            m_2DCollisionCounter--;
        }

        protected override void OnUpdate(float ts)
        {
            float movementForce = HorizontalForce;

            if (!Colliding)
            {
                movementForce *= 0.4f;
            }

            /*Vector3 translation = Translation;
            if (translation.Y < 0)
            {
                Translation = new Vector3(0.0f, 5.0f, 0.0f);
                m_PhysicsBody.LinearVelocity = Vector3.Zero;
            }*/

			if (Input.IsKeyPressed(KeyCode.W))
				m_PhysicsBody.AddForce(m_Transform.LocalTransform.Forward * movementForce);
			else if (Input.IsKeyPressed(KeyCode.S))
				m_PhysicsBody.AddForce(m_Transform.LocalTransform.Forward * -movementForce);
			else if (Input.IsKeyPressed(KeyCode.D))
				m_PhysicsBody.AddForce(m_Transform.LocalTransform.Right * movementForce);
			else if (Input.IsKeyPressed(KeyCode.A))
				m_PhysicsBody.AddForce(m_Transform.LocalTransform.Right * -movementForce);

			if (Colliding && Input.IsKeyPressed(KeyCode.Space))
                m_PhysicsBody.AddForce(m_Transform.LocalTransform.Up * JumpForce);

            if (Colliding)
                m_MeshMaterial.AlbedoColor = new Vector3(1.0f, 0.0f, 0.0f);
            else
                m_MeshMaterial.AlbedoColor = new Vector3(0.2f, 0.2f, 0.2f);

            if (Triggering)
            {
                Console.WriteLine("Triggered!");
                m_MeshMaterial.AlbedoColor = new Vector3(0.0f, 1.0f, 0.0f);
            }
            else
                m_MeshMaterial.AlbedoColor = new Vector3(0.2f, 0.2f, 0.2f);

            Vector3 linearVelocity = m_PhysicsBody.LinearVelocity;
            linearVelocity.Clamp(new Vector3(-MaxSpeed.X, -1000, -MaxSpeed.Z), MaxSpeed);
            m_PhysicsBody.LinearVelocity = linearVelocity;
        }

    }
}
