#pragma once

#include "Application.h"
#include <glm/glm.hpp>
#include <vector>

#include "Rectangle.h"
#include "NavMesh.h"
#include "Agent.h"

// derived application class that wraps up all globals neatly
class AIAssessment : public Application
{
public:

	AIAssessment();
	virtual ~AIAssessment();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	void GenerateNavMesh();

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	std::vector<Rectangle>	m_obstacles;
	std::vector<glm::vec2>	m_patrol;
	unsigned int m_patrolIndex;
	bool m_patrolPersuit;
	std::vector<glm::vec2>  m_path;
	unsigned int m_pathIndex;

	Agent m_patrolAgent;
	Agent m_pathAgent;

	NavMesh	m_mesh;
};