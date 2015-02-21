#include "Actor.h"

void Actor::Update(float a_deltaTime, const glm::vec3& a_gravity)
{
	if (nullptr != m_geometry)
	{
		// static movement
		Spin(m_angularVelocity * a_deltaTime);
		Move(m_velocity * a_deltaTime);

		if (m_dynamic)
		{
			// linear force
			glm::vec3 force = m_force + a_gravity;
			if (0 < m_material.linearDrag)
				force -= GetVelocity() * m_material.linearDrag;

			// linear acceleration
			float m = GetMass();
			if (glm::vec3(0) != force && 0 != m)
			{
				glm::vec3 deltaV = (force / m) * a_deltaTime;
				Move(0.5f * deltaV * a_deltaTime);
				Accelerate(deltaV);
			}

			// angular force
			glm::vec3 torque = m_torque;
			if (0 < m_material.rotationalDrag)
			{
				torque -= GetAngularVelocity() * m_material.rotationalDrag;
			}

			// angular acceleration
			float i = GetRotationalInertia(torque);
			if (0 != i)
			{
				glm::vec3 deltaAV = torque * a_deltaTime / i;
				Spin(0.5f * deltaAV * a_deltaTime);
				AccelerateRotation(deltaAV);
			}
		}
	}
}

void Actor::ResolveCollision(Actor* a_actor1, Actor* a_actor2)
{
	Geometry::Collision collision;
	if (nullptr != a_actor1 && nullptr != a_actor2 && a_actor1 != a_actor2 &&
		(a_actor1->IsDynamic() || a_actor2->IsDynamic()) &&
		Geometry::DetectCollision(a_actor1->GetGeometry(), a_actor2->GetGeometry(), &collision))
	{
		// resolve interpenetration
		if (a_actor1->IsDynamic() && a_actor2->IsDynamic())
		{
			a_actor1->Move(-collision.normal * collision.interpenetration * 0.5f);
			a_actor2->Move(collision.normal * collision.interpenetration * 0.5f);
		}
		else if (a_actor1->IsDynamic())
		{
			a_actor1->Move(-collision.normal * collision.interpenetration);
			collision.point -= collision.normal * 0.5f;
		}
		else if (a_actor2->IsDynamic())
		{
			a_actor2->Move(collision.normal * collision.interpenetration);
			collision.point += collision.normal * 0.5f;
		}

		glm::vec3 r1 = collision.point - a_actor1->GetPosition();
		glm::vec3 r2 = collision.point - a_actor2->GetPosition();

		glm::vec3 q1 = glm::inverse(a_actor1->GetOrientation()) *
					   (glm::inverse(a_actor1->GetInertiaTensor()) *
						glm::cross(r1, collision.normal));
		glm::vec3 q2 = glm::inverse(a_actor2->GetOrientation()) *
					   (glm::inverse(a_actor2->GetInertiaTensor()) *
						glm::cross(r2, collision.normal));

		float lambda = fmin(a_actor1->m_material.elasticity, a_actor2->m_material.elasticity) * 2.0f *

		/*
		// apply collision force
		glm::vec3 v1 = collision.normal * glm::dot(collision.normal, a_actor1->GetVelocity());
		glm::vec3 v2 = collision.normal * glm::dot(collision.normal, a_actor2->GetVelocity());
		glm::vec3 tangentdv = (a_actor2->GetPointVelocity(collision.normal) - v2) -
							  (a_actor1->GetPointVelocity(collision.normal) - v1);
		float e = fmin(a_actor1->m_material.elasticity, a_actor2->m_material.elasticity) + 1;
		float sf = fmin(a_actor1->m_material.staticFriction, a_actor2->m_material.staticFriction);
		float df = fmin(a_actor1->m_material.dynamicFriction, a_actor2->m_material.dynamicFriction);
		if (a_actor1->IsDynamic() && a_actor2->IsDynamic())
		{
			float m1 = a_actor1->GetMass();
			float m2 = a_actor2->GetMass();
			float m = m1 * m2 / (m1 + m2);
			glm::vec3 f = (v2 - v1) * e * m;
			if (glm::length2(tangentdv * m) > glm::length(f * sf))
			{
				a_actor1->ApplyImpulse(tangentdv * glm::length(f) * df, collision.point);
			}
			a_actor1->ApplyImpulse(f, collision.point);
			a_actor2->ApplyImpulse(-f, collision.point);
		}
		else if (a_actor1->IsDynamic())
		{
			a_actor1->ApplyImpulse((v2 - v1) * e * a_actor1->GetMass(), collision.point);
		}
		else if (a_actor2->IsDynamic())
		{
			a_actor2->ApplyImpulse((v1 - v2) * e * a_actor2->GetMass(), collision.point);
		}/**/

	}
}

glm::vec3 Actor::GetPointVelocity(const glm::vec3& a_point) const
{
	if (!m_geometry->Contains(a_point))
		return glm::vec3(0);
	if (a_point == GetPosition() || glm::vec3(0) == m_angularVelocity)
		return m_velocity;
	return m_velocity + glm::cross(m_angularVelocity, a_point - GetPosition());
}

static bool validImpulse(const glm::vec3& a_vec3, float a_threshold = 0.0001f)
{
	return !(isnan(a_vec3.x) || isnan(a_vec3.y) || isnan(a_vec3.z) ||
			 glm::vec3(0) == a_vec3 || glm::length2(a_vec3) < a_threshold);
}

void Actor::ApplyLinearImpulse(const glm::vec3& a_impulse)
{
	if (validImpulse(a_impulse))
	{
		float m = GetMass();
		if (0 != m)
			Accelerate(a_impulse / m);
	}
}
void Actor::ApplyAngularImpulse(const glm::vec3& a_angularImpulse)
{
	if (validImpulse(a_angularImpulse))
	{
		float i = GetRotationalInertia(a_angularImpulse);
		if (0 != i)
			AccelerateRotation(a_angularImpulse / i);
	}
}
void Actor::ApplyImpulse(const glm::vec3& a_impulse, const glm::vec3& contactPoint)
{
	if (validImpulse(a_impulse))
	{
		glm::vec3 d = GetPosition() - contactPoint;
		if (glm::vec3(0) == d)
		{
			ApplyLinearImpulse(a_impulse);
		}
		else
		{
			glm::vec3 linearImpulse = glm::normalize(a_impulse) *
									  fabs(glm::dot(a_impulse, glm::normalize(d)));
			ApplyLinearImpulse(linearImpulse);
			ApplyAngularImpulse(glm::cross(d, a_impulse - linearImpulse));
		}
	}
}
