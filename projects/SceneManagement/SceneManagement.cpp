#include "SceneManagement.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

SceneManagement::SceneManagement()
{

}

SceneManagement::~SceneManagement()
{

}

bool SceneManagement::onCreate(int a_argc, char* a_argv[]) 
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

	return true;
}

void SceneManagement::onUpdate(float a_deltaTime) 
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

	

	glm::vec4 plane = glm::vec4(1, 0, 0, -4.75);
	glm::vec4 sphere = glm::vec4(-5, 0, 0, 0.5f);
	Gizmos::addSphere(sphere.xyz(), sphere.w, 4, 4, glm::vec4(1, 1, 0, 1));

	glm::vec4 planes[6];
	getFrustrumPlanes(m_projectionMatrix, planes);

	glm::mat4 viewMatrix = glm::inverse(m_cameraMatrix);
	sphere.xyz = (viewMatrix * glm::vec4(sphere.xyz(), 1)).xyz();

	for (int i = 0; i < 6; ++i)
	{
		if (planeSphereTest(planes[i], sphere) < 0)
		{
			printf("behind\n");
			break;
		}

		if (i == 5)
		{
			printf("visible\n");
		}
	}

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

void SceneManagement::onDraw() 
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

void SceneManagement::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new SceneManagement();
	
	if (app->create("AIE - SceneManagement",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}

int	SceneManagement::planeSphereTest(const glm::vec4& a_plane,
									 const glm::vec4& a_sphere)
{
	// distance from center of sphere to nearest point on the plane
	float dist = glm::dot<float>(a_plane.xyz(), a_sphere.xyz()) - a_plane.w;

	// radius of sphere
	float radius = (a_sphere.w < 0) ? -a_sphere.w : a_sphere.w;

	// 1 = plane in front of sphere, -1 = plane behind sphere, 0 = intersection
	return (dist > radius) ? 1 : (dist < -radius) ? -1 : 0;
}

void SceneManagement::getFrustrumPlanes(const glm::mat4& a_transform, glm::vec4* a_planes)
{
	// right plane
	a_planes[0] = glm::vec4(a_transform[0][3] - a_transform[0][0],
							a_transform[1][3] - a_transform[1][0],
							a_transform[2][3] - a_transform[2][0],
							a_transform[3][3] - a_transform[3][0]);

	// left plane
	a_planes[1] = glm::vec4(a_transform[0][3] + a_transform[0][0],
							a_transform[1][3] + a_transform[1][0],
							a_transform[2][3] + a_transform[2][0],
							a_transform[3][3] + a_transform[3][0]);

	// top plane
	a_planes[2] = glm::vec4(a_transform[0][3] - a_transform[0][1],
							a_transform[1][3] - a_transform[1][1],
							a_transform[2][3] - a_transform[2][1],
							a_transform[3][3] - a_transform[3][1]);

	// bottom plane
	a_planes[3] = glm::vec4(a_transform[0][3] + a_transform[0][1],
							a_transform[1][3] + a_transform[1][1],
							a_transform[2][3] + a_transform[2][1],
							a_transform[3][3] + a_transform[3][1]);

	// far plane
	a_planes[4] = glm::vec4(a_transform[0][3] - a_transform[0][2],
							a_transform[1][3] - a_transform[1][2],
							a_transform[2][3] - a_transform[2][2],
							a_transform[3][3] - a_transform[3][2]);

	// near plane
	a_planes[5] = glm::vec4(a_transform[0][3] + a_transform[0][2],
							a_transform[1][3] + a_transform[1][2],
							a_transform[2][3] + a_transform[2][2],
							a_transform[3][3] + a_transform[3][2]);
}
