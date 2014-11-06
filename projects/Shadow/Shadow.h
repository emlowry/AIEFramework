#pragma once

#include "Application.h"
#include "FBXFile.h"
#include <glm/glm.hpp>

struct Buffers
{
	unsigned int vao, vbo;
};

struct OGL_FBXRenderData
{
	unsigned int VBO; // vertex buffer object
	unsigned int IBO; // index buffer object
	unsigned int VAO; // vertex array object
};

// derived application class that wraps up all globals neatly
class Shadow : public Application
{
public:

	Shadow();
	virtual ~Shadow();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	void createShadowBuffer();
	void setUpLightAndShadowMatrix(float count);
	void create2DQuad();
	void renderShadowMap();
	void displayShadowMap();
	void drawScene();
	void InitFBXSceneResource(FBXFile *a_pScene);
	void UpdateFBXSceneResource(FBXFile *a_pScene);
	void DestroyFBXSceneResource(FBXFile *a_pScene);

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;
	glm::mat4	m_shadowProjectionViewMatrix;

	glm::vec4	m_lightDirection;

	unsigned int m_shadowWidth, m_shadowHeight, m_shadowFramebuffer, m_shadowTexture, m_2dprogram, m_shadowShader, m_program;

	Buffers m_2dQuad;
	FBXFile* m_fbx;
};