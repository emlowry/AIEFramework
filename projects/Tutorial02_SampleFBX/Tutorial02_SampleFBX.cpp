#include "Tutorial02_SampleFBX.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

Tutorial02_SampleFBX::Tutorial02_SampleFBX()
{

}

Tutorial02_SampleFBX::~Tutorial02_SampleFBX()
{

}

bool Tutorial02_SampleFBX::onCreate(int a_argc, char* a_argv[]) 
{
	// initialise the Gizmos helper class
	Gizmos::create();

	// create a world-space matrix for a camera
	m_cameraMatrix = glm::inverse( glm::lookAt(glm::vec3(10,10,10),glm::vec3(0,0,0), glm::vec3(0,1,0)) );
	
	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, DEFAULT_SCREENWIDTH/(float)DEFAULT_SCREENHEIGHT, 0.1f, 1000.0f);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// load the shader
	const char* aszInputs[] = { "Position", "Color", "TexCoord1" };
	const char* aszOutputs[] = { "outColour" };

	// load shader internally calls glCreateShader...
	GLuint vshader = Utility::loadShader("./shaders/SimpleFBX_Tutorial.vert", GL_VERTEX_SHADER);
	GLuint pshader = Utility::loadShader("./shaders/SimpleFBX_Tutorial.frag", GL_FRAGMENT_SHADER);

	m_shader = Utility::createProgram(vshader, 0, 0, 0, pshader, 3, aszInputs, 1, aszOutputs);

	// free our shader once we built our program
	glDeleteShader(vshader);
	glDeleteShader(pshader);

	return true;
}

void Tutorial02_SampleFBX::onUpdate(float a_deltaTime) 
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
}

void Tutorial02_SampleFBX::onDraw() 
{
	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse( m_cameraMatrix );
	
	// draw the gizmos from this frame
	Gizmos::draw(m_projectionMatrix, viewMatrix);
}

void Tutorial02_SampleFBX::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	glDeleteShader(m_shader);
}

void Tutorial02_SampleFBX::InitFBXSceneResource(FBXFile *a_pScene)
{

}

void Tutorial02_SampleFBX::UpdateFBXSceneResource(FBXFile *a_pScene)
{

}

void Tutorial02_SampleFBX::RenderFBXSceneResource(FBXFile *a_pScene, glm::mat4 a_view, glm::mat4 a_projection)
{

}

void Tutorial02_SampleFBX::DestroyFBXSceneResource(FBXFile *a_pScene)
{

}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new Tutorial02_SampleFBX();
	
	if (app->create("AIE - Tutorial02_SampleFBX",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}