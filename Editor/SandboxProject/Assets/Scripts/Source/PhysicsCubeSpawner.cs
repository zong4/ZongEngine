using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

using Engine;

namespace PhysicsStress
{
    public class PhysicsCubeSpawner : Entity
    {

        public Prefab PhysicsCube;

        private int m_CubeCounter = 0;

        private void SpawnCube(Vector3 translation)
        {
            Transform transform = new Transform();
            transform.Position = translation;
            transform.Scale = new Vector3(0.25f);
            Scene.InstantiatePrefab(PhysicsCube, transform);
            m_CubeCounter++;
        }

        protected override void OnCreate()
        {
            int levels = 20;
            int size = 50;
            float cubeSize = 0.25f;

            float xzOffset = -size * 0.5f * cubeSize;
            float yOffset = 2.0f;
            for (int y = 0; y < levels; y++)
            {
                for (int x = 0; x < size; x++)
                {
                    for (int z = 0; z < size; z++)
                    {
                        SpawnCube(new Vector3(x * cubeSize + xzOffset, y * cubeSize + yOffset, z * cubeSize + xzOffset));
                        if (x == 0 || x == size - 1)
                            ;
                        else
                            z += size - 2;

                    }

                }

            }

            Log.Warn($"Spawned {m_CubeCounter} cubes!");
           
        }
    }
}
