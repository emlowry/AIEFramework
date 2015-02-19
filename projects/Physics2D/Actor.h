#include "Gizmos.h"
#include "Geometry.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

class Actor
{
public:

	struct Material
	{
		float density;
		float elasticity;
		float linearDrag;
		float rotationalDrag;
		float staticFriction;
		float dynamicFriction;
		Material(float a_density = 1.0f, float a_elasticity = 0.99f,
				 float a_linearDrag = 0.01f, float a_rotationalDrag = 0.01f,
				 float a_staticFriction = 0.0f, float a_dynamicFriction = 0.0f)
			: density(a_density), elasticity(a_elasticity),
			  linearDrag(a_linearDrag), rotationalDrag(a_rotationalDrag),
			  staticFriction(a_staticFriction), dynamicFriction(a_dynamicFriction) {}
	};

	static const float MIN_LINEAR_SPEED;
	static const float MIN_ANGULAR_SPEED;

	Actor(const Geometry& a_geometry,
		  const glm::vec4& a_color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f),
		  bool a_dynamic = false,
		  const Material& a_material = Material(),
		  float a_mass = 0.0f,
		  const glm::vec3& a_velocity = glm::vec3(0),
		  const glm::mat3& a_inertiaTensor = glm::mat3(0))
		: m_color(a_color), m_geometry(a_geometry.Clone()), m_dynamic(a_dynamic),
		  m_mass(a_mass), m_velocity(a_velocity), m_force(0),
		  m_material(a_material), m_inertiaTensor(a_inertiaTensor) {}
	Actor(const Geometry& a_geometry,
		  const glm::vec4& a_color,
		  const Material& a_material,
		  float a_mass = 0.0f,
		  const glm::vec3& a_velocity = glm::vec3(0),
		  const glm::mat3& a_inertiaTensor = glm::mat3(0))
		: m_color(a_color), m_geometry(a_geometry.Clone()), m_dynamic(true),
		  m_mass(a_mass), m_velocity(a_velocity), m_force(0),
		  m_material(a_material), m_inertiaTensor(a_inertiaTensor) {}
	~Actor() { delete m_geometry; m_geometry = nullptr; }

	virtual void Update(float a_deltaTime, const glm::vec3& a_gravity = glm::vec3(0));
	virtual void Render()
	{
		m_geometry->Render(m_color);
	}

	const glm::vec4& GetColor() const { return m_color; }
	glm::vec3 GetPosition() const { return m_geometry->position; }
	glm::quat GetOrientation() const { return m_geometry->orientation(); }
	const Geometry& GetGeometry() const { return *m_geometry; }
	Geometry& GetGeometry() { return *m_geometry; }
	const glm::vec3& GetVelocity() const { return m_velocity; }
	const glm::vec3& GetAngularVelocity() const { return m_angularVelocity; }
	float GetMass() const
	{
		return (0 != m_mass ? m_mass : m_material.density * m_geometry->volume());
	}
	float GetRotationalInertia(const glm::vec3& a_axis) const
	{
		glm::vec3 axis = glm::normalize(a_axis);
		return glm::dot(axis, (glm::mat3(0) != m_inertiaTensor ? m_inertiaTensor :
								m_geometry->interiaTensorDividedByMass() * GetMass()) * axis);
	}
	bool IsDynamic() const { return m_dynamic; }

	void SetMass(float a_mass = 0.0f) { m_mass = a_mass; }
	void SetPosition(const glm::vec3& a_position = glm::vec3(0))
	{
		m_geometry->position = a_position;
	}
	void SetOrientation(const glm::quat& a_orientation = glm::quat(0, glm::vec3(0)))
	{
		m_geometry->orientation(a_orientation);
	}
	void SetVelocity(const glm::vec3& a_velocity = glm::vec3(0))
	{
		m_velocity = a_velocity;
	}
	void SetAngularVelocity(const glm::vec3& a_angularVelocity = glm::vec3(0))
	{
		m_angularVelocity = a_angularVelocity;
	}
	void Move(const glm::vec3& a_displacement = glm::vec3(0))
	{
		m_geometry->position += a_displacement;
	}
	void Spin(const glm::vec3& a_rotation = glm::vec3(0))
	{
		m_geometry->spin(a_rotation);
	}
	void Accelerate(const glm::vec3& a_deltaV = glm::vec3(0))
	{
		m_velocity += a_deltaV;
	}
	void AccelerateRotation(const glm::vec3& a_deltaAV = glm::vec3(0))
	{
		m_angularVelocity += a_deltaAV;
	}

	void SpeedCheck()
	{
		if (glm::dot(m_velocity, m_velocity) < MIN_LINEAR_SPEED)
			m_velocity = glm::vec3(0);
		if (glm::dot(m_angularVelocity, m_angularVelocity) < MIN_ANGULAR_SPEED)
			m_angularVelocity = glm::vec3(0);
	}

	static void ResolveCollision(Actor* a_actor1, Actor* a_actor2);

protected:

	glm::vec4 m_color;
	Geometry* m_geometry;
	bool m_dynamic;
	float m_mass;
	glm::mat3 m_inertiaTensor;
	Material m_material;
	glm::vec3 m_velocity;
	glm::vec3 m_angularVelocity;
	glm::vec3 m_force;
	glm::vec3 m_torque;
};
