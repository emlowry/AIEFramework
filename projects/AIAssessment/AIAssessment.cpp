#include "AIAssessment.h"
#include "Gizmos.h"
#include "Utilities.h"
#include "Fuzzy.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <stack>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

const glm::mat4 ROTATE_UP_TRANSFORM = glm::mat4(1, 0, 0, 0,
												0, 0, 1, 0,
												0, -1, 0, 0,
												0, 0, 0, 1);

class ChooseNextTargetOnPath : public Behavior
{
public:

	ChooseNextTargetOnPath(Path* a_path,
						   unsigned int* a_pathIndex,
						   float a_altitude = 0,
						   bool a_loop = false)
		: m_path(a_path), m_pathIndex(a_pathIndex),
		  m_altitude(a_altitude), m_loop(a_loop) {}
	virtual ~ChooseNextTargetOnPath() {}

	virtual bool execute(Agent* a_agent)
	{
		if (nullptr != m_path && nullptr != m_pathIndex && (m_loop || *m_pathIndex + 1 < m_path->size()))
		{
			++(*m_pathIndex);
			if (m_loop) *m_pathIndex %= m_path->size();
			a_agent->setTarget(glm::vec3((*m_path)[*m_pathIndex], m_altitude));
			return true;
		}
		return false;
	}

	Path* m_path;
	unsigned int* m_pathIndex;
	float m_altitude;
	bool m_loop;
};

class ChooseNearestTargetOnPath : public Behavior
{
public:

	ChooseNearestTargetOnPath(Path* a_path, unsigned int* a_pathIndex, float a_altitude = 0)
	: m_path(a_path), m_pathIndex(a_pathIndex), m_altitude(a_altitude) {}
	virtual ~ChooseNearestTargetOnPath() {}

	virtual bool execute(Agent* a_agent)
	{
		if (nullptr != m_path && nullptr != m_pathIndex && 0 < m_path->size())
		{
			float dist1 = glm::distance2(a_agent->getPosition().xy(), (*m_path)[0]);
			unsigned int index = 0;
			for (unsigned int i = 1; i < m_path->size(); ++i)
			{
				float dist2 = glm::distance2(a_agent->getPosition().xy(), (*m_path)[i]);
				if (dist2 < dist1)
				{
					dist1 = dist2;
					index = i;
				}
			}
			*m_pathIndex = index;
			a_agent->setTarget(glm::vec3((*m_path)[index], m_altitude));
			return true;
		}
		return false;
	}

	Path* m_path;
	unsigned int* m_pathIndex;
	float m_altitude;
};

class TargetAgentPosition : public Behavior
{
public:

	TargetAgentPosition(Agent* a_agent) : m_agent(a_agent) {}
	virtual ~TargetAgentPosition() {}

	virtual bool execute(Agent* a_agent)
	{
		if (nullptr == a_agent || nullptr == m_agent)
			return false;
		a_agent->setTarget(glm::vec3(m_agent->getPosition().xy(), a_agent->getPosition().z));
		return true;
	}

	Agent* m_agent;
};

class MoveToTarget : public Behavior
{
public:

	MoveToTarget(float a_speed) : m_speed(fabs(a_speed)) {}
	virtual ~MoveToTarget() {}

	virtual bool execute(Agent* a_agent)
	{
		if (nullptr == a_agent) return false;

		glm::vec3 displacement = a_agent->getPosition() - a_agent->getTarget();
		float distanceSquared = glm::dot(displacement, displacement);
		float move = m_speed*Utility::getDeltaTime();
		if (distanceSquared <= move * move)
		{
			a_agent->setPosition(a_agent->getTarget());
			return true;
		}
		a_agent->setPosition(a_agent->getPosition() - glm::normalize(displacement) * move);
		return false;
	}

	float m_speed;
};

class CanSeeAgent : public Behavior
{
public:

	CanSeeAgent(Agent* a_agent, NavMesh* a_mesh)
		: m_agent(a_agent), m_mesh(a_mesh) {}
	virtual ~CanSeeAgent() {}

	virtual bool execute(Agent* a_agent)
	{
		return m_mesh->lineOfSight(a_agent->getPosition().xy, m_agent->getPosition().xy);
	}

	Agent* m_agent;
	NavMesh* m_mesh;
};

float randFloat(float max = 1.0f, float min = 0.0f)
{
	return ((float)rand() / (float)RAND_MAX) * (max - min) + min;
}

class Wander : public Behavior
{
public:
	Wander(NavMesh* a_mesh, float a_collisionDistance, float a_speed,
		   float a_maxTurn, float a_turnChange)
		: m_mesh(a_mesh), m_collisionDistance(a_collisionDistance),
		  m_speed(a_speed), m_maxTurn(a_maxTurn), m_turnChange(a_turnChange),
		  m_currentAngle(0), m_currentTurn(0) {}
	virtual ~Wander() {}

	virtual bool execute(Agent* a_agent)
	{
		float deltaTime = Utility::getDeltaTime();
		glm::vec3 pos = a_agent->getPosition();

		// adjust turn
		m_currentTurn += (randFloat(1, -1) * m_turnChange * deltaTime);
		if (m_currentTurn > m_maxTurn)
			m_currentTurn = m_maxTurn;
		else if (m_currentTurn < -m_maxTurn)
			m_currentTurn = -m_maxTurn;

		// adjust angle
		m_currentAngle += m_currentTurn * deltaTime;
		glm::vec2 dir = glm::vec2(cosf(m_currentAngle), sinf(m_currentAngle));

		// avoid navmesh edges
		NavMeshTile* tile = m_mesh->getTile(pos.xy());
		if (nullptr == tile) return false;
		float distanceFromRight = tile->rect.topRight.x - pos.x;
		float distanceFromBottom = pos.y - tile->rect.bottomLeft.y;
		float distanceFromLeft = pos.x - tile->rect.bottomLeft.x;
		float distanceFromTop = tile->rect.topRight.y - pos.y;
		if (nullptr == tile->neighbors[Rectangle::RIGHT] &&
			distanceFromRight < m_collisionDistance)
			dir.x -= (m_collisionDistance - distanceFromRight) / m_collisionDistance;
		if (nullptr == tile->neighbors[Rectangle::BOTTOM] &&
			distanceFromBottom < m_collisionDistance)
			dir.y += (m_collisionDistance - distanceFromBottom) / m_collisionDistance;
		if (nullptr == tile->neighbors[Rectangle::LEFT] &&
			distanceFromLeft < m_collisionDistance)
			dir.x += (m_collisionDistance - distanceFromLeft) / m_collisionDistance;
		if (nullptr == tile->neighbors[Rectangle::TOP] &&
			distanceFromTop < m_collisionDistance)
			dir.y -= (m_collisionDistance - distanceFromTop) / m_collisionDistance;

		// save final angle
		if (0.0f != dir.x || 0.0f != dir.y)
			m_currentAngle = atan2f(dir.y, dir.x);

		// update position
		a_agent->setPosition(pos + glm::vec3((glm::normalize(dir) * m_speed * deltaTime), 0));
		return true;
	}

	NavMesh* m_mesh;
	float m_collisionDistance, m_speed, m_maxTurn, m_turnChange, m_currentAngle, m_currentTurn;
};

class ChooseRandomPath : public Behavior
{
public:

	ChooseRandomPath(Path* a_path,
					 unsigned int* a_pathIndex,
					 NavMesh* a_mesh)
		: m_path(a_path), m_pathIndex(a_pathIndex), m_mesh(a_mesh) {}
	virtual ~ChooseRandomPath() {}

	virtual bool execute(Agent* a_agent)
	{
		if (nullptr == a_agent || nullptr == m_mesh ||
			nullptr == m_path || nullptr == m_pathIndex ||
			nullptr == m_mesh->getTile(a_agent->getPosition().xy()))
			return false;

		glm::vec2 end = a_agent->getTarget().xy();
		do
		{
			NavMeshTile* tile = nullptr;
			while (nullptr == tile) tile = (*m_mesh)[rand() % m_mesh->size()];
			end = glm::vec2(randFloat(tile->rect.topRight.x, tile->rect.bottomLeft.x),
							randFloat(tile->rect.topRight.y, tile->rect.bottomLeft.y));
		} while (!m_mesh->calculatePath(a_agent->getPosition().xy(), end, *m_path));
		*m_pathIndex = 0;
		a_agent->setTarget(glm::vec3((*m_path)[0], a_agent->getPosition().z));
		return true;
	}

	NavMesh* m_mesh;
	Path* m_path;
	unsigned int* m_pathIndex;
};

class PathExists : public Behavior
{
public:

	PathExists(Path* a_path) : m_path(a_path) {}
	virtual ~PathExists() {}

	virtual bool execute(Agent* a_agent)
	{
		return 0 < m_path->size();
	}

	Path* m_path;
};

class DestroyPath : public Behavior
{
public:

	DestroyPath(Path* a_path) : m_path(a_path) {}
	virtual ~DestroyPath() {}

	virtual bool execute(Agent* a_agent)
	{
		m_path->clear();
		return true;
	}

	Path* m_path;
};

AIAssessment::AIAssessment()
{

}

AIAssessment::~AIAssessment()
{

}

bool AIAssessment::onCreate(int a_argc, char* a_argv[]) 
{
	// initialise the Gizmos helper class
	Gizmos::create();

	// create a world-space matrix for a camera
	m_cameraMatrix = glm::inverse( glm::lookAt(glm::vec3(10,10,10),glm::vec3(0,0,0), glm::vec3(0,0,1)) );

	// get window dimensions to calculate aspect ratio
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);

	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, width / (float)height, 0.1f, 1000.0f);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// create obstacles
	m_obstacles.push_back(Rectangle(glm::vec2(2.5, 3.5), glm::vec2(0.5f)));
	m_obstacles.push_back(Rectangle(glm::vec2(-5.5, 6.5), glm::vec2(2)));
	m_obstacles.push_back(Rectangle(glm::vec2(-7, -2), glm::vec2(1.5)));
	m_obstacles.push_back(Rectangle(glm::vec2(1, -4), glm::vec2(1)));
	m_obstacles.push_back(Rectangle(glm::vec2(6.5, -3), glm::vec2(0.5f, 5.0f)));
	m_obstacles.push_back(Rectangle(glm::vec2(6, 6.5), glm::vec2(7.5f, 0.25f)));

	// create patrol route
	m_patrol.push_back(glm::vec2(0, 0));
	m_patrol.push_back(glm::vec2(4, 8.5));
	m_patrol.push_back(glm::vec2(-7, 1));
	m_patrol.push_back(glm::vec2(7.5, 3.5));
	m_patrol.push_back(glm::vec2(0.5, -7.5));
	m_patrol.push_back(glm::vec2(-1.5, 6));
	m_patrol.push_back(glm::vec2(-8, -9));

	// create NavMesh
	GenerateNavMesh();

	// lambda function for getting the squared distance between the agents
	auto range = [&] { return glm::distance2(m_pathAgent.getPosition(), m_patrolAgent.getPosition()); };

	// create patrolling agent

	// use fuzzy logic to decide if this agent should chase the other agent
	FuzzyLogic* chase = new FuzzyLogic();

	// if the other agent is close enough, chase it
	Sequence* persuit = new Sequence();
	persuit->addChild(new SetValue<bool>(&m_patrolPersuit, true));
	persuit->addChild(new TargetAgentPosition(&m_pathAgent));
	persuit->addChild(new MoveToTarget(2.0f));
	chase->addRule(persuit,
				   new FuzzyLogic::LeftShoulder(range, 4.0, 16.0),
				   FuzzyLogic::LeftShoulder(1.0, 2.0), 0.5);

	Sequence* patrol = new Sequence();

	// if the other agent is far enough away, don't chase it any more
	Selector* stopPersuit = new Selector();
	stopPersuit->addChild(new CheckValue<bool>(&m_patrolPersuit, false));
	Sequence* resumePatrol = new Sequence();
	resumePatrol->addChild(new SetValue<bool>(&m_patrolPersuit, false));
	resumePatrol->addChild(new ChooseNearestTargetOnPath(&m_patrol, &m_patrolIndex, 1.25f));
	stopPersuit->addChild(resumePatrol);
	patrol->addChild(stopPersuit);

	// move along the patrol route
	patrol->addChild(new MoveToTarget(2.0f));
	patrol->addChild(new ChooseNextTargetOnPath(&m_patrol, &m_patrolIndex, 1.25f, true));

	chase->addRule(patrol,
				   new FuzzyLogic::RightShoulder(range, 16.0, 4.0),
				   FuzzyLogic::RightShoulder(2.0, 1.0), 2.5);

	m_patrolAgent.setBehavior(chase);
	m_patrolAgent.setPosition(glm::vec3(m_patrol[0], 1.25f));
	m_patrolAgent.setTarget(glm::vec3(m_patrol[0], 1.25f));
	m_patrolIndex = 0;
	m_patrolPersuit = false;

	// create random-path-following agent
	Sequence* path = new Sequence();

	// use fuzzy logic to decide if a path is needed
	FuzzyLogic* flee = new FuzzyLogic();
	Selector* randomPath = new Selector();
	randomPath->addChild(new PathExists(&m_path));
	Sequence* makePath = new Sequence();
	makePath->addChild(new CanSeeAgent(&m_patrolAgent, &m_mesh));
	makePath->addChild(new ChooseRandomPath(&m_path, &m_pathIndex, &m_mesh));
	randomPath->addChild(makePath);
	flee->addRule(randomPath,
				  new FuzzyLogic::LeftShoulder(range, 1.0, 4.0),
				  FuzzyLogic::LeftShoulder(1.0, 2.0), 0.5);
	flee->addRule(nullptr,
				  new FuzzyLogic::RightShoulder(range, 4.0, 1.0),
				  FuzzyLogic::RightShoulder(2.0, 1.0), 2.5);
	path->addChild(flee);

	// if no path exists, wander randomly
	Selector* wander = new Selector();
	wander->addChild(new PathExists(&m_path));
	wander->addChild(new Wander(&m_mesh, 0.25, 1.0, 1.5, 50));
	path->addChild(wander);

	// otherwise, follow the path
	path->addChild(new PathExists(&m_path));
	path->addChild(new MoveToTarget(3.0f));
	Selector* finishPath = new Selector();
	finishPath->addChild(new ChooseNextTargetOnPath(&m_path, &m_pathIndex, 0.5f));
	finishPath->addChild(new DestroyPath(&m_path));
	path->addChild(finishPath);

	m_pathIndex = 0;
	m_pathAgent.setBehavior(path);

	return true;
}

void AIAssessment::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement( m_cameraMatrix, a_deltaTime, 10, glm::vec3(0,0,1) );

	// clear all gizmos from last frame
	Gizmos::clear();
	
	// add an identity matrix gizmo
	Gizmos::addTransform( glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1) );

	// add a 20x20 grid on the XZ-plane
	for ( int i = 0 ; i < 21 ; ++i )
	{
		Gizmos::addLine( glm::vec3(-10 + i, 10, 0), glm::vec3(-10 + i, -10, 0), 
						 i == 10 ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1) );
		
		Gizmos::addLine( glm::vec3(10, -10 + i, 0), glm::vec3(-10, -10 + i, 0), 
						 i == 10 ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1) );
	}

	// add obstacles
	for each (auto obstacle in m_obstacles)
	{
		Gizmos::addAABBFilled(glm::vec3(obstacle.center(), 0.5),
							  glm::vec3(obstacle.extents(), 0.5),
							  glm::vec4(0.5f, 0.5f, 0.5f, 1));
	}

	// add agents
	m_patrolAgent.update(a_deltaTime);
	Gizmos::addCylinderFilled(m_patrolAgent.getPosition(), 0.5, 0.25, 8,
							  glm::vec4(0, (m_patrolPersuit ? 1 : 0), 1, 1), &ROTATE_UP_TRANSFORM);
	Gizmos::addLine(glm::vec3(m_patrolAgent.getTarget().xy(), 1.125),
					glm::vec3(m_patrolAgent.getPosition().xy(), 1.125),
					glm::vec4(0, 1, 1, 1));
	m_pathAgent.update(a_deltaTime);
	Gizmos::addCylinderFilled(m_pathAgent.getPosition(), 0.25, 0.5, 8,
							  glm::vec4(1, 0, (0 == m_path.size() ? 1 : 0), 1),
							  &ROTATE_UP_TRANSFORM);
	if (0 < m_path.size())
	{
		Gizmos::addLine(glm::vec3(m_pathAgent.getPosition().xy(), 0.125),
						glm::vec3(m_pathAgent.getTarget().xy(), 0.125),
						glm::vec4(1, 0, 1, 1));
	}

	// add patrol route
	for (unsigned int i = 0; i < m_patrol.size(); ++i)
	{
		Gizmos::addSphere(glm::vec3(m_patrol[i], 1.125), 0.125, 4, 8,
						  glm::vec4(0, (!m_patrolPersuit && i == m_patrolIndex ? 1 : 0), 1, 1));
		Gizmos::addLine(glm::vec3(m_patrol[i], 1.125),
						glm::vec3(m_patrol[(i > 0 ? i : m_patrol.size()) - 1], 1.125),
						glm::vec4(0, 0, 1, 1));
	}

	// add path
	if (0 < m_path.size())
	{
		Gizmos::addSphere(glm::vec3(m_path.back(), 0.125), 0.125, 4, 8, glm::vec4(1, 0, 1, 1));
	}
	for (unsigned int i = 1; i < m_path.size(); ++i)
	{
		Gizmos::addLine(glm::vec3(m_path[i - 1], 0.125),
						glm::vec3(m_path[i], 0.125),
						glm::vec4(1, 0, (i > m_pathIndex ? 1 : 0), 1));
	}

	// add NavMesh
	m_mesh.addGizmos();

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

void AIAssessment::onDraw() 
{
	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse( m_cameraMatrix );
	
	// draw the gizmos from this frame
	Gizmos::draw(m_projectionMatrix, viewMatrix);

	// get window dimensions for 2D orthographic projection
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);
	Gizmos::draw2D(glm::ortho<float>(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f));
}

void AIAssessment::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();
	m_mesh.deleteTiles();
	std::stack<Behavior*> behaviors;
	behaviors.push(m_pathAgent.getBehavior());
	behaviors.push(m_patrolAgent.getBehavior());
	while (!behaviors.empty())
	{
		Behavior* behavior = behaviors.top();
		behaviors.pop();
		Composite* composite = dynamic_cast<Composite*>(behavior);
		if (nullptr != composite)
		{
			while (0 < composite->childCount())
			{
				Behavior* child = composite->lastChild();
				composite->popChild();
				if (nullptr != child)
					behaviors.push(child);
			}
			FuzzyLogic* fuzzy = dynamic_cast<FuzzyLogic*>(behavior);
			if (nullptr != fuzzy)
			{
				fuzzy->destroyRules();
			}
		}
		delete behavior;
	}
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new AIAssessment();
	
	if (app->create("AIE - AIAssessment",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}

void AIAssessment::GenerateNavMesh()
{
	// create tiles
	for (unsigned int i = 0; i < 20; ++i)
	{
		for (unsigned int j = 0; j < 20; ++j)
		{
			NavMeshTile* tile = new NavMeshTile((float)i - 9.5f, (float)j - 9.5f);
			for each (auto obstacle in m_obstacles)
			{
				if (obstacle.intersects(tile->rect))
				{
					delete tile;
					tile = nullptr;
					break;
				}
			}
			if (nullptr != tile)
			{
				m_mesh.push_back(tile);
			}
		}
	}	// create tiles

	m_mesh.linkTiles();
}	// GenerateNavMesh()
