using System;
using System.Collections;
using System.Collections.Generic;

namespace Hazel
{
	public class Scene : IEquatable<Scene>
	{
		internal AssetHandle m_Handle;
		public AssetHandle Handle => m_Handle;

		public static float TimeScale
		{
			set { InternalCalls.Scene_SetTimeScale(value); }
		}

		internal Scene() { m_Handle = AssetHandle.Invalid; }
		internal Scene(AssetHandle handle) { m_Handle = handle; }

		// This entire Scene is currently static, since we can only have one scene at a time right now
		public static Entity CreateEntity(string tag = "Unnamed") => new Entity(InternalCalls.Scene_CreateEntity(tag));
		public static void DestroyEntity(Entity entity)
		{
			if (entity == null)
				return;

			if (!InternalCalls.Scene_IsEntityValid(entity.ID))
				return;

			// Remove this entity from the cache if it's been indexed either with the ID or Tag hash code
			if (!s_EntityCache.Remove(entity.ID))
				s_EntityCache.Remove((ulong)entity.Tag.GetHashCode());

			InternalCalls.Scene_DestroyEntity(entity.ID);
		}

		public static void DestroyAllChildren(Entity entity)
		{
			if (entity == null)
				return;
			if (!InternalCalls.Scene_IsEntityValid(entity.ID))
				return;
			// Remove this entity from the cache if it's been indexed either with the ID or Tag hash code
			if (!s_EntityCache.Remove(entity.ID))
				s_EntityCache.Remove((ulong)entity.Tag.GetHashCode());
			ulong[] ids = InternalCalls.Scene_GetChildrenIDs(entity.ID);
			foreach (ulong id in ids)
				s_EntityCache.Remove(id);
			InternalCalls.Scene_DestroyAllChildren(entity.ID);
		}

		private static Dictionary<ulong, Entity> s_EntityCache = new Dictionary<ulong, Entity>(50);

		public static Entity FindEntityByTag(string tag)
		{
			ulong hashCode = (ulong)tag.GetHashCode();

			Entity result = null;

			if (s_EntityCache.ContainsKey(hashCode) && s_EntityCache[hashCode] != null)
			{
				result = s_EntityCache[hashCode];
				if (!InternalCalls.Scene_IsEntityValid(result.ID))
				{
					s_EntityCache.Remove(hashCode);
					result = null;
				}
			}

			if (result == null)
			{ 
				ulong entityID = InternalCalls.Scene_FindEntityByTag(tag);
				Entity newEntity = entityID != 0 ? new Entity(entityID) : null;
				s_EntityCache[hashCode] = newEntity;

				if (newEntity != null)
					newEntity.DestroyedEvent += OnEntityDestroyed;

				result = newEntity;
			}

			return result;
		}

		public static Entity FindEntityByID(ulong entityID)
		{
			if (s_EntityCache.ContainsKey(entityID) && s_EntityCache[entityID] != null)
			{
				var entity = s_EntityCache[entityID];

				if (!InternalCalls.Scene_IsEntityValid(entity.ID))
				{
					s_EntityCache.Remove(entityID);
					entity = null;
				}

				return entity;
			}

			if (!InternalCalls.Scene_IsEntityValid(entityID))
				return null;

			Entity newEntity = new Entity(entityID);
			s_EntityCache[entityID] = newEntity;
			newEntity.DestroyedEvent += OnEntityDestroyed;
			return newEntity;
		}

		public static Entity InstantiatePrefab(Prefab prefab)
		{
			ulong entityID = InternalCalls.Scene_InstantiatePrefab(ref prefab.m_Handle);
			return entityID == 0 ? null : new Entity(entityID);
		}

		public static Entity InstantiatePrefab(Prefab prefab, Vector3 translation)
		{
			ulong entityID = InternalCalls.Scene_InstantiatePrefabWithTranslation(ref prefab.m_Handle, ref translation);
			return entityID == 0 ? null : new Entity(entityID);
		}

		public static Entity InstantiatePrefab(Prefab prefab, Transform transform)
		{
			ulong entityID = InternalCalls.Scene_InstantiatePrefabWithTransform(ref prefab.m_Handle, ref transform.Position, ref transform.Rotation, ref transform.Scale);
			return entityID == 0 ? null : new Entity(entityID);
		}

		public static Entity InstantiatePrefabWithParent(Prefab prefab, Entity parent)
		{
			ulong entityID = InternalCalls.Scene_InstantiatePrefab(ref prefab.m_Handle);
			return entityID == 0 ? null : new Entity(entityID) { Parent = parent };
		}

		public static Entity InstantiatePrefabWithParent(Prefab prefab, Vector3 translation, Entity parent)
		{
			ulong entityID = InternalCalls.Scene_InstantiateChildPrefabWithTranslation(parent.ID, ref prefab.m_Handle, ref translation);
			return entityID == 0 ? null : new Entity(entityID) { Parent = parent };
		}

		public static Entity InstantiatePrefabWithParent(Prefab prefab, Transform transform, Entity parent)
		{
			ulong entityID = InternalCalls.Scene_InstantiateChildPrefabWithTransform(parent.ID, ref prefab.m_Handle, ref transform.Position, ref transform.Rotation, ref transform.Scale);
			return entityID == 0 ? null : new Entity(entityID) { Parent = parent };
		}

		public static Entity[] GetEntities() => InternalCalls.Scene_GetEntities();

		private static void OnEntityDestroyed(Entity entity)
		{
			entity.DestroyedEvent -= OnEntityDestroyed;

			// Remove this entity from the cache if it's been indexed either with the ID or Tag hash code
			if (!s_EntityCache.Remove(entity.ID))
				s_EntityCache.Remove((ulong)entity.Tag.GetHashCode());
		}

		internal static void OnSceneChange()
		{
			foreach (var kv in s_EntityCache)
			{
				if (kv.Value != null)
					kv.Value.DestroyedEvent -= OnEntityDestroyed;
			}

			s_EntityCache.Clear();
		}

		public override bool Equals(object obj) => obj is Scene other && Equals(other);

		public bool Equals(Scene other)
		{
			if (other is null)
				return false;

			if (ReferenceEquals(this, other))
				return true;

			return m_Handle == other.m_Handle;
		}
		private static bool IsValid(Scene scene)
		{
			if(scene is null) 
				return false;
			
			//return InternalCalls.SceneManager_IsSceneHandleValid(scene.m_Handle);
			return scene.m_Handle.IsValid();
		}

		public override int GetHashCode() => m_Handle.GetHashCode();

		public static bool operator ==(Scene sceneA, Scene sceneB) => sceneA is null ? sceneB is null : sceneA.Equals(sceneB);
		public static bool operator !=(Scene sceneA, Scene sceneB) => !(sceneA == sceneB);
		
		public static implicit operator bool(Scene entity) => IsValid(entity);

	}
}
