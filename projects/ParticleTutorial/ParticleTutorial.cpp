#include "ParticleTutorial.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

ParticleTutorial::ParticleTutorial()
{

}

ParticleTutorial::~ParticleTutorial()
{

}

bool ParticleTutorial::onCreate(int a_argc, char* a_argv[]) 
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

	// load shaders and link shader program
	m_vertShader = Utility::loadShader("shaders/ParticleTutorial.vert", GL_VERTEX_SHADER);
	m_fragShader = Utility::loadShader("shaders/ParticleTutorial.frag", GL_FRAGMENT_SHADER);

	// our vertex buffer has 3 properties per-vertex
	const char* inputs[] = { "Position", "Colour" };
	m_shader = Utility::createProgram(m_vertShader, 0, 0, 0, m_fragShader, 2, inputs);

	m_emitter = new ParticleEmitter();
	m_emitter->initalise(1000, 500, 0.1f, 0.9f, 1.0f, 5.0f, 0.1f, 1.0f,
						 glm::vec4(1, 1, 0, 1), glm::vec4(1, 0, 0, 1));

	return true;
}

void ParticleTutorial::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement(m_cameraMatrix, a_deltaTime, 10);
	m_emitter->update(a_deltaTime, m_cameraMatrix);

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
}

void ParticleTutorial::onDraw() 
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

	// bind shader to the GPU
	glUseProgram(m_shader);

	// fetch locations of the view and projection matrices and bind them
	unsigned int location = glGetUniformLocation(m_shader, "view");
	glUniformMatrix4fv(location, 1, false, glm::value_ptr(viewMatrix));

	location = glGetUniformLocation(m_shader, "projection");
	glUniformMatrix4fv(location, 1, false, glm::value_ptr(m_projectionMatrix));

	m_emitter->draw();
}

void ParticleTutorial::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	// delete the shader
	glDeleteProgram(m_shader);
	glDeleteShader(m_vertShader);
	glDeleteShader(m_fragShader);
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new ParticleTutorial();
	
	if (app->create("AIE - ParticleTutorial",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}