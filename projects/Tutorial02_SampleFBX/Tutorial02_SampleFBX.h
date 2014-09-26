#pragma once

#include "Application.h"
#include "FBXFile.h"
#include <glm/glm.hpp>

struct OGL_FBXRenderData
{
	unsigned int VBO; // vertex buffer object
	unsigned int IBO; // index buffer object
	unsigned int VAO; // vertex array object
};

// derived application class that wraps up all globals neatly
class Tutorial02_SampleFBX : public Application
{
public:

	Tutorial02_SampleFBX();
	virtual ~Tutorial02_SampleFBX();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	void InitFBXSceneResource(FBXFile *a_pScene);
	void UpdateFBXSceneResource(FBXFile *a_pScene);
	void RenderFBXSceneResource(FBXFile *a_pScene, glm::mat4 a_view, glm::mat4 a_projection);
	void DestroyFBXSceneResource(FBXFile *a_pScene);

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	unsigned int m_shader;
	FBXFile *m_fbx;
};