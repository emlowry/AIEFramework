#pragma once

#include <vector>

class Agent;

class Behavior
{
public:

	Behavior() {}
	virtual ~Behavior() {}

	virtual bool	execute(Agent* a_agent) = 0;
};

class Composite : public Behavior
{
public:

	Composite() {}
	virtual ~Composite() {}

	void			addChild(Behavior* a_behavior)	{ m_children.push_back(a_behavior); }
	void			popChild()						{ m_children.pop_back(); }
	unsigned int	childCount()					{ return m_children.size(); }
	Behavior*		lastChild()						{ return m_children.back(); }

protected:

	std::vector<Behavior*>	m_children;
};

template<typename T>
class CheckValue : public Behavior
{
public:

	CheckValue(T* a_value, const T& a_desiredValue)
		: m_value(a_value), m_desiredValue(a_desiredValue) {}
	virtual ~CheckValue() {}

	virtual bool execute(Agent* a_agent)
	{
		return (nullptr == m_value ? false : m_desiredValue == *m_value);
	}

	T* m_value;
	T m_desiredValue;
};

template<typename T>
class SetValue : public Behavior
{
public:

	SetValue(T* a_value, const T& a_desiredValue)
		: m_value(a_value), m_desiredValue(a_desiredValue) {}
	virtual ~SetValue() {}

	virtual bool execute(Agent* a_agent)
	{
		if (nullptr == m_value)
			return false;
		*m_value = m_desiredValue;
		return true;
	}

	T* m_value;
	T m_desiredValue;
};

class Selector : public Composite
{
public:

	Selector() {}
	virtual ~Selector() {}

	virtual bool	execute(Agent* a_agent)
	{
		for (auto child : m_children)
		{
			if (child->execute(a_agent) == true)
				return true;
		}
		return false;
	}
};

class Sequence : public Composite
{
public:

	Sequence() {}
	virtual ~Sequence() {}

	virtual bool	execute(Agent* a_agent)
	{
		for (auto child : m_children)
		{
			if (child->execute(a_agent) == false)
				return false;
		}
		return true;
	}
};