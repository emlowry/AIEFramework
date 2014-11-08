#pragma once

#include "Application.h"
#include <glm/glm.hpp>

// derived application class that wraps up all globals neatly
class SceneManagement : public Application
{
public:

	SceneManagement();
	virtual ~SceneManagement();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	/**
	 * a_plane.xyz = normal, a_plane.w = distance from origin
	 * a_sphere.xyz = center, abs(a_sphere.w) = radius
	 */
	int planeSphereTest(const glm::vec4& a_plane, const glm::vec4& a_sphere);
	void getFrustrumPlanes(const glm::mat4& a_transform, glm::vec4* a_planes);


	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;
};