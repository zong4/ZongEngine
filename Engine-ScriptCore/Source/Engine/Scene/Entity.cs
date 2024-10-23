using System;
using System.Collections.Generic;

namespace Hazel
{
	public class Entity : IEquatable<Entity>
	{
		public event Action<Entity> CollisionBeginEvent;
		public event Action<Entity> CollisionEndEvent;
		public event Action<Entity> Collision2DBeginEvent;
		public event Action<Entity> Collision2DEndEvent;
		public event Action<Entity> TriggerBeginEvent;
		public event Action<Entity> TriggerEndEvent;
		public event Action<Entity> DestroyedEvent;

		public event Action<Vector3, Vector3> JointBreakEvent;

		protected event Action<Identifier> AnimationEvent;

		private Entity m_Parent;
		private Dictionary<Type, Component> m_ComponentCache = new Dictionary<Type, Component>();
		private TransformComponent m_TransformComponent;

		protected Entity() { ID = 0; }

		internal Entity(ulong id)
		{
			ID = id;
		}

		public readonly ulong ID;
		public string Tag => GetComponent<TagComponent>().Tag;
		public TransformComponent Transform
		{
			get
			{
				if (m_TransformComponent == null)
					m_TransformComponent = GetComponent<TransformComponent>();

				return m_TransformComponent;
			}
		}

		public Vector3 Translation
		{
			get => Transform.Translation;
			set => Transform.Translation = value;
		}

		public Vector3 Rotation
		{
			get => Transform.Rotation;
			set => Transform.Rotation = value;
		}

		public Vector3 Scale
		{
			get => Transform.Scale;
			set => Transform.Scale = value;
		}

		public Entity Parent
		{
			get
			{
				// NOTE(Peter): This just to reduce heap allocations!
				ulong parentID = InternalCalls.Entity_GetParent(ID);
				if (m_Parent == null || m_Parent.ID != parentID)
				{
					m_Parent = InternalCalls.Scene_IsEntityValid(parentID)
						? new Entity(parentID) : null;
				}

				return m_Parent;
			}

			set => InternalCalls.Entity_SetParent(ID, value != null ? value.ID : 0);
		}

		public Entity[] Children => InternalCalls.Entity_GetChildren(ID);

		protected virtual void OnCreate() { }
		protected virtual void OnUpdate(float ts) { }
		protected virtual void OnLateUpdate(float ts) { }
		protected virtual void OnPhysicsUpdate(float ts) { }
		protected virtual void OnDestroy() { }

		public void SetRotation(Quaternion rotation)
		{
			GetComponent<TransformComponent>().SetRotation(rotation);
		}

		public T CreateComponent<T>() where T : Component, new()
		{
			if (HasComponent<T>())
				return GetComponent<T>();

			Type componentType = typeof(T);
			InternalCalls.Entity_CreateComponent(ID, componentType);
			T component = new T { Entity = this };
			m_ComponentCache.Add(componentType, component);
			return component;
		}

		public bool HasComponent<T>() where T : Component => InternalCalls.Entity_HasComponent(ID, typeof(T));
		public bool HasComponent(Type type) => InternalCalls.Entity_HasComponent(ID, type);
		public T GetComponent<T>() where T : Component, new()
		{
			Type componentType = typeof(T);

			if (!HasComponent<T>())
			{
				if (m_ComponentCache.ContainsKey(componentType))
					m_ComponentCache.Remove(componentType);

				return null;
			}

			if (!m_ComponentCache.ContainsKey(componentType))
			{
				T component = new T { Entity = this };
				m_ComponentCache.Add(componentType, component);
				return component;
			}

			return m_ComponentCache[componentType] as T;
		}

		public bool RemoveComponent<T>() where T : Component
		{
			Type componentType = typeof(T);
			bool removed = InternalCalls.Entity_RemoveComponent(ID, componentType);

			if (removed && m_ComponentCache.ContainsKey(componentType))
				m_ComponentCache.Remove(componentType);

			return removed;
		}

		public Entity FindEntityByTag(string tag) => Scene.FindEntityByTag(tag);
		public Entity FindEntityByID(ulong entityID) => Scene.FindEntityByID(entityID);

		public Entity Instantiate(Prefab prefab) => Scene.InstantiatePrefab(prefab);
		public Entity Instantiate(Prefab prefab, Vector3 translation) => Scene.InstantiatePrefab(prefab, translation);
		public Entity Instantiate(Prefab prefab, Transform transform) => Scene.InstantiatePrefab(prefab, transform);

		public Entity InstantiateChild(Prefab prefab) => Scene.InstantiatePrefabWithParent(prefab, this);
		public Entity InstantiateChild(Prefab prefab, Vector3 translation) => Scene.InstantiatePrefabWithParent(prefab, translation, this);
		public Entity InstantiateChild(Prefab prefab, Transform transform) => Scene.InstantiatePrefabWithParent(prefab, transform, this);

		/// <summary>
		/// Returns true if it possbile to treat this entity as if it were of type T,
		/// returns false otherwise.
		/// <returns></returns>
		public bool Is<T>() where T : Entity
		{
			if (!HasComponent<ScriptComponent>())
				return false;

			ScriptComponent sc = GetComponent<ScriptComponent>();
			object instance = sc?.Instance;
			return instance is T;
		}

		/// <summary>
		/// Returns this entity cast to type T if that is possible, otherwise returns null.
		/// <returns></returns>
		public T As<T>() where T : Entity
		{
			ScriptComponent sc = GetComponent<ScriptComponent>();
			object instance = sc?.Instance;
			return instance as T;
		}

		// Destroys the calling Entity
		public void Destroy() => Scene.DestroyEntity(this);
		public void Destroy(Entity other) => Scene.DestroyEntity(other);
		public void DestroyAllChildren() => Scene.DestroyAllChildren(this); 

		[Obsolete("Obsolete and only here for backwards compatibility. Use Collision2DBeginEvent += MyEvent; instead!", false)]
		public void AddCollision2DBeginCallback(Action<Entity> callback) => Collision2DBeginEvent += callback;
		[Obsolete("Obsolete and only here for backwards compatibility. Use Collision2DEndEvent += MyEvent; instead!", false)]
		public void AddCollision2DEndCallback(Action<Entity> callback) => Collision2DEndEvent += callback;
		[Obsolete("Obsolete and only here for backwards compatibility. Use CollisionBeginEvent += MyEvent; instead!", false)]
		public void AddCollisionBeginCallback(Action<Entity> callback) => CollisionBeginEvent += callback;
		[Obsolete("Obsolete and only here for backwards compatibility. Use CollisionEndEvent += MyEvent; instead!", false)]
		public void AddCollisionEndCallback(Action<Entity> callback) => CollisionEndEvent += callback;
		[Obsolete("Obsolete and only here for backwards compatibility. Use TriggerBeginEvent += MyEvent; instead!", false)]
		public void AddTriggerBeginCallback(Action<Entity> callback) => TriggerBeginEvent += callback;
		[Obsolete("Obsolete and only here for backwards compatibility. Use TriggerEndEvent += MyEvent; instead!", false)]
		public void AddTriggerEndCallback(Action<Entity> callback) => TriggerEndEvent += callback;

		private void OnCollisionBeginInternal(ulong id) => CollisionBeginEvent?.Invoke(new Entity(id));
		private void OnCollisionEndInternal(ulong id) => CollisionEndEvent?.Invoke(new Entity(id));

		private void OnTriggerBeginInternal(ulong id) => TriggerBeginEvent?.Invoke(new Entity(id));
		private void OnTriggerEndInternal(ulong id) => TriggerEndEvent?.Invoke(new Entity(id));

		private void OnCollision2DBeginInternal(ulong id) => Collision2DBeginEvent?.Invoke(new Entity(id));
		private void OnCollision2DEndInternal(ulong id) => Collision2DEndEvent?.Invoke(new Entity(id));

		private void OnJointBreakInternal(Vector3 linearForce, Vector3 angularForce) => JointBreakEvent?.Invoke(linearForce, angularForce);

		private void OnAnimationEventInternal(Identifier eventID) => AnimationEvent?.Invoke(eventID);
		
		private void OnDestroyInternal()
		{
			DestroyedEvent?.Invoke(this);
			OnDestroy();
			m_ComponentCache.Clear();
		}

		public override bool Equals(object obj) => obj is Entity other && Equals(other);

		// NOTE(Peter): Implemented according to Microsofts official documentation:
		// https://docs.microsoft.com/en-us/dotnet/csharp/programming-guide/statements-expressions-operators/how-to-define-value-equality-for-a-type
		public bool Equals(Entity other)
		{
			if (other is null)
				return false;

			if (ReferenceEquals(this, other))
				return true;

			return ID == other.ID;
		}
		private static bool IsValid(Entity entity)
		{
			if(entity is null) 
				return false;
			
			return InternalCalls.Scene_IsEntityValid(entity.ID);
		}

		public override int GetHashCode() => (int)ID;

		public static bool operator ==(Entity entityA, Entity entityB) => entityA is null ? entityB is null : entityA.Equals(entityB);
		public static bool operator !=(Entity entityA, Entity entityB) => !(entityA == entityB);
		
		public static implicit operator bool(Entity entity) => IsValid(entity);

	}
}
