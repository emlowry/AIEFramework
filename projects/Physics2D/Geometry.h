#include "Gizmos.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>

struct Geometry
{
public:

	enum Type { NONE, PLANE, SPHERE, BOX };

	struct Plane;
	struct Box;
	struct Sphere;

	virtual void Render(const glm::vec4& a_color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)) = 0;

	Type GetType() const { return type; }

	glm::vec3 position;
	glm::mat4 rotation;
	static const glm::mat4 NO_ROTATION;

protected:

	Geometry(const glm::vec3& a_position = glm::vec3(0),
		const glm::mat4& a_rotation = NO_ROTATION,
		Type a_type = NONE)
		: position(a_position), rotation(a_rotation), type(a_type) {}

	Type type;
};
const glm::mat4 Geometry::NO_ROTATION = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

struct Geometry::Plane : public Geometry
{
	Plane(const glm::vec3& a_normal = glm::vec3(0, 1, 0),
		  const glm::vec3& a_origin = glm::vec3(0),
		  const glm::vec3& a_up = glm::vec3(0, 1, 0),
		  unsigned int a_increments = 20, float a_size = 1.0f)
		: Geometry(a_origin, glm::orientation(a_normal, a_up), PLANE),
		  increments(a_increments), size(a_size) {}

	virtual void Render(const glm::vec4& a_color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f))
	{
		Gizmos::addGrid(position, increments, size, a_color, &rotation);
	}

	unsigned int increments;
	float size;
};

struct Geometry::Box : public Geometry
{
	Box(const glm::vec3& a_size = glm::vec3(1),
		const glm::vec3& a_center = glm::vec3(0),
		const glm::mat4& a_rotation = NO_ROTATION)
		: Geometry(a_center, a_rotation, BOX), size(a_size) {}

	virtual void Render(const glm::vec4& a_color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f))
	{
		Gizmos::addAABBFilled(position, size, a_color, &rotation);
	}

	glm::vec3 size;
};

struct Geometry::Sphere : public Geometry
{
	Sphere(float a_radius = 0.5f,
		   const glm::vec3& a_center = glm::vec3(0),
		   const glm::mat4& a_rotation = NO_ROTATION)
		: Geometry(a_center, a_rotation, SPHERE), radius(a_radius) {}

	virtual void Render(const glm::vec4& a_color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f))
	{
		Gizmos::addSphere(position, radius, 8, 16, a_color, &rotation);
	}

	float radius;
};
