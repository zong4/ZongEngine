using System;
using System.Collections.Generic;
using System.Linq;

namespace Hazel
{
	[EditorAssignable]
	public sealed class Scene : Asset<Scene>
	{
		public static float TimeScale
		{
			set
			{
				unsafe { InternalCalls.Scene_SetTimeScale(value); }
			}
		}

		public static Entity CreateEntity(string tag = "Unnamed")
		{
			unsafe { return new Entity(InternalCalls.Scene_CreateEntity(tag)); }
		}

		public static void DestroyEntity(Entity entity)
		{
			if (entity == null)
				return;

			unsafe
			{
				if (!InternalCalls.Scene_IsEntityValid(entity.ID))
					return;

				// Remove this entity from the cache if it's been indexed either with the ID or Tag hash code
				if (!s_EntityCache.Remove(entity.ID))
					s_EntityCache.Remove((ulong)entity.Tag.GetHashCode());

				InternalCalls.Scene_DestroyEntity(entity.ID);
			}
		}

		public static void DestroyAllChildren(Entity entity)
		{
			if (entity == null)
				return;

			unsafe
			{
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
		}

		private static Dictionary<ulong, Entity?> s_EntityCache = new Dictionary<ulong, Entity?>(50);

		public static Entity? FindEntityByTag(string tag)
		{
			ulong hashCode = (ulong)tag.GetHashCode();

			Entity? result = null;

			unsafe
			{
				if (s_EntityCache.ContainsKey(hashCode) && s_EntityCache[hashCode] != null)
				{
					result = s_EntityCache[hashCode];

					if (!InternalCalls.Scene_IsEntityValid(result!.ID))
					{
						s_EntityCache.Remove(hashCode);
						result = null;
					}
				}

				if (result == null)
				{ 
					ulong entityID = InternalCalls.Scene_FindEntityByTag(tag);
					var newEntity = entityID != 0 ? new Entity(entityID) : null;
					s_EntityCache[hashCode] = newEntity;

					if (newEntity != null)
						newEntity.DestroyedEvent += OnEntityDestroyed;

					result = newEntity;
				}
			}

			return result;
		}

		public static Entity? FindEntityByID(ulong entityID)
		{
			unsafe
			{
				if (s_EntityCache.ContainsKey(entityID) && s_EntityCache[entityID] != null)
				{
					var entity = s_EntityCache[entityID];

					if (!InternalCalls.Scene_IsEntityValid(entity!.ID))
					{
						s_EntityCache.Remove(entityID);
						entity = null;
					}

					return entity;
				}

				if (!InternalCalls.Scene_IsEntityValid(entityID))
					return null;
			}

			var newEntity = new Entity(entityID);
			s_EntityCache[entityID] = newEntity;
			newEntity.DestroyedEvent += OnEntityDestroyed;
			return newEntity;
		}

		public static Entity? InstantiatePrefab(Prefab prefab)
		{
			unsafe
			{
				ulong entityID = InternalCalls.Scene_InstantiatePrefab(prefab.Handle);
				return entityID == 0 ? null : new Entity(entityID);
			}
		}

		public static Entity? InstantiatePrefab(Prefab prefab, Vector3 translation)
		{
			unsafe
			{
				ulong entityID = InternalCalls.Scene_InstantiatePrefabWithTranslation(prefab.Handle, &translation);
				return entityID == 0 ? null : new Entity(entityID);
			}
		}

		public static Entity? InstantiatePrefab(Prefab prefab, Transform transform)
		{
			unsafe
			{
				ulong entityID = InternalCalls.Scene_InstantiatePrefabWithTransform(prefab.Handle, &transform.Position, &transform.Rotation, &transform.Scale);
				return entityID == 0 ? null : new Entity(entityID);
			}
		}

		public static Entity? InstantiatePrefabWithParent(Prefab prefab, Entity parent)
		{
			unsafe
			{
				ulong entityID = InternalCalls.Scene_InstantiatePrefab(prefab.Handle);
				return entityID == 0 ? null : new Entity(entityID) { Parent = parent };
			}
		}

		public static Entity? InstantiatePrefabWithParent(Prefab prefab, Vector3 translation, Entity parent)
		{
			unsafe
			{
				ulong entityID = InternalCalls.Scene_InstantiateChildPrefabWithTranslation(parent.ID, prefab.Handle, &translation);
				return entityID == 0 ? null : new Entity(entityID) { Parent = parent };
			}
		}

		public static Entity? InstantiatePrefabWithParent(Prefab prefab, Transform transform, Entity parent)
		{
			unsafe
			{
				ulong entityID = InternalCalls.Scene_InstantiateChildPrefabWithTransform(parent.ID, prefab.Handle, &transform.Position, &transform.Rotation, &transform.Scale);
				return entityID == 0 ? null : new Entity(entityID) { Parent = parent };
			}
		}

		public static Entity[] GetEntities()
		{
			Entity[] entities;
			
			unsafe
			{
				using var entityIDs = InternalCalls.Scene_GetEntities();
				entities = new Entity[entityIDs.Length];
				for (int i = 0; i < entityIDs.Length; i++)
				{
					entities[i] = new Entity(entityIDs[i]);
				}
			}

			return entities;
		}

		private static void OnEntityDestroyed(Entity entity)
		{
			entity.DestroyedEvent -= OnEntityDestroyed;

			// Remove this entity from the cache if it's been indexed either with the ID or Tag hash code
			if (!s_EntityCache.Remove(entity.ID))
				s_EntityCache.Remove((ulong)entity.Tag.GetHashCode());
		}

		internal static void OnSceneChange()
		{
			foreach (var entity in s_EntityCache.Select(kv => kv.Value))
			{
				if (entity != null)
					entity.DestroyedEvent -= OnEntityDestroyed;
			}

			s_EntityCache.Clear();
		}

	}
}
