namespace Engine
{
	public class Collider
	{
		public Collider()
		{
			Entity = null;
			CollisionShape = null;
			Offset = Vector3.Zero;
			RigidBody = null;
		}

		public Collider(ulong entityID, Shape collisionShape, Vector3 offset)
		{
			Entity = new Entity(entityID);
			CollisionShape = collisionShape;
			Offset = offset;
			RigidBody = Entity.GetComponent<RigidBodyComponent>();
		}

		public Entity Entity { get; private set; }
		public Shape CollisionShape { get; protected set; }
		public Vector3 Offset { get; protected set; }
		public RigidBodyComponent RigidBody { get; private set; }

		public override string ToString()
		{
			return "Engine.Collider";
		}
	}
}
