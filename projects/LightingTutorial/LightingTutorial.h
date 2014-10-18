#pragma once

#include "Application.h"
#include "FBXFile.h"
#include <glm/glm.hpp>

// derived application class that wraps up all globals neatly
class LightingTutorial : public Application
{
public:

	LightingTutorial();
	virtual ~LightingTutorial();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	void createOpenGLBuffers(FBXFile* a_fbx);
	void cleanupOpenGLBuffers(FBXFile* a_fbx);

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	unsigned int m_vertShader;
	unsigned int m_fragShader;
	unsigned int m_shader;

	FBXFile* m_fbx;
};