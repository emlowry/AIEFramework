#include "Gizmos.h"
#include "Geometry.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Actor
{
public:

	Actor(Geometry* a_geometry = nullptr,
		  const glm::vec4& a_color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
		  bool a_dynamic = false,
		  float a_mass = 1.0f,
		  const glm::vec3& a_velocity = glm::vec3(0),
		  const glm::vec3& a_force = glm::vec3(0))
		: m_color(a_color), m_geometry(a_geometry), m_dynamic(a_dynamic),
		  m_mass(a_mass), m_velocity(a_velocity), m_force(a_force) {}
	Actor(Geometry* a_geometry,
		  const glm::vec4& a_color,
		  float a_mass,
		  const glm::vec3& a_velocity = glm::vec3(0),
		  const glm::vec3& a_force = glm::vec3(0))
		: m_color(a_color), m_geometry(a_geometry), m_dynamic(true),
		  m_mass(a_mass), m_velocity(a_velocity), m_force(a_force) {}
	~Actor()
	{
		if (nullptr != m_geometry)
			delete m_geometry;
	}

	virtual void Update(float a_deltaTime, const glm::vec3& a_force = glm::vec3(0))
	{
		if (m_dynamic && nullptr != m_geometry)
		{
			m_geometry->position += m_velocity * a_deltaTime;
			glm::vec3 force = m_force + a_force;
			if (glm::vec3(0) != force && m_mass != 0.0f)
			{
				glm::vec3 deltaV = (force / m_mass) * a_deltaTime;
				m_velocity += deltaV;
				m_geometry->position += 0.5f * deltaV * a_deltaTime;
			}
		}
	}
	virtual void Render()
	{
		if (nullptr != m_geometry)
			m_geometry->Render(m_color);
	}

protected:

	glm::vec4 m_color;
	Geometry* m_geometry;
	bool m_dynamic;
	float m_mass;
	glm::vec3 m_velocity;
	glm::vec3 m_force;
};
