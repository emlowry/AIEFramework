#include "Geometry.h"

const glm::mat4 Geometry::NO_ROTATION = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

void Geometry::YawPitchRoll(glm::mat4& a_rotation,
							float a_yaw, float a_pitch, float a_roll,
							const glm::vec3& a_yawAxis,
							const glm::vec3& a_rollAxis)
{
	if (0 == a_yaw && 0 == a_pitch && 0 == a_roll)
	{
		a_rotation = NO_ROTATION;
		return;
	}
	glm::vec3 yawAxis = a_yawAxis;
	glm::vec3 rollAxis = a_rollAxis;
	glm::vec3 pitchAxis;
	bool correctRollAxis = (0 != glm::dot(yawAxis, rollAxis));
	if (glm::vec3(0) == yawAxis && glm::vec3(0) == rollAxis)
	{
		yawAxis.z = 1.0f;
		rollAxis.x = 1.0f;
		correctRollAxis = false;
	}
	else if (glm::vec3(0) == rollAxis || rollAxis == yawAxis || rollAxis == -yawAxis)
	{
		glm::vec3 projections(glm::dot(yawAxis, glm::vec3(1, 0, 0)),
							  glm::dot(yawAxis, glm::vec3(0, 1, 0)),
							  glm::dot(yawAxis, glm::vec3(0, 0, 1)));
		if (fabs(projections.x) > fabs(projections.z) &&
			fabs(projections.x) > fabs(projections.y))
			rollAxis = glm::cross(glm::vec3(0, 0 > projections.x ? 1 : -1, 0), yawAxis);
		else if (fabs(projections.y) > fabs(projections.z))
			rollAxis = glm::cross(glm::vec3(0 > projections.y ? -1 : 1, 0, 0), yawAxis);
		else
			rollAxis = glm::cross(glm::vec3(0, 0 > projections.z ? -1 : 1, 0), yawAxis);
		correctRollAxis = false;
	}
	else if (glm::vec3(0) == yawAxis)
	{
		glm::vec3 projections(glm::dot(rollAxis, glm::vec3(1, 0, 0)),
							  glm::dot(rollAxis, glm::vec3(0, 1, 0)),
							  glm::dot(rollAxis, glm::vec3(0, 0, 1)));
		if (fabs(projections.y) > fabs(projections.x) &&
			fabs(projections.y) > fabs(projections.z))
			yawAxis = glm::cross(rollAxis, glm::vec3(0 > projections.y ? 1 : -1, 0, 0));
		else if (fabs(projections.z) > fabs(projections.x))
			yawAxis = glm::cross(rollAxis, glm::vec3(0, 0 > projections.z ? -1 : 1, 0));
		else
			yawAxis = glm::cross(rollAxis, glm::vec3(0, 0 > projections.x ? -1 : 1, 0));
		correctRollAxis = false;
	}
	pitchAxis = glm::cross(yawAxis, rollAxis);
	if (correctRollAxis)
		rollAxis = glm::cross(pitchAxis, yawAxis);
	yawAxis = glm::normalize(yawAxis);
	pitchAxis = glm::normalize(pitchAxis);
	rollAxis = glm::normalize(rollAxis);
	a_rotation = glm::rotate(a_yaw, yawAxis) * glm::rotate(a_pitch, pitchAxis) * glm::rotate(a_roll, rollAxis);
}

typedef bool(*CollisionDetector)(Geometry* a_shape1, Geometry* a_shape2,
								 Geometry::Collision* a_collision);

static bool Flip(CollisionDetector a_detector,
				 Geometry* a_shape1, Geometry* a_shape2,
				 Geometry::Collision* a_collision)
{
	bool result = a_detector(a_shape2, a_shape1, a_collision);
	if (result && nullptr != a_collision)
	{
		Geometry* temp = a_collision->shape1;
		a_collision->shape1 = a_collision->shape2;
		a_collision->shape2 = temp;
		a_collision->normal *= -1.0f;
	}
	return result;
}

static bool PlanePlane(Geometry* a_shape1, Geometry* a_shape2,
					   Geometry::Collision* a_collision);
static bool PlaneSphere(Geometry* a_shape1, Geometry* a_shape2,
						Geometry::Collision* a_collision);
static bool PlaneBox(Geometry* a_shape1, Geometry* a_shape2,
					 Geometry::Collision* a_collision);

static bool SpherePlane(Geometry* a_shape1, Geometry* a_shape2,
						Geometry::Collision* a_collision)
{
	return Flip(PlaneSphere, a_shape1, a_shape2, a_collision);
}
static bool SphereSphere(Geometry* a_shape1, Geometry* a_shape2,
						 Geometry::Collision* a_collision);
static bool SphereBox(Geometry* a_shape1, Geometry* a_shape2,
					  Geometry::Collision* a_collision);

static bool BoxPlane(Geometry* a_shape1, Geometry* a_shape2,
					 Geometry::Collision* a_collision)
{
	return Flip(PlaneBox, a_shape1, a_shape2, a_collision);
}
static bool BoxSphere(Geometry* a_shape1, Geometry* a_shape2,
					  Geometry::Collision* a_collision)
{
	return Flip(SphereBox, a_shape1, a_shape2, a_collision);
}
static bool BoxBox(Geometry* a_shape1, Geometry* a_shape2,
				   Geometry::Collision* a_collision);

static CollisionDetector g_collisionFunctions[Geometry::TYPE_COUNT][Geometry::TYPE_COUNT] =
{
	{ nullptr, nullptr, nullptr, nullptr },
	{ nullptr, PlanePlane, PlaneSphere, PlaneBox },
	{ nullptr, SpherePlane, SphereSphere, SphereBox },
	{ nullptr, BoxPlane, BoxSphere, BoxBox }
};

bool Geometry::DetectCollision(Geometry* a_shape1, Geometry* a_shape2,
							   Geometry::Collision* a_collision)
{
	// validity check
	if (nullptr == a_shape1 || nullptr == a_shape2 || a_shape1 == a_shape2 ||
		Geometry::TYPE_COUNT <= a_shape1->GetType() || Geometry::TYPE_COUNT <= a_shape2->GetType())
		return false;

	// call appropriate function
	CollisionDetector f = g_collisionFunctions[a_shape1->GetType()][a_shape2->GetType()];
	if (nullptr == f)
		return false;
	return f(a_shape1, a_shape2, a_collision);
}

bool PlanePlane(Geometry* a_shape1, Geometry* a_shape2,
				Geometry::Collision* a_collision)
{
	// type check
	Geometry::Plane* plane1 = dynamic_cast<Geometry::Plane*>(a_shape1);
	Geometry::Plane* plane2 = dynamic_cast<Geometry::Plane*>(a_shape2);
	if (nullptr == plane1 || nullptr == plane2)
		return false;

	// the only non-colliding planes are parallel planes with distance between them
	glm::vec3 normal1 = plane1->normal();
	glm::vec3 normal2 = plane2->normal();
	glm::vec3 cross = glm::cross(normal1, normal2);
	if (glm::vec3(0) == cross &&
		0 != glm::dot(normal1, plane2->position - plane1->position))
		return false;

	// otherwise, planes always collide
	if (nullptr != a_collision)
	{
		a_collision->shape1 = a_shape1;
		a_collision->shape2 = a_shape2;
		a_collision->interpenetration = 0;
		if (glm::vec3(0) == cross)
			a_collision->normal = normal1;
		else
			a_collision->normal = glm::normalize(normal1 + (0 > glm::dot(normal1, normal2) ? -normal2 : normal2));
	}
	return true;
}
bool PlaneSphere(Geometry* a_shape1, Geometry* a_shape2,
				 Geometry::Collision* a_collision)
{
	// type check
	Geometry::Plane* plane = dynamic_cast<Geometry::Plane*>(a_shape1);
	Geometry::Sphere* sphere = dynamic_cast<Geometry::Sphere*>(a_shape2);
	if (nullptr == plane || nullptr == sphere)
		return false;

	// sphere and plane collide if distance between <= radius
	float distance = fabs(glm::dot(plane->normal(), sphere->position - plane->position));
	if (distance <= sphere->radius)
	{
		if (nullptr != a_collision)
		{
			a_collision->shape1 = a_shape1;
			a_collision->shape2 = a_shape2;
			a_collision->normal = plane->normal();
			a_collision->interpenetration = sphere->radius - distance;
		}
		return true;
	}
	return false;
}
bool PlaneBox(Geometry* a_shape1, Geometry* a_shape2,
			  Geometry::Collision* a_collision)
{
	// type check
	Geometry::Plane* plane = dynamic_cast<Geometry::Plane*>(a_shape1);
	Geometry::Box* box = dynamic_cast<Geometry::Box*>(a_shape2);
	if (nullptr == plane || nullptr == box)
		return false;

	// get distances from plane to vertices, with positive distances in the normal
	// direction and negative distances in the opposite direction
	float max, min;
	auto vertices = box->vertices();
	glm::vec3 normal = plane->normal();
	bool start = true;
	for (auto vertex : vertices)
	{
		float distance = glm::dot(normal, vertex - plane->position);
		if (start)
		{
			max = distance;
			min = distance;
		}
		else
		{
			if (distance > max)
				max = distance;
			if (distance < min)
				min = distance;
		}
	}

	// if there are points on both sides, there's an intersection
	if (0 <= max && 0 >= min)
	{
		if (nullptr != a_collision)
		{
			a_collision->shape1 = a_shape1;
			a_collision->shape2 = a_shape2;
			a_collision->normal = normal * (fabs(min) > max ? -1.0f : 1.0f);
			a_collision->interpenetration = fmin(fabs(min), max);
		}
		return true;
	}
	return false;
}

bool SphereSphere(Geometry* a_shape1, Geometry* a_shape2,
				  Geometry::Collision* a_collision)
{
	// type check
	Geometry::Sphere* sphere1 = dynamic_cast<Geometry::Sphere*>(a_shape1);
	Geometry::Sphere* sphere2 = dynamic_cast<Geometry::Sphere*>(a_shape2);
	if (nullptr == sphere1 || nullptr == sphere2)
		return false;

	// spheres collide if center-center distance is <= sum of radii
	float squareDistance = glm::distance2(sphere1->position, sphere2->position);
	float collisionDistance = sphere1->radius + sphere2->radius;
	if (squareDistance <= collisionDistance*collisionDistance)
	{
		if (nullptr != a_collision)
		{
			a_collision->shape1 = a_shape1;
			a_collision->shape2 = a_shape2;
			a_collision->normal = glm::normalize(sphere2->position - sphere1->position);
			a_collision->interpenetration = collisionDistance - sqrt(squareDistance);
		}
		return true;
	}
	return false;
}
bool SphereBox(Geometry* a_shape1, Geometry* a_shape2,
			   Geometry::Collision* a_collision)
{
	// type check
	Geometry::Sphere* sphere = dynamic_cast<Geometry::Sphere*>(a_shape1);
	Geometry::Box* box = dynamic_cast<Geometry::Box*>(a_shape2);
	if (nullptr == sphere || nullptr == box)
		return false;

	// TODO
	return false;
}

bool BoxBox(Geometry* a_shape1, Geometry* a_shape2,
			Geometry::Collision* a_collision)
{
	// type check
	Geometry::Box* box1 = dynamic_cast<Geometry::Box*>(a_shape1);
	Geometry::Box* box2 = dynamic_cast<Geometry::Box*>(a_shape2);
	if (nullptr == box1 || nullptr == box2)
		return false;

	// first check - generalize to sphere to avoid unneccessary calculations
	float d = box1->size.length() + box2->size.length();
	if (d*d < glm::distance2(box1->position, box2->position))
		return false;

	// rotate second box into coordinate system of first
	glm::mat4 inv = glm::inverse(box1->rotation);
	Geometry::Box relBox2(box2->size, (inv * glm::vec4(box2->position, 1)).xyz());
	relBox2.rotation = inv * box2->rotation;

	// TODO
	return false;
}

std::vector<glm::vec3> Geometry::Box::vertices() const
{
	std::vector<glm::vec3> points;
	points.push_back(position + (rotation * glm::vec4(size.x, size.y, size.z, 0) * 0.5f).xyz());
	points.push_back(position + (rotation * glm::vec4(size.x, size.y, -size.z, 0) * 0.5f).xyz());
	points.push_back(position + (rotation * glm::vec4(size.x, -size.y, size.z, 0) * 0.5f).xyz());
	points.push_back(position + (rotation * glm::vec4(size.x, -size.y, -size.z, 0) * 0.5f).xyz());
	points.push_back(position + (rotation * glm::vec4(-size.x, size.y, size.z, 0) * 0.5f).xyz());
	points.push_back(position + (rotation * glm::vec4(-size.x, size.y, -size.z, 0) * 0.5f).xyz());
	points.push_back(position + (rotation * glm::vec4(-size.x, -size.y, size.z, 0) * 0.5f).xyz());
	points.push_back(position + (rotation * glm::vec4(-size.x, -size.y, -size.z, 0) * 0.5f).xyz());
	return points;
}
