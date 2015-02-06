#include "Actors.h"
#include "Utilities.h"
#include <vector>

class Scene
{
public:

	Scene(const glm::vec3& a_gravity = glm::vec3(0.0f, -9.81f, 0.0f),
		float a_timeStep = 0.05f)
		: m_gravity(a_gravity), m_timeStep(a_timeStep),
		m_lastUpdate(Utility::getTotalTime()) {}
	~Scene() { ClearActors(); }

	void AddActor(Actor* a_actor) { m_actors.push_back(a_actor); }
	void ClearActors()
	{
		while (!m_actors.empty())
		{
			Actor* actor = m_actors.back();
			m_actors.pop_back();
			if (nullptr != actor)
				delete actor;
		}
	}

	void Update()
	{
		float time = Utility::getTotalTime();
		while (time - m_lastUpdate >= m_timeStep)
		{
			m_lastUpdate += m_timeStep;
			for (auto actor : m_actors)
				actor->Update(m_timeStep, m_gravity);

			for (unsigned int i = 0; i < m_actors.size(); ++i)
			{
				for (unsigned int j = i + 1; j < m_actors.size(); ++j)
					Actor::ResolveCollision(m_actors[i], m_actors[j]);
			}
		}
	}

	void Render()
	{
		for (auto actor : m_actors)
			actor->Render();
	}

	const std::vector<Actor*>& GetActors() { return m_actors; }

protected:

	glm::vec3 m_gravity;
	float m_timeStep;
	float m_lastUpdate;

	std::vector<Actor*> m_actors;

};
