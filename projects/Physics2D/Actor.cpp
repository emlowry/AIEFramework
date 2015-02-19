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
		glm::vec3 v1 = collision.normal * glm::dot(collision.normal, a_actor1->GetVelocity());
		glm::vec3 c1 = a_actor1->GetVelocity() - v1;
		glm::vec3 v2 = collision.normal * glm::dot(collision.normal, a_actor2->GetVelocity());
		glm::vec3 c2 = a_actor2->GetVelocity() - v2;
		float e = fmin(a_actor1->m_material.elasticity, a_actor2->m_material.elasticity);
		if (a_actor1->IsDynamic() && a_actor2->IsDynamic())
		{
			float m1 = a_actor1->GetMass();
			float m2 = a_actor2->GetMass();
			float m = m1 + m2;
			a_actor1->SetVelocity(((v2 - v1) * m2 * e + v1 * m1 + v2 * m2) / m + c1);
			a_actor2->SetVelocity(((v1 - v2) * m1 * e + v2 * m2 + v1 * m1) / m + c2);
			a_actor1->Move(-collision.normal * collision.interpenetration * 0.5f);
			a_actor2->Move(collision.normal * collision.interpenetration * 0.5f);
		}
		else if (a_actor1->IsDynamic())
		{
			a_actor1->SetVelocity((v2 - v1) * e + v2 + c1);
			a_actor1->Move(-collision.normal * collision.interpenetration);
		}
		else if (a_actor2->IsDynamic())
		{
			a_actor2->SetVelocity((v1 - v2) * e + v1 + c2);
			a_actor2->Move(collision.normal * collision.interpenetration);
		}
	}
}
