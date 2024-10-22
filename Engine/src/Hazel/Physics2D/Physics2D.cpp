#include "hzpch.h"
#include "Physics2D.h"
#include "Hazel/Script/ScriptEngine.h"

namespace Hazel {

	std::vector<Raycast2DResult> Physics2D::Raycast(Ref<Scene> scene, const glm::vec2& point0, const glm::vec2& point1)
	{
		class RayCastCallback : public b2RayCastCallback
		{
		public:
			RayCastCallback(std::vector<Raycast2DResult>& results, glm::vec2 p0, glm::vec2 p1)
				: m_Results(results), m_Point0(p0), m_Point1(p1)
			{

			}
			virtual float ReportFixture(b2Fixture* fixture, const b2Vec2& point,
				const b2Vec2& normal, float fraction)
			{
				UUID entityID = *(UUID*)&fixture->GetBody()->GetUserData();
				Entity entity = ScriptEngine::GetSceneContext()->GetEntityWithUUID(entityID);
				float distance = glm::distance(m_Point0, m_Point1) * fraction;
				m_Results.emplace_back(entity, glm::vec2(point.x, point.y), glm::vec2(normal.x, normal.y), distance);
				return 1.0f;
			}
		private:
			std::vector<Raycast2DResult>& m_Results;
			glm::vec2 m_Point0, m_Point1;
		};

		auto& b2dWorld = scene->m_Registry.get<Box2DWorldComponent>(scene->m_SceneEntity).World;
		std::vector<Raycast2DResult> results;
		RayCastCallback callback(results, point0, point1);
		b2dWorld->RayCast(&callback, { point0.x, point0.y }, { point1.x, point1.y });
		return results;
	}

}
