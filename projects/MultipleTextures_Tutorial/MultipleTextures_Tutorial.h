#pragma once

#include "Application.h"
#include "FBXFile.h"
#include <glm/glm.hpp>
#include <GL/glew.h>

struct OGL_FBXRenderData
{
	unsigned int VBO; // vertex buffer object
	unsigned int IBO; // index buffer object
	unsigned int VAO; // vertex array object
};

// derived application class that wraps up all globals neatly
class MultipleTextures_Tutorial : public Application
{
public:

	MultipleTextures_Tutorial();
	virtual ~MultipleTextures_Tutorial();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	void InitFBXSceneResource(FBXFile *a_pScene);
	void UpdateFBXSceneResource(FBXFile *a_pScene);
	void RenderFBXSceneResource(FBXFile *a_pScene, glm::mat4 a_view, glm::mat4 a_projection);
	void DestroyFBXSceneResource(FBXFile *a_pScene);

	unsigned int loadTexture(const char* a_fileName, GLenum a_format = GL_RGBA);
	unsigned int loadCube(const char* a_topFileName,
						  const char* a_bottomFileName,
						  const char* a_northFileName,
						  const char* a_southFileName,
						  const char* a_eastFileName,
						  const char* a_westFileName,
						  GLenum a_topFormat = GL_RGBA,
						  GLenum a_bottomFormat = GL_RGBA,
						  GLenum a_northFormat = GL_RGBA,
						  GLenum a_southFormat = GL_RGBA,
						  GLenum a_eastFormat = GL_RGBA,
						  GLenum a_westFormat = GL_RGBA);

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	unsigned int m_shader;
	FBXFile *m_fbx;

	unsigned int m_decayTexture;
	float m_decayValue;
	unsigned int m_metallicTexture;

	unsigned int m_cubemap_texture;
	unsigned int m_skybox_vao;
	unsigned int m_skybox_vbo;
	unsigned int m_skybox_ibo;
	unsigned int m_skybox_shader;
};