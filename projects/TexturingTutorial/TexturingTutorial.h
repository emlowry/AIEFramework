#pragma once

#include "Application.h"
#include <glm/glm.hpp>

// a basic vertex structure supporting position, colour and texture coordinate
struct BasicVertex
{
	glm::vec4 position;
	glm::vec4 colour;
	glm::vec2 texCoord;
};

// derived application class that wraps up all globals neatly
class TexturingTutorial : public Application
{
public:

	TexturingTutorial();
	virtual ~TexturingTutorial();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	unsigned int m_texture;

	unsigned int m_vao;
	unsigned int m_vbo;
	unsigned int m_ibo;

	unsigned int m_vertShader;
	unsigned int m_fragShader;
	unsigned int m_shader;
};