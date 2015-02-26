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

		float e = 1.0f + fmin(a_actor1->m_material.elasticity, a_actor2->m_material.elasticity);
		glm::vec3 n = collision.normal;
		glm::vec3 Vr = a_actor2->GetVelocity() - a_actor1->GetVelocity();
		float invM = (a_actor1->IsDynamic() ? 1.0f / a_actor1->GetMass() : 0) +
					 (a_actor2->IsDynamic() ? 1.0f / a_actor2->GetMass() : 0);
		glm::vec3 r1 = collision.point - a_actor1->GetPosition();
		glm::vec3 r2 = collision.point - a_actor2->GetPosition();
		glm::mat3 invI1 = glm::inverse(a_actor1->GetInertiaTensor());
		glm::mat3 invI2 = glm::inverse(a_actor2->GetInertiaTensor());
		glm::vec3 j = -e * n * glm::dot(Vr, n) /
							(invM + glm::dot(n, glm::cross(invI1 * glm::cross(r1, n), r1)) +
							glm::dot(n, glm::cross(invI2 * glm::cross(r2, n), r2)));
		if (a_actor1->IsDynamic())
			a_actor1->ApplyImpulse(-j, collision.point);
		if (a_actor2->IsDynamic())
			a_actor2->ApplyImpulse(j, collision.point);

		/*
		// collision force
		glm::vec3 v1 = collision.normal * glm::dot(collision.normal, a_actor1->GetVelocity());
		glm::vec3 v2 = collision.normal * glm::dot(collision.normal, a_actor2->GetVelocity());
		glm::vec3 dv = v2 - v1;
		//glm::vec3 surfaceDV = (a_actor2->GetPointVelocity(collision.point) - v2) -
		//					  (a_actor1->GetPointVelocity(collision.point) - v1);
		//surfaceDV -= collision.normal * glm::dot(collision.normal, surfaceDV);
		float m = (!a_actor2->IsDynamic() ? a_actor1->GetMass() :
				   !a_actor1->IsDynamic() ? a_actor2->GetMass() :
				   a_actor1->GetMass() * a_actor2->GetMass() / (a_actor1->GetMass() + a_actor2->GetMass()));
		glm::vec3 f = dv * (fmin(a_actor1->m_material.elasticity, a_actor2->m_material.elasticity) + 1) * m;
		if (a_actor1->IsDynamic())
			a_actor1->ApplyImpulse(f, collision.point);
		if (a_actor2->IsDynamic())
			a_actor2->ApplyImpulse(-f, collision.point);

		// friction force
		glm::vec3 surfaceDV = a_actor2->GetPointVelocity(collision.point) -
							  a_actor1->GetPointVelocity(collision.point);
		surfaceDV -= collision.normal * glm::dot(collision.normal, surfaceDV);
		if (glm::length2(surfaceDV * m) >
			glm::length2(f * 0.5f * (a_actor1->m_material.staticFriction +
									 a_actor2->m_material.staticFriction)))
		{
			glm::vec3 friction = -surfaceDV * glm::length(f) * 0.5f * (a_actor1->m_material.dynamicFriction +
																	   a_actor2->m_material.dynamicFriction);
			if (a_actor1->IsDynamic())
				a_actor1->ApplyImpulse(friction, collision.point);
			if (a_actor2->IsDynamic())
				a_actor2->ApplyImpulse(-friction, collision.point);
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
			AccelerateRotation(glm::inverse(GetInertiaTensor()) * a_angularImpulse);
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
			ApplyLinearImpulse(a_impulse);// linearImpulse);
			ApplyAngularImpulse(glm::cross(d, a_impulse/* - linearImpulse/**/));
		}
	}
}