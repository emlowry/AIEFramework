#pragma once

#include "Application.h"
#include <glm/glm.hpp>

//\=========================================================================
//\ A Simple Vertex Structure
//\  - Two integers for position
//\  - Four floats for colour (colour is required to be a float array of 16
//\    bytes length in all recent versions of openGL)
//\=========================================================================
struct Vertex4p4c
{
	glm::vec4 position;
	glm::vec4 colour;
};

// derived application class that wraps up all globals neatly
class Tutorial_2 : public Application
{
public:

	Tutorial_2();
	virtual ~Tutorial_2();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	// function to create a grid
	void GenerateGrid(unsigned int rows, unsigned int cols);

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;
	float		m_time;

	//Our vertex and index buffers
	unsigned int      m_VAO;
	unsigned int      m_VBO;
	unsigned int      m_IBO;

	//Where we save our shaderID
	unsigned int 	m_programID;
};