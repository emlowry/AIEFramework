#pragma once

#include "Behavior.h"
#include <glm/glm.hpp>

class Agent
{
public:

	Agent() : m_behavior(nullptr), m_position(0), m_target(0) {}
	virtual ~Agent() {}

	const glm::vec3&	getPosition() const	{ return m_position; }
	const glm::vec3&	getTarget() const	{ return m_target; }

	void				setPosition(const glm::vec3& a_pos)	{ m_position = a_pos; }
	void				setTarget(const glm::vec3& a_pos )	{ m_target = a_pos; }

	void				setBehavior(Behavior* a_behavior)	{ m_behavior = a_behavior; }
	Behavior*			getBehavior()						{ return m_behavior; }

	virtual void	update(float a_deltaTime)
	{
		if (nullptr != m_behavior)
			m_behavior->execute(this);
	}

private:

	Behavior*	m_behavior;

	glm::vec3	m_position;
	glm::vec3	m_target;
};