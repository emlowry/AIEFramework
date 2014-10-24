#include "LightingTutorial.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

LightingTutorial::LightingTutorial()
{

}

LightingTutorial::~LightingTutorial()
{

}

bool LightingTutorial::onCreate(int a_argc, char* a_argv[]) 
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
	m_vertShader = Utility::loadShader("shaders/lit.vert", GL_VERTEX_SHADER);
	m_fragShader = Utility::loadShader("shaders/lit.frag", GL_FRAGMENT_SHADER);
	const char* inputs[] = { "Position", "Normal" };
	m_shader = Utility::createProgram(m_vertShader, 0, 0, 0, m_fragShader, 2, inputs);

	m_lightAmbient = glm::vec3(0.0625, 0, 0.125);
	m_lightDirection = glm::vec3(-0.48, -0.8, -0.36);
	m_lightColour = glm::vec3(1, 0.75, 0.875);

	m_fbx = new FBXFile();
	m_fbx->load("models/stanford/Bunny.fbx");

	createOpenGLBuffers(m_fbx);

	return true;
}

void LightingTutorial::onUpdate(float a_deltaTime) 
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

void LightingTutorial::onDraw() 
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

	// light attributes
	location = glGetUniformLocation(m_shader, "lightAmbient");
	glUniform3fv(location, 1, &m_lightAmbient[0]);
	location = glGetUniformLocation(m_shader, "lightDirection");
	glUniform3fv(location, 1, &m_lightDirection[0]);
	location = glGetUniformLocation(m_shader, "lightColour");
	glUniform3fv(location, 1, &m_lightColour[0]);
	
	// send camera position
	location = glGetUniformLocation(m_shader, "cameraPosition");
	glUniform3fv(location, 1, glm::value_ptr(m_cameraMatrix[3]));

	// bind our vertex array object and draw the mesh
	for (unsigned int i = 0; i < m_fbx->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = m_fbx->getMeshByIndex(i);
		FBXMaterial* material = mesh->m_material;
		location = glGetUniformLocation(m_shader, "hasMaterial");
		glUniform1i(location, nullptr != material ? GL_TRUE : GL_FALSE);
		if (nullptr != material)
		{
			location = glGetUniformLocation(m_shader, "materialAmbient");
			glUniform4fv(location, 1, &(material->ambient[0]));
			location = glGetUniformLocation(m_shader, "materialDiffuse");
			glUniform4fv(location, 1, &(material->diffuse[0]));
			location = glGetUniformLocation(m_shader, "materialSpecular");
			glUniform4fv(location, 1, &(material->specular[0]));
		}

		unsigned int* glData = (unsigned int*)mesh->m_userData;

		glBindVertexArray(glData[0]);
		glDrawElements(GL_TRIANGLES, (unsigned int)mesh->m_indices.size(), GL_UNSIGNED_INT, 0);
	}
}

void LightingTutorial::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	cleanupOpenGLBuffers(m_fbx);
}

void LightingTutorial::createOpenGLBuffers(FBXFile* a_fbx)
{
	// create the GL VAO/VBO/IBO data for meshes
	for (unsigned int i = 0; i < a_fbx->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = a_fbx->getMeshByIndex(i);

		// storage for the opengl data in 3 unsigned int
		unsigned int* glData = new unsigned int[3];

		glGenVertexArrays(1, &glData[0]);
		glBindVertexArray(glData[0]);

		glGenBuffers(1, &glData[1]);
		glGenBuffers(1, &glData[2]);

		glBindBuffer(GL_ARRAY_BUFFER, glData[1]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glData[2]);

		glBufferData(GL_ARRAY_BUFFER, mesh->m_vertices.size() * sizeof(FBXVertex), mesh->m_vertices.data(), GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->m_indices.size() * sizeof(unsigned int), mesh->m_indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0); // position
		glEnableVertexAttribArray(1); // normal
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), 0);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_TRUE, sizeof(FBXVertex), ((char*)0) + FBXVertex::NormalOffset);

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		mesh->m_userData = glData;
	}
}

void LightingTutorial::cleanupOpenGLBuffers(FBXFile* a_fbx)
{
	// bind our vertex array object and draw the mesh
	for (unsigned int i = 0; i < a_fbx->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = a_fbx->getMeshByIndex(i);

		unsigned int* glData = (unsigned int*)mesh->m_userData;

		glDeleteVertexArrays(1, &glData[0]);
		glDeleteBuffers(1, &glData[1]);
		glDeleteBuffers(1, &glData[2]);

		delete[] glData;
	}
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new LightingTutorial();
	
	if (app->create("AIE - LightingTutorial",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}