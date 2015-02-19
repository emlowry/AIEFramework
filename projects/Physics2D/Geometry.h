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
		glm::vec3 point;
		glm::vec3 normal; // points from shape1 to shape 2
		float interpenetration;

		~Collision()
		{
			if (nullptr != m_shape1)
			{
				delete m_shape1;
				m_shape1 = nullptr;
			}
			if (nullptr != m_shape2)
			{
				delete m_shape2;
				m_shape2 = nullptr;
			}
		}

		const Geometry& shape1() const { return *m_shape1; }
		Geometry& shape1() { return *m_shape1; }
		void shape1(const Geometry& a_shape)
		{
			if (nullptr != m_shape1)
				delete m_shape1;
			m_shape1 = a_shape.Clone();
		}
		const Geometry& shape2() const { return *m_shape2; }
		Geometry& shape2() { return *m_shape2; }
		void shape2(const Geometry& a_shape)
		{
			if (nullptr != m_shape2)
				delete m_shape2;
			m_shape2 = a_shape.Clone();
		}

	private:
		Geometry* m_shape1 = nullptr;
		Geometry* m_shape2 = nullptr;
	};

	struct Plane;
	struct Box;
	struct Sphere;

	// static functions
	static void Rotation(glm::quat& a_orientation, const glm::vec3& a_rotation = glm::vec3(0))
	{
		float angle = glm::length(a_rotation);
		AxisAngle(a_orientation, angle, a_rotation);
	}
	static glm::quat Rotation(const glm::vec3& a_rotation = glm::vec3(0))
	{
		glm::quat result;
		Rotation(result, a_rotation);
		return result;
	}
	static void AxisAngle(glm::quat& a_orientation,
						  float a_angle = 0,
						  const glm::vec3& a_axis = glm::vec3(0, 0, 1));
	static glm::quat AxisAngle(float a_angle = 0,
							   const glm::vec3& a_axis = glm::vec3(0, 0, 1))
	{
		glm::quat result;
		AxisAngle(result, a_angle, a_axis);
		return result;
	}
	static void YawPitchRoll(glm::quat& a_orientation,
							 float a_yaw = 0, float a_pitch = 0, float a_roll = 0,
							 const glm::vec3& a_yawAxis = glm::vec3(0, 0, 1),
							 const glm::vec3& a_rollAxis = glm::vec3(0, 0, 1));
	static glm::quat YawPitchRoll(float a_yaw = 0, float a_pitch = 0, float a_roll = 0,
								  const glm::vec3& a_yawAxis = glm::vec3(0, 0, 1),
								  const glm::vec3& a_rollAxis = glm::vec3(0, 0, 1))
	{
		glm::quat result;
		YawPitchRoll(result, a_yaw, a_pitch, a_roll, a_yawAxis, a_rollAxis);
		return result;
	}
	static bool DetectCollision(const Geometry& a_shape1, const Geometry& a_shape2,
								Geometry::Collision* a_collision = nullptr);

	// abstract functions
	virtual glm::vec3 AxisAlignedExtents() const = 0;
	virtual glm::vec3 ClosestSurfacePointTo(const glm::vec3& a_point,
											glm::vec3* a_normal = nullptr) const = 0;
	virtual Geometry* Clone() const = 0;
	virtual bool Contains(const glm::vec3& a_point) const = 0;
	virtual void Render(const glm::vec4& a_color = glm::vec4(0.5f, 0.5f, 0.5f, 1.0f)) = 0;

	// implemented member functions
	Type GetType() const { return type; }
	glm::vec3 ToWorld(const glm::vec3& a_localCoordinate, bool a_isDirection = false) const;
	glm::vec3 ToWorld(float a_localX, float a_localY, float a_localZ, bool a_isDirection = false) const
	{
		return ToWorld(glm::vec3(a_localX, a_localY, a_localZ), a_isDirection);
	}
	glm::vec3 ToLocal(const glm::vec3& a_worldCoordinate, bool a_isDirection = false) const;
	glm::vec3 ToLocal(float a_worldX, float a_worldY, float a_worldZ, bool a_isDirection = false) const
	{
		return ToLocal(glm::vec3(a_worldX, a_worldY, a_worldZ), a_isDirection);
	}

	static const glm::mat4 NO_ROTATION;
	static const glm::quat UNROTATED_ORIENTATION;

	glm::vec3 position;
	virtual float volume() const = 0;
	virtual float area() const = 0;
	virtual glm::mat3 interiaTensorDividedByMass() const = 0;

	const glm::vec3& axis(unsigned int a_index = 2) const { return m_rotationMatrix[a_index % 3].xyz(); }
	const glm::mat4* rotationMatrix() const { return &m_rotationMatrix; }
	const glm::quat& orientation() const { return m_orientation; }
	void orientation(const glm::quat& a_orientation)
	{
		m_orientation = a_orientation;
		m_rotationMatrix = glm::mat4_cast(m_orientation);
	}
	void orientation(const glm::vec3& a_rotation)
	{
		orientation(Rotation(a_rotation));
	}
	void orientation(float a_angle, glm::vec3& a_axis = glm::vec3(0, 0, 1))
	{
		orientation(AxisAngle(a_angle, a_axis));
	}
	void spin(const glm::quat& a_rotation)
	{
		m_orientation *= a_rotation;
		m_rotationMatrix *= glm::mat4_cast(m_orientation);
	}
	void spin(const glm::vec3& a_rotation)
	{
		spin(Rotation(a_rotation));
	}
	void spin(float a_angle, glm::vec3& a_axis = glm::vec3(0, 0, 1))
	{
		spin(AxisAngle(a_angle, a_axis));
	}


protected:

	// constructors are protected, since this is a base class
	Geometry(const glm::vec3& a_position = glm::vec3(0), Type a_type = NONE)
		: position(a_position), type(a_type), m_rotationMatrix(NO_ROTATION),
		  m_orientation(UNROTATED_ORIENTATION) {}
	Geometry(const glm::vec3& a_position,
			 const glm::mat4& a_rotation,
			 Type a_type = NONE)
		: position(a_position), m_rotationMatrix(a_rotation), type(a_type),
		  m_orientation(glm::quat_cast(a_rotation)) {}
	Geometry(const glm::vec3& a_position,
			 const glm::quat& a_orientation,
			 Type a_type = NONE)
		: position(a_position), m_rotationMatrix(glm::mat4_cast(a_orientation)),
		  type(a_type), m_orientation(a_orientation) {}
	Geometry(const glm::vec3& a_position,
			 const glm::vec3& a_forward, const glm::vec3& a_up = glm::vec3(0,0,1),
			 Type a_type = NONE)
		: position(a_position), type(a_type),
		  m_rotationMatrix(glm::orientation(a_forward, a_up))
	{
		m_orientation = glm::quat_cast(m_rotationMatrix);
	}
	Geometry(const glm::vec3& a_position,
			 const glm::vec3& a_axis, float a_angle,
			 Type a_type = NONE)
		: position(a_position), type(a_type)
	{
		AxisAngle(m_orientation, a_angle, a_axis);
		m_rotationMatrix = glm::mat4_cast(m_orientation);
	}
	Geometry(const glm::vec3& a_position,
			 float a_yaw, float a_pitch, float a_roll,
			 const glm::vec3& a_yawAxis,
			 const glm::vec3& a_rollAxis,
			 Type a_type = NONE)
		: position(a_position), type(a_type)
	{
		YawPitchRoll(m_orientation, a_yaw, a_pitch, a_roll, a_yawAxis, a_rollAxis);
		m_rotationMatrix = glm::mat4_cast(m_orientation);
	}
	Geometry(const glm::vec3& a_position,
			 float a_yaw, float a_pitch, float a_roll,
			 Type a_type)
		: position(a_position), type(a_type)
	{
		YawPitchRoll(m_orientation, a_yaw, a_pitch, a_roll);
		m_rotationMatrix = glm::mat4_cast(m_orientation);
	}

private:

	glm::quat m_orientation;
	glm::mat4 m_rotationMatrix;
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
	Plane(unsigned int a_increments, float a_size,
		  const glm::vec3& a_origin,
		  const glm::quat& a_orientation)
		: Geometry(a_origin, a_orientation, PLANE),
		  increments(a_increments), size(a_size) {}

	virtual glm::vec3 AxisAlignedExtents() const
	{
		return glm::vec3(0);	// actually infinite
	}
	virtual glm::vec3 ClosestSurfacePointTo(const glm::vec3& a_point,
											glm::vec3* a_normal = nullptr) const;
	virtual Geometry* Clone() const
	{
		return new Plane(increments, size, position, orientation());
	}
	virtual bool Contains(const glm::vec3& a_point) const
	{
		return (0 == glm::dot(a_point - position, normal()));
	}
	virtual void Render(const glm::vec4& a_color = glm::vec4(0.0f, 1.0f, 0.0f, 1.0f))
	{
		Gizmos::addGrid(position, increments, size, (a_color + glm::vec4(1)) * 0.5f,
						glm::vec4(1), a_color, rotationMatrix());
	}

	glm::vec3 normal() const { return axis(2); }
	glm::vec3 up() const { return axis(1); }
	glm::vec3 right() const { return axis(0); }

	unsigned int increments;
	float size;
	virtual float volume() const { return 0; }
	virtual float area() const { return 0; }
	virtual glm::mat3 interiaTensorDividedByMass() const { return glm::mat3(0); }
};

struct Geometry::Box : public Geometry
{
	Box(const glm::vec3& a_extents = glm::vec3(1),
	    const glm::vec3& a_center = glm::vec3(0))
		: Geometry(a_center, BOX), extents(a_extents) {}
	Box(const glm::vec3& a_extents,
		const glm::vec3& a_center,
		const glm::quat& a_orientation)
		: Geometry(a_center, a_orientation, BOX), extents(a_extents) {}
	Box(const glm::vec3& a_extents,
		const glm::vec3& a_center,
		const glm::vec3& a_forward, const glm::vec3& a_up = glm::vec3(0,0,1))
		: Geometry(a_center, a_forward, a_up, BOX), extents(a_extents) {}
	Box(const glm::vec3& a_extents,
		const glm::vec3& a_center,
		const glm::vec3& a_axis, float a_angle)
		: Geometry(a_center, a_axis, a_angle, BOX), extents(a_extents) {}
	Box(const glm::vec3& a_extents,
		const glm::vec3& a_center,
		float a_yaw, float a_pitch, float a_roll)
		: Geometry(a_center, a_yaw, a_pitch, a_roll, BOX), extents(a_extents) {}
	Box(const glm::vec3& a_extents,
		const glm::vec3& a_center,
		float a_yaw, float a_pitch, float a_roll,
		const glm::vec3& a_yawAxis,
		const glm::vec3& a_rollAxis = glm::vec3(1, 0, 0))
		: Geometry(a_center, a_yaw, a_pitch, a_roll, a_yawAxis, a_rollAxis, BOX), extents(a_extents) {}

	virtual glm::vec3 AxisAlignedExtents() const;
	virtual glm::vec3 ClosestSurfacePointTo(const glm::vec3& a_point,
											glm::vec3* a_normal = nullptr) const;
	virtual Geometry* Clone() const
	{
		return new Box(extents, position, orientation());
	}
	virtual bool Contains(const glm::vec3& a_point) const;
	virtual void Render(const glm::vec4& a_color = glm::vec4(1.0f, 0.0f, 0.0f, 1.0f))
	{
		Gizmos::addAABBFilled(position, extents, a_color, &m_rotationMatrix);
	}

	glm::vec3 size() const { return extents * 2.0f; }
	std::vector<glm::vec3> vertices() const;

	glm::vec3 extents;
	virtual float volume() const { return extents.x * extents.y * extents.z * 8; }
	virtual float area() const
	{
		return (extents.x*extents.y + extents.y*extents.z + extents.z*extents.x) * 2;
	}
	virtual glm::mat3 interiaTensorDividedByMass() const;
};

struct Geometry::Sphere : public Geometry
{
	Sphere(float a_radius = 0.5f,
		   const glm::vec3& a_center = glm::vec3(0))
		   : Geometry(a_center, SPHERE), radius(a_radius) {}
	Sphere(float a_radius,
		   const glm::vec3& a_center,
		   const glm::quat& a_orientation)
		   : Geometry(a_center, a_orientation, SPHERE), radius(a_radius) {}
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

	virtual glm::vec3 AxisAlignedExtents() const
	{
		return glm::vec3(radius);
	}
	virtual glm::vec3 ClosestSurfacePointTo(const glm::vec3& a_point,
											glm::vec3* a_normal = nullptr) const;
	virtual Geometry* Clone() const
	{
		return new Sphere(radius, position, orientation());
	}
	virtual bool Contains(const glm::vec3& a_point) const
	{
		return (glm::distance2(position, a_point) <= radius*radius);
	}
	virtual void Render(const glm::vec4& a_color = glm::vec4(0.0f, 0.0f, 1.0f, 1.0f))
	{
		Gizmos::addSphere(position, radius, 8, 16, a_color, &m_rotationMatrix);
	}

	float radius;
	virtual float volume() const { return radius * radius * radius * glm::pi<float>() * 4 / 3; }
	virtual float area() const { return radius * radius * glm::pi<float>() * 4; }
	virtual glm::mat3 interiaTensorDividedByMass() const;
};
