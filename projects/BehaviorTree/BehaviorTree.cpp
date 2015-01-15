#include "BehaviorTree.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#include "Agent.h"
#include "Behavior.h"

// WithinRange
// RandomizeTarget
// SeekTarget

// if WithinRange then RandomiseTarget else SeekTarget

class WithinRange : public Behavior
{
public:

	WithinRange(float a_range) : m_range2(a_range*a_range) {}
	virtual ~WithinRange() {}

	virtual bool	execute(Agent* a_agent)
	{
		float dist2 = glm::distance2(a_agent->getPosition(), a_agent->getTarget());
		if (dist2 <= m_range2)
			return true;
		return false;
	}

	float m_range2;
};

class RandomizeTarget : public Behavior
{
public:

	RandomizeTarget(float a_radius) : m_radius(a_radius) {}
	virtual ~RandomizeTarget() {}

	virtual bool	execute(Agent* a_agent)
	{
		glm::vec3 target(0);
		target.xz = glm::circularRand(m_radius);
		a_agent->setTarget(target);
		return true;
	}

	float m_radius;
};

class SeekTarget : public Behavior
{
public:

	SeekTarget(float a_speed) : m_speed(a_speed) {}
	virtual ~SeekTarget() {}

	virtual bool	execute(Agent* a_agent)
	{
		glm::vec3 pos = a_agent->getPosition();
		glm::vec3 dir = glm::normalize(a_agent->getTarget() - pos);
		a_agent->setPosition(pos + dir * m_speed * Utility::getDeltaTime());
		return true;
	}

	float m_speed;
};

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

BehaviorTree::BehaviorTree()
{

}

BehaviorTree::~BehaviorTree()
{

}

bool BehaviorTree::onCreate(int a_argc, char* a_argv[]) 
{
	// initialise the Gizmos helper class
	Gizmos::create();

	// create a world-space matrix for a camera
	m_cameraMatrix = glm::inverse( glm::lookAt(glm::vec3(10,10,10),glm::vec3(0,0,0), glm::vec3(0,1,0)) );

	// get window dimensions to calculate aspect ratio
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);

	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, width / (float)height, 0.1f, 1000.0f);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	m_agent = new Agent();

	Behavior* seek = new SeekTarget(3);
	Behavior* random = new RandomizeTarget(10);
	Behavior* within = new WithinRange(0.5f);

	Sequence* seq = new Sequence();
	seq->addChild(within);
	seq->addChild(random);

	Selector* root = new Selector();
	root->addChild(seq);
	root->addChild(seek);

	m_behavior = root;

	m_agent->setBehavior(m_behavior);

	return true;
}

void BehaviorTree::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement( m_cameraMatrix, a_deltaTime, 10 );

	// clear all gizmos from last frame
	Gizmos::clear();
	
	// add an identity matrix gizmo
	Gizmos::addTransform( glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1) );

	// add a 20x20 grid on the XZ-plane
	for ( int i = 0 ; i < 21 ; ++i )
	{
		Gizmos::addLine( glm::vec3(-10 + i, 0, 10), glm::vec3(-10 + i, 0, -10), 
						 i == 10 ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1) );
		
		Gizmos::addLine( glm::vec3(10, 0, -10 + i), glm::vec3(-10, 0, -10 + i), 
						 i == 10 ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1) );
	}

	m_agent->update(a_deltaTime);
	
	Gizmos::addAABBFilled(m_agent->getPosition(), glm::vec3(0.5f), glm::vec4(1, 1, 0, 1));
	Gizmos::addAABBFilled(m_agent->getTarget(), glm::vec3(0.1f), glm::vec4(1, 0, 0, 1));

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

void BehaviorTree::onDraw() 
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

void BehaviorTree::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new BehaviorTree();
	
	if (app->create("AIE - BehaviorTree",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}