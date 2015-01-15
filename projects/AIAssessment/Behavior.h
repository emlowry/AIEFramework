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

class Selector : public Composite
{
public:

	Selector() {}
	virtual ~Selector() {}

	virtual bool	execute(Agent* a_agent)
	{
		for (unsigned int i = 0; i < m_children.size(); ++i)
		{
			if (m_children[i]->execute(a_agent) == true)
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
		for (unsigned int i = 0; i < m_children.size(); ++i)
		{
			if (m_children[i]->execute(a_agent) == false)
				return false;
		}
		return true;
	}
};