#include "Tutorial1.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

Tutorial1::Tutorial1()
{

}

Tutorial1::~Tutorial1()
{

}

bool Tutorial1::onCreate(int a_argc, char* a_argv[]) 
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

	// initialize rotating matrix and moveable object matrix
	m_rotatingMatrix = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);
	m_moveMatrix = glm::mat4(1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	return true;
}

// move an object based on keyboard arrow key input
void Tutorial1::moveObject(glm::mat4& a_transform, float a_deltaTime, float a_speed)
{
	// calculate how much to transform
	float frameSpeed = a_deltaTime * a_speed;

	// translate
	if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
		a_transform[3] += a_transform[2] * frameSpeed;
	if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
		a_transform[3] -= a_transform[2] * frameSpeed;

	// rotate
	int iRotate = (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) ? 1 : 0;
	if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
		iRotate -= 1;
	if (iRotate != 0)
	{
		glm::mat4 mMat = glm::axisAngleMatrix(a_transform[1].xyz(), frameSpeed * iRotate);
		a_transform[0] = mMat * a_transform[0];
		a_transform[1] = mMat * a_transform[1];
		a_transform[2] = mMat * a_transform[2];
	}
}

void Tutorial1::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement( m_cameraMatrix, a_deltaTime, 10 );

	// update rotating matrix
	m_rotatingMatrix = glm::rotate(m_rotatingMatrix, a_deltaTime, glm::vec3(0, 1, 0));

	// update moveable object matrix
	moveObject( m_moveMatrix, a_deltaTime, 10);

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

	// add shapes
	Gizmos::addAABBFilled( glm::vec3(0, 1, -5), glm::vec3(1, 1, 1), glm::vec4(0.5, 0.25, 1, 1) );
	Gizmos::addCylinderFilled( glm::vec3(5, 1, -5), 1, 1, 8, glm::vec4(0, 1, 1, 1) );
	Gizmos::addSphere( glm::vec3(-5, 1, 0), 1, 8, 8, glm::vec4(1, 1, 0, 1) );
	Gizmos::addRing( glm::vec3(5, 0, 0), 1, 1.5, 8, glm::vec4(1, 0, 0, 1) );
	Gizmos::addArc( glm::vec3(0, 0, 0), 0, 5, glm::pi<float>() / 4, 4, glm::vec4(0, 1, 0, 1) );

	// add rotating object
	Gizmos::addArcRing(glm::vec3(2, 0, -2), 0, 1, 1.5, glm::pi<float>() / 4, 4, glm::vec4(1, 0.5, 0, 1), &m_rotatingMatrix);

	// add moving object
	Gizmos::addTransform(m_moveMatrix);

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

void Tutorial1::onDraw() 
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
	Gizmos::draw2D(glm::ortho<float>(0, width, 0, height, -1.0f, 1.0f));
}

void Tutorial1::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new Tutorial1();
	
	if (app->create("AIE - Tutorial1",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}