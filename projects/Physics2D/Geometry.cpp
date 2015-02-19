#include "Geometry.h"
#include <functional>

const glm::mat4 Geometry::NO_ROTATION = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
const glm::quat Geometry::UNROTATED_ORIENTATION = glm::quat(1, 0, 0, 0);

void Geometry::AxisAngle(glm::quat& a_orientation,
						 float a_angle, const glm::vec3& a_axis)
{
	if (glm::vec3(0) == a_axis)
	{
		a_orientation = UNROTATED_ORIENTATION;
		return;
	}
	float angle = a_angle;
	while (angle > glm::pi<float>())
		angle -= glm::pi<float>() * 2;
	while (angle <= -glm::pi<float>())
		angle += glm::pi<float>() * 2;
	a_orientation = glm::quat(glm::cos(angle / 2), glm::normalize(a_axis) * glm::sin(angle / 2));
}
void Geometry::YawPitchRoll(glm::quat& a_orientation,
							float a_yaw, float a_pitch, float a_roll,
							const glm::vec3& a_yawAxis,
							const glm::vec3& a_rollAxis)
{
	if (0 == a_yaw && 0 == a_pitch && 0 == a_roll)
	{
		a_orientation = UNROTATED_ORIENTATION;
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
	a_orientation = AxisAngle(a_yaw, yawAxis) * AxisAngle(a_pitch, pitchAxis) * AxisAngle(a_roll, rollAxis);
}

typedef bool(*CollisionDetector)(const Geometry& a_shape1, const Geometry& a_shape2,
								 Geometry::Collision* a_collision);

static bool Flip(CollisionDetector a_detector,
				 const Geometry& a_shape1, const Geometry& a_shape2,
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

static bool PlanePlane(const Geometry& a_shape1, const Geometry& a_shape2,
					   Geometry::Collision* a_collision);
static bool PlaneSphere(const Geometry& a_shape1, const Geometry& a_shape2,
						Geometry::Collision* a_collision);
static bool PlaneBox(const Geometry& a_shape1, const Geometry& a_shape2,
					 Geometry::Collision* a_collision);

static bool SpherePlane(const Geometry& a_shape1, const Geometry& a_shape2,
						Geometry::Collision* a_collision)
{
	return Flip(PlaneSphere, a_shape1, a_shape2, a_collision);
}
static bool SphereSphere(const Geometry& a_shape1, const Geometry& a_shape2,
						 Geometry::Collision* a_collision);
static bool SphereBox(const Geometry& a_shape1, const Geometry& a_shape2,
					  Geometry::Collision* a_collision);

static bool BoxPlane(const Geometry& a_shape1, const Geometry& a_shape2,
					 Geometry::Collision* a_collision)
{
	return Flip(PlaneBox, a_shape1, a_shape2, a_collision);
}
static bool BoxSphere(const Geometry& a_shape1, const Geometry& a_shape2,
					  Geometry::Collision* a_collision)
{
	return Flip(SphereBox, a_shape1, a_shape2, a_collision);
}
static bool BoxBox(const Geometry& a_shape1, const Geometry& a_shape2,
				   Geometry::Collision* a_collision);

static CollisionDetector g_collisionFunctions[Geometry::TYPE_COUNT][Geometry::TYPE_COUNT] =
{
	{ nullptr, nullptr, nullptr, nullptr },
	{ nullptr, PlanePlane, PlaneSphere, PlaneBox },
	{ nullptr, SpherePlane, SphereSphere, SphereBox },
	{ nullptr, BoxPlane, BoxSphere, BoxBox }
};

bool Geometry::DetectCollision(const Geometry& a_shape1, const Geometry& a_shape2,
							   Geometry::Collision* a_collision)
{
	// validity check
	if (&a_shape1 == &a_shape2 ||
		Geometry::TYPE_COUNT <= a_shape1.GetType() ||
		Geometry::TYPE_COUNT <= a_shape2.GetType())
		return false;

	// call appropriate function
	CollisionDetector f = g_collisionFunctions[a_shape1.GetType()][a_shape2.GetType()];
	if (nullptr == f)
		return false;
	return f(a_shape1, a_shape2, a_collision);
}

bool PlanePlane(const Geometry& a_shape1, const Geometry& a_shape2,
				Geometry::Collision* a_collision)
{
	// type check
	const Geometry::Plane* plane1 = dynamic_cast<const Geometry::Plane*>(&a_shape1);
	const Geometry::Plane* plane2 = dynamic_cast<const Geometry::Plane*>(&a_shape2);
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
		a_collision->shape1(a_shape1);
		a_collision->shape2(a_shape2);
		a_collision->interpenetration = 0;
		glm::vec3 midpoint = (plane1->position + plane2->position) * 0.5f;
		if (glm::vec3(0) == cross)
		{
			a_collision->normal = normal1;
			a_collision->point = midpoint;
		}
		else
		{
			a_collision->normal = glm::normalize(normal1 + (0 > glm::dot(normal1, normal2) ? -normal2 : normal2));
			glm::vec3 p;
			float d1 = glm::dot(plane1->position, normal1);
			float d2 = glm::dot(plane2->position, normal2);
			if (0 != cross.z)
			{
				p.x = (d1*normal2.y - d2*normal1.y) / cross.z;
				p.y = (d2*normal1.x - d1*normal2.x) / cross.z;
				p.z = 0;
			}
			else if (0 != cross.y)
			{
				p.z = (d1*normal2.x - d2*normal1.x) / cross.y;
				p.x = (d2*normal1.z - d1*normal2.z) / cross.y;
				p.y = 0;
			}
			else // 0 != cross.x
			{
				p.y = (d1*normal2.z - d2*normal1.z) / cross.x;
				p.z = (d2*normal1.y - d1*normal2.y) / cross.x;
				p.x = 0;
			}
			cross = glm::normalize(cross);
			a_collision->point = p + cross*glm::dot(cross, midpoint - p);
		}
	}
	return true;
}
bool PlaneSphere(const Geometry& a_shape1, const Geometry& a_shape2,
				 Geometry::Collision* a_collision)
{
	// type check
	const Geometry::Plane* plane = dynamic_cast<const Geometry::Plane*>(&a_shape1);
	const Geometry::Sphere* sphere = dynamic_cast<const Geometry::Sphere*>(&a_shape2);
	if (nullptr == plane || nullptr == sphere)
		return false;

	// sphere and plane collide if distance between <= radius
	glm::vec3 normal = plane->normal();
	glm::vec3 displacement = sphere->position - plane->position;
	float distance = glm::dot(normal, displacement);
	if (0 > distance)
	{
		normal *= -1.0f;
		distance *= -1;
	}
	if (distance <= sphere->radius)
	{
		if (nullptr != a_collision)
		{
			a_collision->shape1(a_shape1);
			a_collision->shape2(a_shape2);
			a_collision->normal = normal;
			a_collision->interpenetration = sphere->radius - distance;
			a_collision->point = sphere->position - a_collision->normal * distance;
		}
		return true;
	}
	return false;
}
void GetMinAndMax(const std::vector<glm::vec3>& a_points,
				  std::function<float(const glm::vec3&)> a_distanceFunction,
				  float& a_min, float& a_max,
				  glm::vec3& a_minPoint, glm::vec3& a_maxPoint)
{
	std::vector<glm::vec3> maxPoints, minPoints;
	bool start = true;
	for (auto point : a_points)
	{
		float distance = a_distanceFunction(point);
		if (start || distance > a_max)
		{
			a_max = distance;
			if (distance != a_max)
				maxPoints.clear();
			maxPoints.push_back(point);
		}
		if (start || distance < a_min)
		{
			a_min = distance;
			if (distance != a_min)
				minPoints.clear();
			minPoints.push_back(point);
		}
		start = false;
	}
	a_minPoint = glm::vec3(0);
	for (auto point : minPoints)
		a_minPoint += point;
	a_minPoint /= (float)(minPoints.size());
	a_maxPoint = glm::vec3(0);
	for (auto point : maxPoints)
		a_maxPoint += point;
	a_maxPoint /= (float)(maxPoints.size());
}

bool PlaneBox(const Geometry& a_shape1, const Geometry& a_shape2,
			  Geometry::Collision* a_collision)
{
	// type check
	const Geometry::Plane* plane = dynamic_cast<const Geometry::Plane*>(&a_shape1);
	const Geometry::Box* box = dynamic_cast<const Geometry::Box*>(&a_shape2);
	if (nullptr == plane || nullptr == box)
		return false;

	// get distances from plane to vertices, with positive distances in the normal
	// direction and negative distances in the opposite direction
	glm::vec3 normal = plane->normal();
	float max, min;
	glm::vec3 minPoint, maxPoint;
	GetMinAndMax(box->vertices(),
				 [&](const glm::vec3& a_point)
				 {
					return glm::dot(normal, a_point - plane->position);
				 },
				 min, max, minPoint, maxPoint);

	// if there are points on both sides, there's an intersection
	if (0 >= max * min)
	{
		if (nullptr != a_collision)
		{
			a_collision->shape1(a_shape1);
			a_collision->shape2(a_shape2);
			a_collision->normal = normal * (fabs(min) > max ? -1.0f : 1.0f);
			a_collision->interpenetration = fmin(fabs(min), max);
			glm::vec3 p = (fabs(min) > max ? maxPoint : minPoint);
			a_collision->point = p - normal * glm::dot(normal, p - plane->position);
		}
		return true;
	}
	return false;
}

bool SphereSphere(const Geometry& a_shape1, const Geometry& a_shape2,
				  Geometry::Collision* a_collision)
{
	// type check
	const Geometry::Sphere* sphere1 = dynamic_cast<const Geometry::Sphere*>(&a_shape1);
	const Geometry::Sphere* sphere2 = dynamic_cast<const Geometry::Sphere*>(&a_shape2);
	if (nullptr == sphere1 || nullptr == sphere2)
		return false;

	// spheres collide if center-center distance is <= sum of radii
	float squareDistance = glm::distance2(sphere1->position, sphere2->position);
	float collisionDistance = sphere1->radius + sphere2->radius;
	if (squareDistance <= collisionDistance*collisionDistance)
	{
		if (nullptr != a_collision)
		{
			a_collision->shape1(a_shape1);
			a_collision->shape2(a_shape2);
			a_collision->normal = glm::normalize(sphere2->position - sphere1->position);
			a_collision->interpenetration = collisionDistance - sqrt(squareDistance);
			float d = sphere1->radius - a_collision->interpenetration / 2;
			a_collision->point = sphere1->position + a_collision->normal * d;
		}
		return true;
	}
	return false;
}
bool SphereBox(const Geometry& a_shape1, const Geometry& a_shape2,
			   Geometry::Collision* a_collision)
{
	// type check
	const Geometry::Sphere* sphere = dynamic_cast<const Geometry::Sphere*>(&a_shape1);
	const Geometry::Box* box = dynamic_cast<const Geometry::Box*>(&a_shape2);
	if (nullptr == sphere || nullptr == box)
		return false;

	// find closest point on box surface
	bool inside = box->Contains(sphere->position);
	glm::vec3 closestPoint = box->ClosestSurfacePointTo(sphere->position);

	// if sphere center is inside box or no more than a radius away from the nearest surface point,
	// then there's a collision.
	if (inside || sphere->Contains(closestPoint))
	{
		if (nullptr != a_collision)
		{
			a_collision->shape1(a_shape1);
			a_collision->shape2(a_shape2);
			a_collision->normal =
				glm::normalize(closestPoint - sphere->position) * (inside ? -1.0f : 1.0f);
			a_collision->interpenetration = sphere->radius +
				(glm::distance(closestPoint, sphere->position) * (inside ? 1 : -1));
			float d = sphere->radius - a_collision->interpenetration / 2;
			a_collision->point = sphere->position + a_collision->normal * d;
		}
		return true;
	}
	return false;
}

// negative value = no overlap
static float ProjectionOverlap(const glm::vec3& a_axis,
							   const std::vector<glm::vec3>& a_group1,
							   const std::vector<glm::vec3>& a_group2,
							   glm::vec3& a_midPoint)
{
	float min1, min2, max1, max2;
	glm::vec3 minPoint1, minPoint2, maxPoint1, maxPoint2;
	auto project = [&](const glm::vec3& a_point)
	{
		return glm::dot(a_axis, a_point);
	};
	GetMinAndMax(a_group1, project, min1, max1, minPoint1, maxPoint1);
	GetMinAndMax(a_group2, project, min2, max2, minPoint2, maxPoint2);
	bool group1First = fabs(max1 - min2) < fabs(max2 - min1);
	a_midPoint = (group1First ? maxPoint1 + minPoint2 : maxPoint2 + minPoint1) * 0.5f;
	return (group1First ? max1 - min2 : max2 - min1);
}

bool BoxBox(const Geometry& a_shape1, const Geometry& a_shape2,
			Geometry::Collision* a_collision)
{
	// type check
	const Geometry::Box* box1 = dynamic_cast<const Geometry::Box*>(&a_shape1);
	const Geometry::Box* box2 = dynamic_cast<const Geometry::Box*>(&a_shape2);
	if (nullptr == box1 || nullptr == box2)
		return false;

	// first check - generalize to sphere to avoid unneccessary calculations
	float d = glm::distance(box1->extents, glm::vec3(0)) +
			  glm::distance(box2->extents, glm::vec3(0));
	if (d*d < glm::distance2(box1->position, box2->position))
		return false;

	// get all the axes to test for separation
	glm::vec3 centerToCenterAxis = glm::normalize(box2->position - box1->position);
	std::vector<glm::vec3> axes;
	axes.push_back(centerToCenterAxis);
	for (unsigned int i = 0; i < 3; ++i)
	{
		axes.push_back(box1->axis(i));
		axes.push_back(box2->axis(i));
		for (unsigned int j = 0; j < 3; ++j)
			axes.push_back(glm::cross(box1->axis(i), box2->axis(j)));
	}

	// test for separation
	auto vertices1 = box1->vertices();
	auto vertices2 = box2->vertices();
	float interpenetration = 0;
	glm::vec3 midPoint;
	glm::vec3 normal;
	bool start = true;
	for (auto axis : axes)
	{
		// test for projection overlap along each axis
		glm::vec3 p;
		float overlap = ProjectionOverlap(axis, vertices1, vertices2, p);

		// if there is no overlap, then there is no collision
		if (0 > overlap)
			return false;

		// store the collision normal that provides the smallest interpenetration
		if (start || overlap < interpenetration)
		{
			interpenetration = overlap;
			normal = (0 > glm::dot(centerToCenterAxis, axis) ? -axis : axis);
			midPoint = p;	// not even close to accurate, but I don't know how to get the right one
		}
		start = false;
	}

	// make sure the collision normal points from the first box to the second
	if (0 > glm::dot(normal, axes[0]))
		normal *= -1;

	// if the projections of each box onto each possible axis always overlap, then the boxes intersect
	if (nullptr != a_collision)
	{
		a_collision->shape1(a_shape1);
		a_collision->shape2(a_shape2);
		a_collision->normal = normal;
		a_collision->interpenetration = interpenetration;
		a_collision->point = midPoint;
	}
	return true;
}

glm::vec3 Geometry::ToWorld(const glm::vec3& a_localCoordinate, bool a_isDirection) const
{
	return (glm::translate(position) * m_rotationMatrix * glm::vec4(a_localCoordinate, a_isDirection ? 0 : 1)).xyz();
}
glm::vec3 Geometry::ToLocal(const glm::vec3& a_worldCoordinate, bool a_isDirection) const
{
	return (glm::inverse(m_rotationMatrix) * glm::translate(-position) * glm::vec4(a_worldCoordinate, a_isDirection ? 0 : 1)).xyz();
}

glm::vec3 Geometry::Box::AxisAlignedExtents() const
{
	glm::vec3 corners[4] = { ToWorld(extents, true),
							 ToWorld(glm::vec3(-extents.x, extents.y, extents.z), true),
							 ToWorld(glm::vec3(extents.x, -extents.y, extents.z), true),
							 ToWorld(glm::vec3(extents.x, extents.y, -extents.z), true) };
	glm::vec3 result(0);
	for (auto corner : corners)
	{
		if (fabs(corner.x) > result.x)
			result.x = fabs(corner.x);
		if (fabs(corner.y) > result.y)
			result.y = fabs(corner.y);
		if (fabs(corner.z) > result.z)
			result.z = fabs(corner.z);
	}
	return result;
}

glm::vec3 Geometry::Plane::ClosestSurfacePointTo(const glm::vec3& a_point,
												 glm::vec3* a_normal) const
{
	float distance = glm::dot(normal(), a_point - position);
	if (nullptr != a_normal)
		*a_normal = normal();
	return a_point - normal()*distance;
}

glm::vec3 Geometry::Box::ClosestSurfacePointTo(const glm::vec3& a_point,
											   glm::vec3* a_normal) const
{
	// rotate into box's coordinate system and calculate distances between point
	// and nearest face in each direction
	glm::vec3 local = ToLocal(a_point);
	glm::vec3 distances = extents - glm::vec3(fabs(local.x), fabs(local.y), fabs(local.z));
	if (0 < distances.x && 0 < distances.y && 0 < distances.z)
	{
		if (distances.x < distances.y && distances.x < distances.z)
			distances.x *= -1;
		else if (distances.y < distances.z)
			distances.y *= -1;
		else
			distances.z *= -1;
	}

	// no normal at corners and edges
	if (nullptr != a_normal)
	{
		if (0 == distances.x && 0 < distances.y && 0 < distances.z)
			*a_normal = ToWorld((0 > local.x ? -1.0f : 1.0f), 0, 0, true);
		else if (0 < distances.x && 0 == distances.y && 0 < distances.z)
			*a_normal = ToWorld(0, (0 > local.y ? -1.0f : 1.0f), 0, true);
		else if (0 < distances.x && 0 < distances.y && 0 == distances.z)
			*a_normal = ToWorld(0, 0, (0 > local.z ? -1.0f : 1.0f), true);
		else
			*a_normal = glm::vec3(0);
	}

	// find closest point on box surface
	return ToWorld((0 > local.x ? -1 : 1) * (extents.x - (0 <= distances.x ? distances.x : 0)),
				   (0 > local.y ? -1 : 1) * (extents.y - (0 <= distances.y ? distances.y : 0)),
				   (0 > local.z ? -1 : 1) * (extents.z - (0 <= distances.z ? distances.z : 0)));
}
glm::vec3 Geometry::Sphere::ClosestSurfacePointTo(const glm::vec3& a_point,
												  glm::vec3* a_normal) const
{
	if (nullptr != a_normal)
		*a_normal = glm::normalize(a_point - position);
	return position + glm::normalize(a_point - position)*radius;
}

std::vector<glm::vec3> Geometry::Box::vertices() const
{
	std::vector<glm::vec3> points;
	points.push_back(ToWorld(extents.x, extents.y, extents.z));
	points.push_back(ToWorld(extents.x, extents.y, -extents.z));
	points.push_back(ToWorld(extents.x, -extents.y, extents.z));
	points.push_back(ToWorld(extents.x, -extents.y, -extents.z));
	points.push_back(ToWorld(-extents.x, extents.y, extents.z));
	points.push_back(ToWorld(-extents.x, extents.y, -extents.z));
	points.push_back(ToWorld(-extents.x, -extents.y, extents.z));
	points.push_back(ToWorld(-extents.x, -extents.y, -extents.z));
	return points;
}

bool Geometry::Box::Contains(const glm::vec3& a_point) const
{
	glm::vec3 local = ToLocal(a_point);
	return (-extents.x <= local.x && local.x <= extents.x &&
			-extents.y <= local.y && local.y <= extents.y &&
			-extents.z <= local.z && local.z <= extents.z);
}

glm::mat3 Geometry::Box::interiaTensorDividedByMass() const
{
	return glm::mat3((extents.y*extents.y + extents.z*extents.z) / 3, 0, 0,
					 0, (extents.x*extents.x + extents.z*extents.z) / 3, 0,
					 0, 0, (extents.x*extents.x + extents.y*extents.y) / 3);
}

glm::mat3 Geometry::Sphere::interiaTensorDividedByMass() const
{
	return glm::mat3(radius * radius * 2 / 3, 0, 0,
					 0, radius * radius * 2 / 3, 0,
					 0, 0, radius * radius * 2 / 3);
}
