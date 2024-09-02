using System;
using Hazel;

namespace Sandbox
{
	public class Testing : Entity
	{

		public Prefab MyPrefab;
		public Scene SceneTest;
		public Entity EntityTest;
		public Mesh MeshTest;
		public StaticMesh StaticMeshTest;
		public Texture2D TextureTest;

		private float m_Value = 0.0f;

		private Entity m_PrefabEntity;

		protected override void OnCreate()
		{
			m_PrefabEntity = Instantiate(MyPrefab);
			DetectGamepad();
		}

		private void DetectGamepad()
		{
			Log.Info($"Detected: {Input.GetConnectedControllerIDs().Length} controllers");

			// NOTE(Peter): Very hacky way to ensure we use a PlayStation or Xbox controller (otherwise it'd pick up my x56 H.O.T.A.S as the first controller)
			foreach (var controllerID in Input.GetConnectedControllerIDs())
			{
				Log.Info($"Controller: {Input.GetControllerName(controllerID)}");
			}
		}

		protected override void OnUpdate(float ts)
		{
			if (Input.IsControllerPresent(0))
			{
				Log.Info($"Controller is present: {Input.GetControllerName(0)}");
			}
			else
			{
				Log.Info($"Controller is NOT present");
			}

			var transform = m_PrefabEntity.GetComponent<TransformComponent>();
			Log.Info($"Transform: {transform.Translation}");

			m_Value += 1.0f * ts;
		}

		protected override void OnDestroy()
		{
			Log.Info($"Value: {m_Value}");
		}

	}
}
