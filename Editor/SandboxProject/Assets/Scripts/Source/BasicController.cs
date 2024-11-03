using System;

using Engine;

namespace Example
{
    public class BasicController : Entity
    {
        public float Speed;
        public float DistanceFromPlayer = 20.0F;

        private Entity m_PlayerEntity;

        protected override void OnCreate()
        {
            m_PlayerEntity = Scene.FindEntityByTag("Player");
        }

        protected override void OnUpdate(float ts)
        {
            Vector3 playerTranslation = m_PlayerEntity.Transform.Translation;

            Vector3 translation = Transform.Translation;
            translation.XY = playerTranslation.XY;
            translation.Z = playerTranslation.Z + DistanceFromPlayer;
            translation.Y = Math.Max(translation.Y, 2.0f);
            Transform.Translation = translation;
        }
    }
}
