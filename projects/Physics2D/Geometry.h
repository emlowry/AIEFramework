#include "Gizmos.h"
#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <vector>

struct Geometry
{
public:

	enum Type
	{
		NONE = 0,
		PLANE = 1,
		SPHERE = 2,
		BOX = 3,

		TYPE_COUNT = 4
	};

	struct Collision
	{
		Geometry* shape1;
		Geometry* shape2;
		glm::vec3 normal; // points from shape1 to shape 2
		float interpenetration;
	};

	struct Plane;
	struct Box;
	struct Sphere;

	virtual void Render(const glm::vec4& a_color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)) = 0;

	Type GetType() const { return type; }

	static bool DetectCollision(Geometry* a_shape1, Geometry* a_shape2,
								Geometry::Collision* a_collision = nullptr);

	static void YawPitchRoll(glm::mat4& a_rotation,
							 float a_yaw = 0, float a_pitch = 0, float a_roll = 0,
							 const glm::vec3& a_yawAxis = glm::vec3(0, 0, 1),
							 const glm::vec3& a_rollAxis = glm::vec3(0, 0, 1));
	static glm::mat4 YawPitchRoll(float a_yaw = 0, float a_pitch = 0, float a_roll = 0,
								  const glm::vec3& a_yawAxis = glm::vec3(0, 0, 1),
								  const glm::vec3& a_rollAxis = glm::vec3(0, 0, 1))
	{
		glm::mat4 result;
		YawPitchRoll(result, a_yaw, a_pitch, a_roll, a_yawAxis, a_rollAxis);
		return result;
	}

	glm::vec3 position;
	glm::mat4 rotation;
	static const glm::mat4 NO_ROTATION;

protected:

	Geometry(const glm::vec3& a_position = glm::vec3(0), Type a_type = NONE)
		: position(a_position), type(a_type), rotation(NO_ROTATION) {}
	Geometry(const glm::vec3& a_position,
			 const glm::mat4& a_rotation,
			 Type a_type = NONE)
		: position(a_position), rotation(a_rotation), type(a_type) {}
	Geometry(const glm::vec3& a_position,
			 const glm::vec3& a_forward, const glm::vec3& a_up = glm::vec3(0,0,1),
			 Type a_type = NONE)
		: position(a_position), type(a_type),
		  rotation(glm::orientation(a_forward, a_up)) {}
	Geometry(const glm::vec3& a_position,
			 const glm::vec3& a_axis, float a_angle,
			 Type a_type = NONE)
		: position(a_position), type(a_type),
		  rotation(0 == a_angle || glm::vec3(0) == a_axis ? NO_ROTATION
					: glm::rotate(a_angle, a_axis)) {}
	Geometry(const glm::vec3& a_position,
			 float a_yaw, float a_pitch, float a_roll,
			 const glm::vec3& a_yawAxis,
			 const glm::vec3& a_rollAxis,
			 Type a_type = NONE)
		: position(a_position), type(a_type)
	{
		YawPitchRoll(rotation, a_yaw, a_pitch, a_roll, a_yawAxis, a_rollAxis);
	}
	Geometry(const glm::vec3& a_position,
			 float a_yaw, float a_pitch, float a_roll,
			 Type a_type)
		: position(a_position), type(a_type)
	{
		YawPitchRoll(rotation, a_yaw, a_pitch, a_roll);
	}

	Type type;
};

struct Geometry::Plane : public Geometry
{
	Plane(unsigned int a_increments = 20, float a_size = 1.0f,
		  const glm::vec3& a_origin = glm::vec3(0),
		  const glm::vec3& a_normal = glm::vec3(0, 0, 1),
		  const glm::vec3& a_verticalAxis = glm::vec3(0, 1, 0))
		: Geometry(a_origin, glm::orientation(a_normal, a_verticalAxis), PLANE),
		  increments(a_increments), size(a_size) {}

	virtual void Render(const glm::vec4& a_color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f))
	{
		Gizmos::addGrid(position, increments, size, a_color, &rotation);
	}

	glm::vec3 normal() const { return rotation[1].xyz(); }
	glm::vec3 up() const { return rotation[2].xyz(); }
	glm::vec3 right() const { return rotation[0].xyz(); }

	unsigned int increments;
	float size;
};

struct Geometry::Box : public Geometry
{
	Box(const glm::vec3& a_size = glm::vec3(1),
	    const glm::vec3& a_center = glm::vec3(0))
		: Geometry(a_center, BOX), size(a_size) {}
	Box(const glm::vec3& a_size,
		const glm::vec3& a_center,
		const glm::vec3& a_forward, const glm::vec3& a_up = glm::vec3(0,0,1))
		: Geometry(a_center, a_forward, a_up, BOX), size(a_size) {}
	Box(const glm::vec3& a_size,
		const glm::vec3& a_center,
		const glm::vec3& a_axis, float a_angle)
		: Geometry(a_center, a_axis, a_angle, BOX), size(a_size) {}
	Box(const glm::vec3& a_size,
		const glm::vec3& a_center,
		float a_yaw, float a_pitch, float a_roll)
		: Geometry(a_center, a_yaw, a_pitch, a_roll, BOX), size(a_size) {}
	Box(const glm::vec3& a_size,
		const glm::vec3& a_center,
		float a_yaw, float a_pitch, float a_roll,
		const glm::vec3& a_yawAxis,
		const glm::vec3& a_rollAxis = glm::vec3(1, 0, 0))
		: Geometry(a_center, a_yaw, a_pitch, a_roll, a_yawAxis, a_rollAxis, BOX), size(a_size) {}

	virtual void Render(const glm::vec4& a_color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f))
	{
		Gizmos::addAABBFilled(position, size, a_color, &rotation);
	}

	std::vector<glm::vec3> vertices() const;

	glm::vec3 size;
};

struct Geometry::Sphere : public Geometry
{
	Sphere(float a_radius = 0.5f,
		   const glm::vec3& a_center = glm::vec3(0))
		   : Geometry(a_center, SPHERE), radius(a_radius) {}
	Sphere(float a_radius,
		   const glm::vec3& a_center,
		   const glm::vec3& a_forward, const glm::vec3& a_up = glm::vec3(0, 0, 1))
		: Geometry(a_center, a_forward, a_up, SPHERE), radius(a_radius) {}
	Sphere(float a_radius,
		   const glm::vec3& a_center,
		   const glm::vec3& a_axis, float a_angle)
		: Geometry(a_center, a_axis, a_angle, SPHERE), radius(a_radius) {}
	Sphere(float a_radius,
		   const glm::vec3& a_center,
		   float a_yaw, float a_pitch, float a_roll)
		: Geometry(a_center, a_yaw, a_pitch, a_roll, SPHERE), radius(a_radius) {}
	Sphere(float a_radius,
		   const glm::vec3& a_center,
		   float a_yaw, float a_pitch, float a_roll,
		   const glm::vec3& a_yawAxis,
		   const glm::vec3& a_rollAxis = glm::vec3(1, 0, 0))
		: Geometry(a_center, a_yaw, a_pitch, a_roll, a_yawAxis, a_rollAxis, SPHERE), radius(a_radius) {}

	virtual void Render(const glm::vec4& a_color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f))
	{
		Gizmos::addSphere(position, radius, 8, 16, a_color, &rotation);
	}

	float radius;
};
