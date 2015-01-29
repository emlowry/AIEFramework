#pragma once

#include "Application.h"
#include <glm/glm.hpp>
#include <PxPhysicsAPI.h>

using namespace physx;

// derived application class that wraps up all globals neatly
class PhysXTutorials : public Application
{
public:

	PhysXTutorials();
	virtual ~PhysXTutorials();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	void setUpPhysX();
	void setUpVisualDebugger();
	void updatePhysX();
	void cleanUpPhysX();

	void addWidget(PxShape* shape, PxActor* actor);
	void addSphere(PxShape* pShape, PxActor* actor);
	void addBox(PxShape* pShape, PxActor* actor);
	void addPlane(PxShape* pShape, PxActor* actor);

	void fire();

	void tutorial_1();

	float m_lastFireTime = 0.0f;

	float m_fireInterval = 0.25f;
	float m_bulletSpeed = 50.0f;
	float m_bulletDensity = 50.0f;
	float m_bulletSize = 0.5f;

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;
};

class myAllocator : public PxAllocatorCallback
{
public:
	virtual ~myAllocator() {}
	virtual void* allocate(size_t size, const char* typeName, const char* filename, int line)
	{
		void* pointer = _aligned_malloc(size, 16);
		return pointer;
	}
	virtual void deallocate(void* ptr)
	{
		_aligned_free(ptr);
	}
};
