#include "GPUParticleTutorial.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

GPUParticleTutorial::GPUParticleTutorial(const char* a_titleFormat)
{
	sprintf_s(m_titleFormat, "%s - [FPS: %%3.2f, Particles: %%g]", a_titleFormat);
	m_titleFormat[255] = '\0';
}

GPUParticleTutorial::~GPUParticleTutorial()
{

}

bool GPUParticleTutorial::onCreate(int a_argc, char* a_argv[]) 
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

	// enable particle transparency
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glAlphaFunc(GL_GREATER, 0);
	glEnable(GL_ALPHA_TEST);

	m_emitter = new GPUParticleEmitter();
	m_emitter->initalise(10000,
		0.05f, 10.0f, 1.25f, 5.0f, 0.2f, 1.0f,
		glm::vec4(1, 0.5, 0.875, 1), glm::vec4(0.5, 0.875, 1, 1),
		"images/bubble.png");

	m_timeSinceLastFPSUpdate = 0.0f;
	m_frameCount = 0;

	return true;
}

void GPUParticleTutorial::onUpdate(float a_deltaTime) 
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

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();

	// update window title with FPS
	m_timeSinceLastFPSUpdate += a_deltaTime;
	if (m_timeSinceLastFPSUpdate >= m_FPSUpdateInterval)
	{
		char title[256];
		sprintf_s(title, m_titleFormat,
				  (float)m_frameCount / m_timeSinceLastFPSUpdate,
				  (float)(m_emitter->getMaxParticles()));
		title[255] = '\0';
		glfwSetWindowTitle(m_window, title);
		m_frameCount = 0;
		m_timeSinceLastFPSUpdate = 0;
	}
}

void GPUParticleTutorial::onDraw() 
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

	m_emitter->draw(m_cameraMatrix, m_projectionMatrix);

	++m_frameCount;
}

void GPUParticleTutorial::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	delete m_emitter;
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new GPUParticleTutorial("AIE - GPUParticleTutorial");
	
	if (app->create("AIE - GPUParticleTutorial",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}