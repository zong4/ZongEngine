using System;
using Hazel;

namespace Example
{
    class PlayerCube : Entity
    {
        public float HorizontalForce = 10.0f;
        public float JumpForce = 10.0f;

        public int NumberOfCubes;
        public Vector2 CubeSpawnOffset;
        public Vector2 CubeSpawnRange;

        private RigidBody2DComponent m_PhysicsBody;
        private Material m_MeshMaterial;

        int m_CollisionCounter = 0;

        public Vector2 MaxSpeed = new Vector2();

        private bool Colliding => m_CollisionCounter > 0;

        private Entity m_CubeEntity;

        protected override void OnCreate()
        {
            m_PhysicsBody = GetComponent<RigidBody2DComponent>();

            StaticMeshComponent meshComponent = GetComponent<StaticMeshComponent>();
            m_MeshMaterial = meshComponent.Mesh.GetMaterial(0);
            m_MeshMaterial.Metalness = 0.0f;

            Collision2DBeginEvent += OnPlayerCollisionBegin;
            Collision2DEndEvent += OnPlayerCollisionEnd;

            m_CubeEntity = Scene.FindEntityByTag("_CUBE");

            for (int i = 0; i < NumberOfCubes; i++)
            {
                float x = CubeSpawnOffset.X + Hazel.Random.Float() * CubeSpawnRange.X;
                float y = CubeSpawnOffset.Y + Hazel.Random.Float() * CubeSpawnRange.Y;
                CreateCube(new Vector2(x, y), Hazel.Random.Float() * 10.0f, i);
            }
        }

        private void CreateCube(Vector2 translation, float rotation, int cubeIndex)
        {
            Entity entity = Scene.CreateEntity($"Cube-{cubeIndex}");
            var tc = entity.GetComponent<TransformComponent>();
            Vector3 position = new Vector3(translation.X, translation.Y, 0.0f);
            tc.Translation = position;
            tc.Rotation = new Vector3(0.0f, 0.0f, rotation);

            var mc = entity.CreateComponent<MeshComponent>();
            mc.Mesh = m_CubeEntity.GetComponent<MeshComponent>().Mesh;
            var rb2d = entity.CreateComponent<RigidBody2DComponent>();
            rb2d.BodyType = RigidBody2DBodyType.Dynamic;
        }

        void OnPlayerCollisionBegin(Entity other)
        {
            m_CollisionCounter++;
        }

        void OnPlayerCollisionEnd(Entity other)
        {
            m_CollisionCounter--;
        }

        protected override void OnUpdate(float ts)
        {
            float movementForce = HorizontalForce;

            if (!Colliding)
                movementForce *= 0.4f;

            if (Input.IsKeyPressed(KeyCode.D))
                m_PhysicsBody.ApplyLinearImpulse(new Vector2(movementForce, 0), new Vector2(), true);
            else if (Input.IsKeyPressed(KeyCode.A))
                m_PhysicsBody.ApplyLinearImpulse(new Vector2(-movementForce, 0), new Vector2(), true);

            if (Colliding && Input.IsKeyPressed(KeyCode.Space))
                m_PhysicsBody.ApplyLinearImpulse(new Vector2(0, JumpForce), new Vector2(0, 0), true);

            if (m_CollisionCounter > 0)
                m_MeshMaterial.AlbedoColor = new Vector3(1.0f, 0.0f, 0.0f);
            else
                m_MeshMaterial.AlbedoColor = new Vector3(0.8f, 0.8f, 0.8f);

            Vector2 linearVelocity = m_PhysicsBody.LinearVelocity;
            linearVelocity.Clamp(new Vector2(-MaxSpeed.X, -1000), MaxSpeed);
            m_PhysicsBody.LinearVelocity = linearVelocity;
        }
    }
}
