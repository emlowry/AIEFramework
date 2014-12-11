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

	// load shaders and link shader program
	m_vertShader = Utility::loadShader("shaders/lit.vert", GL_VERTEX_SHADER);
	m_fragShader = Utility::loadShader("shaders/lit.frag", GL_FRAGMENT_SHADER);
	const char* inputs[] = { "Position", "Normal" };
	m_shader = Utility::createProgram(m_vertShader, 0, 0, 0, m_fragShader, 2, inputs);

	m_lightAmbient = glm::vec3(0.0625, 0, 0.125);

	m_lights[0].color = glm::vec3(1, 0.75, 0.875);
	m_lights[0].position = glm::vec3(-2.4, 1.8, 4.0);
	m_lights[0].attenuation = .5;
	m_lights[1].color = glm::vec3(1, 0.875, 0.75);
	m_lights[1].direction = glm::vec3(-0.48, -0.8, -0.36);
	m_lights[1].position = glm::vec3(3.6, 6.0, 2.7);
	m_lights[1].attenuation = 1.5;
	m_lights[2].color = glm::vec3(0.75, 0.875, 1);
	m_lights[2].direction = glm::vec3(0.36, -0.8, -0.48);
	m_lights[2].position = glm::vec3(-2.7, 6.0, 3.6);
	m_lights[2].attenuation = 1;
	m_lights[2].angle = 45;
	m_lights[2].blur = .1;
	m_lightCount = 3;

	m_fbx = new FBXFile();
	m_fbx->load("models/stanford/Bunny.fbx");

	createOpenGLBuffers(m_fbx);

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

	
	/*
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
	/**/
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

	// get view frustrum
	glm::vec4 planes[6];
	getFrustrumPlanes(m_projectionMatrix, planes);

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
	char buffer[50];
	for (int i = 0; i < m_lightCount; ++i)
	{
		std::sprintf(buffer, "lights[%d].color", i);
		location = glGetUniformLocation(m_shader, buffer);
		glUniform3fv(location, 1, &(m_lights[i].color[0]));
		std::sprintf(buffer, "lights[%d].direction", i);
		location = glGetUniformLocation(m_shader, buffer);
		glUniform3fv(location, 1, &(m_lights[i].direction[0]));
		std::sprintf(buffer, "lights[%d].position", i);
		location = glGetUniformLocation(m_shader, buffer);
		glUniform3fv(location, 1, &(m_lights[i].position[0]));
		std::sprintf(buffer, "lights[%d].power", i);
		location = glGetUniformLocation(m_shader, buffer);
		glUniform1f(location, m_lights[i].power);
		std::sprintf(buffer, "lights[%d].attenuation", i);
		location = glGetUniformLocation(m_shader, buffer);
		glUniform1f(location, m_lights[i].attenuation);
		std::sprintf(buffer, "lights[%d].angle", i);
		location = glGetUniformLocation(m_shader, buffer);
		glUniform1f(location, m_lights[i].angle);
		std::sprintf(buffer, "lights[%d].blur", i);
		location = glGetUniformLocation(m_shader, buffer);
		glUniform1f(location, m_lights[i].blur);
	}
	location = glGetUniformLocation(m_shader, "lightCount");
	glUniform1i(location, m_lightCount);

	// send camera position
	location = glGetUniformLocation(m_shader, "cameraPosition");
	glUniform3fv(location, 1, glm::value_ptr(m_cameraMatrix[3]));

	// bind our vertex array object and draw the mesh
	for (unsigned int i = 0; i < m_fbx->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = m_fbx->getMeshByIndex(i);
		OGL_FBXRenderData* glData = (OGL_FBXRenderData*)mesh->m_userData;

		// test for visibility
		glm::vec4 boundingSphere = glData->sphere;
		boundingSphere.xyz = (viewMatrix * glm::vec4(boundingSphere.xyz(), 1)).xyz();
		for (int i = 0; i < 6; ++i)
		{
			if (planeSphereTest(planes[i], boundingSphere) < 0)
			{
				printf("behind\n");
				break;
			}

			if (i == 5)
			{
				printf("visible\n");

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

				glBindVertexArray(glData->VAO);
				glDrawElements(GL_TRIANGLES, (unsigned int)mesh->m_indices.size(), GL_UNSIGNED_INT, 0);
			}
		}
	}
}

void SceneManagement::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	cleanupOpenGLBuffers(m_fbx);
	m_lightCount = 0;
}

void SceneManagement::createOpenGLBuffers(FBXFile* a_fbx)
{
	// create the GL VAO/VBO/IBO data for meshes
	for (unsigned int i = 0; i < a_fbx->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = a_fbx->getMeshByIndex(i);

		// storage for the opengl data in 3 unsigned int
		OGL_FBXRenderData* glData = new OGL_FBXRenderData();

		glGenVertexArrays(1, &glData->VAO);
		glBindVertexArray(glData->VAO);

		glGenBuffers(1, &glData->VBO);
		glGenBuffers(1, &glData->IBO);

		glBindBuffer(GL_ARRAY_BUFFER, glData->VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glData->IBO);

		glBufferData(GL_ARRAY_BUFFER, mesh->m_vertices.size() * sizeof(FBXVertex), mesh->m_vertices.data(), GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->m_indices.size() * sizeof(unsigned int), mesh->m_indices.data(), GL_STATIC_DRAW);

		// calculate bounding box
		glm::vec3 min, max;
		min = max = mesh->m_vertices[0].position.xyz;
		for each (FBXVertex vertex in mesh->m_vertices)
		{
			if (vertex.position.x > max.x) max.x = vertex.position.x;
			else if (vertex.position.x < min.x) min.x = vertex.position.x;
			if (vertex.position.y > max.y) max.y = vertex.position.y;
			else if (vertex.position.y < min.y) min.y = vertex.position.y;
			if (vertex.position.z > max.z) max.z = vertex.position.z;
			else if (vertex.position.z < min.z) min.z = vertex.position.z;
		}

		// calculate bounding sphere
		glData->sphere.xyz = (min + max) / 2.0f;
		glData->sphere.w = (max - min).length() / 2;

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

void SceneManagement::cleanupOpenGLBuffers(FBXFile* a_fbx)
{
	// bind our vertex array object and draw the mesh
	for (unsigned int i = 0; i < a_fbx->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = a_fbx->getMeshByIndex(i);

		OGL_FBXRenderData* glData = (OGL_FBXRenderData*)mesh->m_userData;

		glDeleteVertexArrays(1, &glData->VAO);
		glDeleteBuffers(1, &glData->VBO);
		glDeleteBuffers(1, &glData->IBO);

		delete glData;
	}
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
