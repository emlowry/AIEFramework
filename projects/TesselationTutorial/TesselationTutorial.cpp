#include "TesselationTutorial.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <stb_image.h>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

TesselationTutorial::TesselationTutorial()
{

}

TesselationTutorial::~TesselationTutorial()
{

}

bool TesselationTutorial::onCreate(int a_argc, char* a_argv[]) 
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
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	Utility::build3DPlane(10, m_vao, m_vbo, m_ibo);

	int format, formatGL;
	unsigned char* data = nullptr;

	data = stbi_load("../../bin/textures/rock_diffuse.tga", &width, &height, &format, STBI_default);

	// convert the stbi format to the actual GL code
	switch (format)
	{
	case STBI_grey: formatGL = GL_LUMINANCE; break;
	case STBI_grey_alpha: formatGL = GL_LUMINANCE_ALPHA; break;
	case STBI_rgb: formatGL = GL_RGB; break;
	case STBI_rgb_alpha: formatGL = GL_RGBA; break;
	};

	glGenTextures(1, &m_texture);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, formatGL, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	data = stbi_load("textures/rock_displacement.tga", &width, &height, &format, STBI_default);

	// convert the stbi format to the actual GL code
	switch (format)
	{
	case STBI_grey: formatGL = GL_LUMINANCE; break;
	case STBI_grey_alpha: formatGL = GL_LUMINANCE_ALPHA; break;
	case STBI_rgb: formatGL = GL_RGB; break;
	case STBI_rgb_alpha: formatGL = GL_RGBA; break;
	};

	glGenTextures(1, &m_displacement);
	glBindTexture(GL_TEXTURE_2D, m_displacement);
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, formatGL, GL_UNSIGNED_BYTE, data);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, 0);

	unsigned int vs = Utility::loadShader("shaders/displace.vert", GL_VERTEX_SHADER);
	unsigned int cs = Utility::loadShader("shaders/displace.cont", GL_TESS_CONTROL_SHADER);
	unsigned int es = Utility::loadShader("shaders/displace.eval", GL_TESS_EVALUATION_SHADER);
	unsigned int fs = Utility::loadShader("shaders/displace.frag", GL_FRAGMENT_SHADER);
	m_shader = Utility::createProgram(vs, cs, es, 0, fs);
	glDeleteShader(vs);
	glDeleteShader(cs);
	glDeleteShader(es);
	glDeleteShader(fs);

	m_displacementScale = 1.0f;
	m_tessLevelInner = 64;
	m_tessLevelOuter = 64;

	return true;
}

void TesselationTutorial::onUpdate(float a_deltaTime) 
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

void TesselationTutorial::onDraw() 
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

	// calculate projection view matrix and bind to uniform
	glm::mat4 projectionViewMatrix = m_projectionMatrix * viewMatrix;
	unsigned int location = glGetUniformLocation(m_shader, "projectionView");
	glUniformMatrix4fv(location, 1, false, glm::value_ptr(projectionViewMatrix));

	// activate texture slot 0 and bind our texture to it
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_texture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_displacement);

	// fetch the location of the texture sampler and bind it to 0
	location = glGetUniformLocation(m_shader, "textureMap");
	glUniform1i(location, 0);
	location = glGetUniformLocation(m_shader, "displacementMap");
	glUniform1i(location, 1);

	location = glGetUniformLocation(m_shader, "displacementScale");
	glUniform1f(location, m_displacementScale);
	location = glGetUniformLocation(m_shader, "tesselationInnerLevel");
	glUniform1i(location, m_tessLevelInner);
	location = glGetUniformLocation(m_shader, "tesselationOuterLevel");
	glUniform1i(location, m_tessLevelOuter);

	glBindVertexArray(m_vao);
	glPatchParameteri(GL_PATCH_VERTICES, 3);
	glDrawElements(GL_PATCHES, 6, GL_UNSIGNED_INT, nullptr);
}

void TesselationTutorial::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new TesselationTutorial();
	
	if (app->create("AIE - TesselationTutorial",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}