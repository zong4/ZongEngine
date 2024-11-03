using Engine;

namespace Example
{
    class RandomColor : Entity
    {
        protected override void OnCreate()
        {
            MeshComponent meshComponent = GetComponent<MeshComponent>();
            Material material = meshComponent.Mesh.GetMaterial(0);
            float r = Random.Float();
            float g = Random.Float();
            float b = Random.Float();
            material.AlbedoColor = new Vector3(r, g, b);
        }

    }
}
