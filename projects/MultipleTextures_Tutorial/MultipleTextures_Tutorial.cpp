#include "MultipleTextures_Tutorial.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <stb_image.h>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

MultipleTextures_Tutorial::MultipleTextures_Tutorial()
{

}

MultipleTextures_Tutorial::~MultipleTextures_Tutorial()
{

}

bool MultipleTextures_Tutorial::onCreate(int a_argc, char* a_argv[]) 
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
	
	// load the shader
	const char* aszInputs[] = { "Position", "Normal", "Tangent", "BiNormal", "TexCoord1" };
	const char* aszOutputs[] = { "outColour" };

	// load shader internally calls glCreateShader...
	GLuint vshader = Utility::loadShader("shaders/MultipleTextures_Tutorial.vert", GL_VERTEX_SHADER);
	GLuint pshader = Utility::loadShader("shaders/MultipleTextures_Tutorial.frag", GL_FRAGMENT_SHADER);

	m_shader = Utility::createProgram(vshader, 0, 0, 0, pshader, 5, aszInputs, 1, aszOutputs);

	// free our shader once we built our program
	glDeleteShader(vshader);
	glDeleteShader(pshader);

	m_fbx = new FBXFile();
	m_fbx->load("Models/soulspear/soulspear.fbx", FBXFile::UNITS_CENTIMETER);
	m_fbx->initialiseOpenGLTextures();
	InitFBXSceneResource(m_fbx);

	// set up decay
	m_decayValue = 0;
	m_decayTexture = loadTexture("images/decay.png");
	m_metallicTexture = loadTexture("images/soulspear_metalic_diffuse.png", GL_RGB);
	
	// load cube map
	m_cubemap_texture = loadCube("images/top.png", "images/bottom.png",
								 "images/north.png", "images/south.png",
								 "images/east.png", "images/west.png");

	// Genorate our cube
	// cube vertices for vertex buffer object
	float cube_vertices[] =
	{
		//	x,	  y,	z
		-1.0,	1.0,	1.0,
		-1.0,	-1.0,	1.0,
		1.0,	-1.0,	1.0,
		1.0,	1.0,	1.0,
		-1.0,	1.0,	-1.0,
		-1.0,	-1.0,	-1.0,
		1.0,	-1.0,	-1.0,
		1.0,	1.0,	-1.0,
	};

	unsigned short cube_indices[] =
	{
		0, 2, 1, 0, 3, 2,	// +Z face
		3, 6, 2, 3, 7, 6,	// +X face
		7, 5, 6, 7, 4, 5,	// -Z face
		4, 1, 5, 4, 0, 1,	// -X face
		0, 7, 3, 0, 4, 7,	// +Y face
		1, 2, 6, 1, 6, 5,	// -Y face
	};

	// OPENGL: genorate the VBO, IBO and VAO
	glGenBuffers(1, &m_skybox_vbo);
	glGenBuffers(1, &m_skybox_ibo);
	glGenVertexArrays(1, &m_skybox_vao);

	// send the data
	glBindVertexArray(m_skybox_vao);
	glBindBuffer(GL_ARRAY_BUFFER, m_skybox_vbo);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_skybox_ibo);

	glBufferData(GL_ARRAY_BUFFER, sizeof(cube_vertices), cube_vertices, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(cube_indices), cube_indices, GL_STATIC_DRAW);

	// setup vertex attributes
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, 0);

	glBindVertexArray(0);

	// load the skybox shader
	const char* aszInputs2[] = { "Position" };
	const char* aszOutputs2[] = { "outColour" };

	// load shader internally calls glCreateShader...
	GLuint vshader2 = Utility::loadShader("shaders/skybox.vert", GL_VERTEX_SHADER);
	GLuint pshader2 = Utility::loadShader("shaders/skybox.frag", GL_FRAGMENT_SHADER);

	m_skybox_shader = Utility::createProgram(vshader2, 0, 0, 0, pshader2, 1, aszInputs2, 1, aszOutputs2);

	return true;
}

void MultipleTextures_Tutorial::onUpdate(float a_deltaTime) 
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
	
	if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS) m_decayValue -= a_deltaTime;
	if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS) m_decayValue += a_deltaTime;

	if (m_decayValue < 0.0f) m_decayValue = 0.0f;
	if (m_decayValue > 1.0f) m_decayValue = 1.0f;
}

void MultipleTextures_Tutorial::onDraw() 
{
	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse( m_cameraMatrix );

	// draw the skybox
	glUseProgram(m_skybox_shader);

	glm::mat4 skyView = viewMatrix;
	skyView[3] = glm::vec4(0, 0, 0, 1);
	glm::mat4 MVP = m_projectionMatrix * skyView * glm::scale(glm::vec3(20, 20, 20));

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap_texture);

	glUniformMatrix4fv(glGetUniformLocation(m_skybox_shader, "ModelViewProjection"), 1, false, glm::value_ptr(MVP));
	glUniform1i(glGetUniformLocation(m_skybox_shader, "CubeMap"), 1);

	glBindVertexArray(m_skybox_vao);
	glDrawElements(GL_TRIANGLES, 72, GL_UNSIGNED_SHORT, 0);

	// clear the depthbuffer
	glClear(GL_DEPTH_BUFFER_BIT);
	
	// draw the gizmos from this frame
	Gizmos::draw(m_projectionMatrix, viewMatrix);

	// get window dimensions for 2D orthographic projection
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);
	Gizmos::draw2D(glm::ortho<float>(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f));

	RenderFBXSceneResource(m_fbx, viewMatrix, m_projectionMatrix);
}

void MultipleTextures_Tutorial::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();

	//glDeleteShader(m_shader);
	glDeleteShader(m_skybox_shader);
	
	DestroyFBXSceneResource(m_fbx);
	m_fbx->unload();
	delete m_fbx;
	m_fbx = NULL;

	// delete the textures
	if (0 != m_decayTexture)
	{
		glDeleteTextures(1, &m_decayTexture);
	}
	if (0 != m_metallicTexture)
	{
		glDeleteTextures(1, &m_metallicTexture);
	}
	if (0 != m_cubemap_texture)
	{
		glDeleteTextures(1, &m_cubemap_texture);
	}
}

void MultipleTextures_Tutorial::InitFBXSceneResource(FBXFile *a_pScene)
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
		glEnableVertexAttribArray(3);
		glEnableVertexAttribArray(4);

		// tell our shaders where the information within our buffers lie
		// eg: attrubute 0 is expected to be the verticy's position. it should be 4 floats, representing xyzw
		// eg: attrubute 1 is expected to be the verticy's color. it should be 4 floats, representing rgba
		// eg: attrubute 2 is expected to be the verticy's texture coordinate. it should be 2 floats, representing U and V
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::PositionOffset);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::NormalOffset);
		glVertexAttribPointer(2, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::TangentOffset);
		glVertexAttribPointer(3, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::BiNormalOffset);
		glVertexAttribPointer(4, 2, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::TexCoord1Offset);

		// finally, where done describing our mesh to the shader
		// we can describe the next mesh
		glBindVertexArray(0);
	}
}

void MultipleTextures_Tutorial::UpdateFBXSceneResource(FBXFile *a_pScene)
{
	a_pScene->getRoot()->updateGlobalTransform();
}

void MultipleTextures_Tutorial::RenderFBXSceneResource(FBXFile *a_pScene, glm::mat4 a_view, glm::mat4 a_projection)
{
	// enable transparent blending
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	// activate a shader
	glUseProgram(m_shader);

	// get the location of uniforms on the shader
	GLint uModelViewProjection = glGetUniformLocation(m_shader, "MVP");
	GLint uModelView = glGetUniformLocation(m_shader, "MV");
	GLint uNormalMatrix = glGetUniformLocation(m_shader, "NormalMatrix");

	GLint uLightPosition = glGetUniformLocation(m_shader, "LightPosition");
	GLint uLightColor = glGetUniformLocation(m_shader, "LightColor");
	GLint uAmbientLightColor = glGetUniformLocation(m_shader, "AmbientLightColor");

	GLint uDiffuseTexture = glGetUniformLocation(m_shader, "DiffuseTexture");
	GLint uSpecularTexture = glGetUniformLocation(m_shader, "SpecularTexture");
	GLint uNormalTexture = glGetUniformLocation(m_shader, "NormalTexture");

	GLint uDecayTexture = glGetUniformLocation(m_shader, "DecayTexture");
	GLint uDecayValue = glGetUniformLocation(m_shader, "DecayValue");
	GLint uMetallicTexture = glGetUniformLocation(m_shader, "MetallicTexture");

	GLint uSkyBox = glGetUniformLocation(m_shader, "SkyBox");
	GLint uCameraForward = glGetUniformLocation(m_shader, "CameraForward");

	glUniform3f(uLightPosition, 3.6f, 8.0f, 4.8f);
	glUniform3f(uLightColor, 1.0f, 1.0f, 1.0f);
	glUniform3f(uAmbientLightColor, 0.1f, 0.1f, 0.1f);

	glActiveTexture(GL_TEXTURE6);
	glBindTexture(GL_TEXTURE_CUBE_MAP, m_cubemap_texture);
	glUniform1i(uSkyBox, 6);

	// for each mesh in the model...
	for (unsigned int i = 0; i<a_pScene->getMeshCount(); ++i)
	{
		// get the current mesh
		FBXMeshNode *mesh = a_pScene->getMeshByIndex(i);

		// get the render data attached to the m_userData pointer for this mesh
		OGL_FBXRenderData *ro = (OGL_FBXRenderData *)mesh->m_userData;

		// Bind the texture to one of the ActiveTextures
		// if your shader supported multiple textures, you would bind each texture to a new Active Texture ID here
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mesh->m_material->textures[FBXMaterial::DiffuseTexture]->handle);

		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, mesh->m_material->textures[FBXMaterial::SpecularTexture]->handle);

		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, mesh->m_material->textures[FBXMaterial::NormalTexture]->handle);

		glActiveTexture(GL_TEXTURE4);
		glBindTexture(GL_TEXTURE_2D, m_decayTexture);

		glActiveTexture(GL_TEXTURE5);
		glBindTexture(GL_TEXTURE_2D, m_metallicTexture);

		// tell the shader which texture to use
		glUniform1i(uDiffuseTexture, 1);
		glUniform1i(uSpecularTexture, 2);
		glUniform1i(uNormalTexture, 3);
		glUniform1i(uDecayTexture, 4);
		glUniform1i(uMetallicTexture, 5);

		// tell the shader the decay value
		glUniform1f(uDecayValue, m_decayValue);

		// send the ModelViewProjection, ModelView, and Normal Matrices to the shader
		glm::mat4 modelView = a_view * mesh->m_globalTransform;
		glm::mat4 modelViewProjection = a_projection * modelView;
		glm::mat3 normalMatrix = glm::transpose(glm::inverse(glm::mat3(modelView)));

		glUniformMatrix4fv(uModelViewProjection, 1, false, glm::value_ptr(modelViewProjection));
		glUniformMatrix4fv(uModelView, 1, false, glm::value_ptr(modelView));
		glUniformMatrix3fv(uNormalMatrix, 1, false, glm::value_ptr(normalMatrix));

		glm::vec3 cameraForward = glm::vec3(0, 0, 1);
		cameraForward = glm::inverse(normalMatrix) * cameraForward;
		glUniform3fv(uCameraForward, 1, glm::value_ptr(cameraForward));

		// bind our vertex array object
		// remember in the initialise function, we bound the VAO and IBO to the VAO
		// so when we bind the VAO, openGL knows what what vertices,
		// indices and vertex attributes to send to the shader
		glBindVertexArray(ro->VAO);
		glDrawElements(GL_TRIANGLES, mesh->m_indices.size(), GL_UNSIGNED_INT, 0);

	}

	// reset back to the default active texture
	glActiveTexture(GL_TEXTURE0);

	// finally, we have finished rendering the meshes
	// disable this shader
	glUseProgram(0);
}

void MultipleTextures_Tutorial::DestroyFBXSceneResource(FBXFile *a_pScene)
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

unsigned int MultipleTextures_Tutorial::loadTexture(const char* a_fileName, GLenum a_format)
{
	// load particle texture
	int width = 0;
	int height = 0;
	int format = 0;
	unsigned char* pixelData = stbi_load(a_fileName, &width, &height, &format, STBI_default);

	// create OpenGL texture handle
	unsigned int textureID = 0;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// set pixel data for texture
	glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, a_format, GL_UNSIGNED_BYTE, pixelData);

	// set filtering
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);

	// clear bound texture state so we don't accidentally change it
	glBindTexture(GL_TEXTURE_2D, 0);

	// delete pixel data
	delete[] pixelData;

	// return texture handle
	return textureID;
}

unsigned int MultipleTextures_Tutorial::loadCube(const char* a_topFileName,
												 const char* a_bottomFileName,
												 const char* a_northFileName,
												 const char* a_southFileName,
												 const char* a_eastFileName,
												 const char* a_westFileName,
												 GLenum a_topFormat,
												 GLenum a_bottomFormat,
												 GLenum a_northFormat,
												 GLenum a_southFormat,
												 GLenum a_eastFormat,
												 GLenum a_westFormat)
{
	// cube mapping must be enabled
	glEnable(GL_TEXTURE_CUBE_MAP);

	// genorate an  OpenGL texture and bind it as a GL_TEXTURE_CUBE_MAP
	unsigned int ID = 0;
	glGenTextures(1, &ID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, ID);

	// set some texture parameters...
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	// temporary width and height variables
	int width, height, format;

	// get the pixels for each of face of the cube and send the data to the graphics card
	unsigned char *pixels_top = stbi_load(a_topFileName, &width, &height, &format, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Y, 0, format, width, height, 0, a_topFormat, GL_UNSIGNED_BYTE, pixels_top);

	unsigned char *pixels_bottom = stbi_load(a_bottomFileName, &width, &height, &format, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Y, 0, format, width, height, 0, a_bottomFormat, GL_UNSIGNED_BYTE, pixels_bottom);

	unsigned char *pixels_north = stbi_load(a_northFileName, &width, &height, &format, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_Z, 0, format, width, height, 0, a_northFormat, GL_UNSIGNED_BYTE, pixels_north);

	unsigned char *pixels_south = stbi_load(a_southFileName, &width, &height, &format, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_Z, 0, format, width, height, 0, a_southFormat, GL_UNSIGNED_BYTE, pixels_south);

	unsigned char *pixels_west = stbi_load(a_westFileName, &width, &height, &format, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X, 0, format, width, height, 0, a_westFormat, GL_UNSIGNED_BYTE, pixels_west);

	unsigned char *pixels_east = stbi_load(a_eastFileName, &width, &height, &format, 0);
	glTexImage2D(GL_TEXTURE_CUBE_MAP_NEGATIVE_X, 0, format, width, height, 0, a_eastFormat, GL_UNSIGNED_BYTE, pixels_east);

	// delete the original image data
	free(pixels_top);
	free(pixels_bottom);
	free(pixels_north);
	free(pixels_south);
	free(pixels_east);
	free(pixels_west);

	return ID;
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new MultipleTextures_Tutorial();
	
	if (app->create("AIE - MultipleTextures_Tutorial",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}