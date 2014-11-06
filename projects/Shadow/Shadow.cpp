#include "Shadow.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

Shadow::Shadow()
{

}

Shadow::~Shadow()
{

}

bool Shadow::onCreate(int a_argc, char* a_argv[]) 
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

	// load shaders and link shader programs
	unsigned int vs = Utility::loadShader("shaders/shadow.vert", GL_VERTEX_SHADER);
	unsigned int fs = Utility::loadShader("shaders/shadow.frag", GL_FRAGMENT_SHADER);
	const char* inputs1[] = { "Position" };
	const char* outputs1[] = { "depth" };
	m_shadowShader = Utility::createProgram(vs, 0, 0, 0, fs, 1, inputs1, 1, outputs1);
	glDeleteShader(vs);
	glDeleteShader(fs);

	vs = Utility::loadShader("shaders/displayMap.vert", GL_VERTEX_SHADER);
	fs = Utility::loadShader("shaders/displayMap.frag", GL_FRAGMENT_SHADER);
	const char* inputs2[] = { "Position", "TexCoord" };
	const char* outputs2[] = { "FragColor" };
	m_2dprogram = Utility::createProgram(vs, 0, 0, 0, fs, 2, inputs2, 1, outputs2);
	glDeleteShader(vs);
	glDeleteShader(fs);

	vs = Utility::loadShader("shaders/displayMap.vert", GL_VERTEX_SHADER);
	fs = Utility::loadShader("shaders/displayMap.frag", GL_FRAGMENT_SHADER);
	const char* inputs3[] = { "Position", "TexCoord", "Normals" };
	const char* outputs3[] = { "FragColor" };
	m_program = Utility::createProgram(vs, 0, 0, 0, fs, 3, inputs3, 1, outputs3);
	glDeleteShader(vs);
	glDeleteShader(fs);

	createShadowBuffer();
	setUpLightAndShadowMatrix(1.0);
	create2DQuad();

	m_fbx = new FBXFile();
	m_fbx->load("./Models/ruinedtank/tank.fbx", FBXFile::UNITS_METER);
	m_fbx->initialiseOpenGLTextures();
	InitFBXSceneResource(m_fbx);

	return true;
}

void Shadow::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement( m_cameraMatrix, a_deltaTime, 10 );

	UpdateFBXSceneResource(m_fbx);

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

void Shadow::onDraw() 
{
	renderShadowMap();
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

	drawScene();
	//displayShadowMap();
}

void Shadow::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	glDeleteShader(m_2dprogram);
	glDeleteShader(m_shadowShader);
	glDeleteShader(m_program);

	DestroyFBXSceneResource(m_fbx);
	m_fbx->unload();
	delete m_fbx;
	m_fbx = NULL;
}

void Shadow::createShadowBuffer()
{
	//resolution of the texture to put our shadow map in
	m_shadowWidth = 1024;
	m_shadowHeight = 1024;

	// The framebuffer, which regroups 0, 1, or more textures, and 0 or 1 depth buffer.
	glGenFramebuffers(1, &m_shadowFramebuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, m_shadowFramebuffer);

	// Depth texture. Slower than a depth buffer, but you can sample it later in your shader
	glGenTextures(1, &m_shadowTexture);
	glBindTexture(GL_TEXTURE_2D, m_shadowTexture);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT16, m_shadowWidth, m_shadowHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, 0);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

	glFramebufferTexture(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, m_shadowTexture, 0);

	glDrawBuffer(GL_NONE); // No color buffer is drawn to.

	// Always check that our framebuffer is ok
	if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
	{
		printf("depth buffer not created");
	}
	else
	{
		printf("Success! created depth buffer\n");
	}

	// return to back-buffer rendering
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void Shadow::setUpLightAndShadowMatrix(float count)
{
	// setup light direction and shadow matrix
	glm::vec3 lightPosition = glm::vec3(1.0f, 1.0f, 0);
	m_lightDirection = glm::normalize(glm::vec4(-lightPosition, 0));

	glm::mat4 depthViewMatrix = glm::lookAt(lightPosition, glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
	glm::mat4 depthProjectionMatrix = glm::ortho<float>(-20, 20, -20, 20, 0, 20);
	m_shadowProjectionViewMatrix = depthProjectionMatrix * depthViewMatrix;
}

void Shadow::create2DQuad()
{
	glGenVertexArrays(1, &m_2dQuad.vao);
	glBindVertexArray(m_2dQuad.vao);

	// create a 200x200 2D GUI quad (resize it to screen-space!)
	glm::vec2 size(200, 200);
	size.x /= DEFAULT_SCREENWIDTH;
	size.y /= DEFAULT_SCREENHEIGHT;
	size *= 2;

	// setup the quad in the top corner
	float quadVertices[] = {
		-1.0f, 1.0f - size.y, 0.0f, 1.0f, 0, 0,
		-1.0f + size.x, 1.0f - size.y, 0.0f, 1.0f, 1, 0,
		-1.0f, 1.0f, 0.0f, 1.0f, 0, 1,

		-1.0f, 1.0f, 0.0f, 1.0f, 0, 1,
		-1.0f + size.x, 1.0f - size.y, 0.0f, 1.0f, 1, 0,
		-1.0f + size.x, 1.0f, 0.0f, 1.0f, 1, 1,
	};

	glGenBuffers(1, &m_2dQuad.vbo);
	glBindBuffer(GL_ARRAY_BUFFER, m_2dQuad.vbo);
	glBufferData(GL_ARRAY_BUFFER, sizeof(float)* 6 * 6, quadVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(float)* 6, 0);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(float)* 6, ((char*)0) + 16);
	glBindVertexArray(0);
}

void Shadow::renderShadowMap()
{
	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_shadowFramebuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	glUseProgram(m_shadowShader);
	glCullFace(GL_FRONT);

	unsigned int location = glGetUniformLocation(m_2dprogram, "lightProjectionViewWorld");
	glUniformMatrix4fv(location, 1, false, glm::value_ptr(m_shadowProjectionViewMatrix));

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, m_shadowTexture);

	// render FBX scene
	for (unsigned int i = 0; i < m_fbx->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = m_fbx->getMeshByIndex(i);
		glBindVertexArray(((GLData*)mesh->m_userData)->vao);
		glDrawElements(GL_TRIANGLES, (unsigned int)mesh->m_indices.size(), GL_UNSIGNED_INT, 0);
	}

	glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
	glUseProgram(0);
}

void Shadow::displayShadowMap()
{
	glClear(GL_DEPTH_BUFFER_BIT);
	glUseProgram(m_2dprogram);

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_shadowTexture);
	unsigned int texLoc = glGetUniformLocation(m_2dprogram, "shadowMap");
	glUniform1i(texLoc, 1);
	glActiveTexture(GL_TEXTURE0);

	glBindVertexArray(m_2dQuad.vao);
	glDrawArrays(GL_TRIANGLES, 0, 6);

	glUseProgram(0);
}

void Shadow::drawScene()
{
	//use the selected shader program
	glUseProgram(m_program);

	// set to render to the back-buffer
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	glCullFace(GL_BACK);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse(m_cameraMatrix);

	// bind the shadow map to texture slot 1
	unsigned int texLoc = glGetUniformLocation(m_program, "shadowMap");
	glUniform1i(texLoc, 1);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, m_shadowTexture);

	// reset back to the default active texture
	glActiveTexture(GL_TEXTURE0);

	// bind projection * view matrix
	unsigned int projectionView = glGetUniformLocation(m_program, "projectionView");
	glm::mat4 projectionViewMatrix = m_projectionMatrix * viewMatrix;
	glUniformMatrix4fv(projectionView, 1, false, glm::value_ptr(projectionViewMatrix));

	// bind light and shadow information
	unsigned int ambient = glGetUniformLocation(m_program, "ambientLight");
	unsigned int lightDir = glGetUniformLocation(m_program, "lightDir");
	glUniform3f(ambient, 0.5f, 0.5f, 0.5f);
	glUniform4fv(lightDir, 1, glm::value_ptr(m_lightDirection));

	// set diffuse texture location for binding
	unsigned int diffuseMap = glGetUniformLocation(m_program, "diffuseMap");

	// get the world transform location and shadow matrix location
	unsigned int world = glGetUniformLocation(m_program, "world");
	unsigned int shadowMatrix = glGetUniformLocation(m_program, "lightProjectionViewWorld");

	// render FBX scene
	for (unsigned int i = 0; i < m_fbx->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = m_fbx->getMeshByIndex(i);
		glm::mat4 shadowMVP = m_shadowProjectionViewMatrix * mesh->m_globalTransform;
		glUniformMatrix4fv(shadowMatrix, 1, false, glm::value_ptr(shadowMVP));
		glUniformMatrix4fv(world, 1, false, glm::value_ptr(mesh->m_globalTransform));

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mesh->m_material->textures[FBXMaterial::DiffuseTexture]->handle);
		glUniform1i(diffuseMap, 2);

		// reset back to the default active texture
		glActiveTexture(GL_TEXTURE0);

		glBindVertexArray(((GLData*)mesh->m_userData)->vao);
		glDrawElements(GL_TRIANGLES, (unsigned int)mesh->m_indices.size(), GL_UNSIGNED_INT, 0);
	}
	glUseProgram(0);
}

void Shadow::InitFBXSceneResource(FBXFile *a_pScene)
{
	// how manu meshes and materials are stored within the fbx file
	unsigned int meshCount = a_pScene->getMeshCount();
	unsigned int matCount = a_pScene->getMaterialCount();

	// loop through each mesh
	for (unsigned int i = 0; i<meshCount; ++i)
	{
		// get the current mesh
		FBXMeshNode *pMesh = a_pScene->getMeshByIndex(i);

		// genorate our OGL_FBXRenderData for storing the meshes VBO, IBO and VAO
		// and assign it to the meshes m_userData pointer so that we can retrive 
		// it again within the render function
		OGL_FBXRenderData *ro = new OGL_FBXRenderData();
		pMesh->m_userData = ro;

		// OPENGL: genorate the VBO, IBO and VAO
		glGenBuffers(1, &ro->VBO);
		glGenBuffers(1, &ro->IBO);
		glGenVertexArrays(1, &ro->VAO);

		// OPENGL: Bind  VAO, and then bind the VBO and IBO to the VAO
		glBindVertexArray(ro->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, ro->VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ro->IBO);

		// Send the vertex data to the VBO
		glBufferData(GL_ARRAY_BUFFER, pMesh->m_vertices.size() * sizeof(FBXVertex), pMesh->m_vertices.data(), GL_STATIC_DRAW);

		// send the index data to the IBO
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMesh->m_indices.size() * sizeof(unsigned int), pMesh->m_indices.data(), GL_STATIC_DRAW);

		// enable the attribute locations that will be used on our shaders
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// tell our shaders where the information within our buffers lie
		// eg: attrubute 0 is expected to be the verticy's position. it should be 4 floats, representing xyzw
		// eg: attrubute 1 is expected to be the verticy's color. it should be 4 floats, representing rgba
		// eg: attrubute 2 is expected to be the verticy's texture coordinate. it should be 2 floats, representing U and V
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::PositionOffset);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::TexCoord1Offset);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::NormalOffset);

		// finally, where done describing our mesh to the shader
		// we can describe the next mesh
		glBindVertexArray(0);
	}
}

void Shadow::UpdateFBXSceneResource(FBXFile *a_pScene)
{
	a_pScene->getRoot()->updateGlobalTransform();
}

void Shadow::DestroyFBXSceneResource(FBXFile *a_pScene)
{
	// how manu meshes and materials are stored within the fbx file
	unsigned int meshCount = a_pScene->getMeshCount();
	unsigned int matCount = a_pScene->getMaterialCount();

	// remove meshes
	for (unsigned int i = 0; i<meshCount; i++)
	{
		// Get the current mesh and retrive the render data we assigned to m_userData
		FBXMeshNode* pMesh = a_pScene->getMeshByIndex(i);
		OGL_FBXRenderData *ro = (OGL_FBXRenderData *)pMesh->m_userData;

		// delete the buffers and free memory from the graphics card
		glDeleteBuffers(1, &ro->VBO);
		glDeleteBuffers(1, &ro->IBO);
		glDeleteVertexArrays(1, &ro->VAO);

		// this is memory we created earlier in the InitFBXSceneResources function
		// make sure to destroy it
		delete ro;

	}

	// loop through each of the materials
	for (unsigned int i = 0; i<matCount; i++)
	{
		// get the current material
		FBXMaterial *pMaterial = a_pScene->getMaterialByIndex(i);
		if (nullptr != pMaterial)
		{
			for (int j = 0; j<FBXMaterial::TextureTypes_Count; j++)
			{
				// delete the texture if it was loaded
				if (nullptr != pMaterial->textures[j] && pMaterial->textures[j]->handle != 0)
					glDeleteTextures(1, &(pMaterial->textures[j]->handle));
			}
		}
	}
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new Shadow();
	
	if (app->create("AIE - Shadow",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}