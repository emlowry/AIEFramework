#pragma once

#include "Application.h"
#include "FBXFile.h"
#include <glm/glm.hpp>
#include <PxPhysicsAPI.h>

using namespace physx;

struct OGL_FBXRenderData
{
	unsigned int VBO; // vertex buffer object
	unsigned int IBO; // index buffer object
	unsigned int VAO; // vertex array object
};

//simple scene node
struct SceneNode
{
	PxRigidActor* physXActor; //actor which controls this node
	FBXFile* fbxFile; //fbx file to use for this node
	glm::mat4 transform; //the nodes transform
};

struct RagdollNode
{
	PxQuat globalRotation;  //rotation of this link in model space - we could have done this
							// relative to the parent node but it's harder to visualize when setting
							// up the data by hand
	PxVec3 scaledGobalPos;	//Position of the link centre in world space which is calculated when we
							// process the node.  It's easiest if we store it here so we have it
							// when we transform the child
	int parentNodeIdx;	//Index of the parent node
	float halfLength;	//half length of the capsule for this node
	float radius;	//radius of capsule for thisndoe
	float parentLinkPos;	//relative position of link centre in parent to this node.  0 is the
							// centre of hte node, -1 is left end of capsule and 1 is right end of
							// capsule relative to x 
	float childLinkPos;	//relative position of link centre in child
	char* name;	//name of link
	PxArticulationLink* linkPtr;	//pointer to link if we are using articulation
	PxRigidDynamic* actorPtr;	//Pointer the PhysX actor which is linked to this node if we are
								// using seperate actors

	//constructor
	RagdollNode(PxQuat _globalRotation, int _parentNodeIdx, float _halfLength, float _radius,
		float _parentLinkPos, float _childLinkPos, char* _name)
		: globalRotation(_globalRotation), parentNodeIdx(_parentNodeIdx), halfLength(_halfLength),
		  radius(_radius), parentLinkPos(_parentLinkPos), childLinkPos(_childLinkPos), name(_name) {}
};

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

	void InitFBXSceneResource(FBXFile *a_pScene);
	void DestroyFBXSceneResource(FBXFile *a_pScene);

	void setUpPhysX();
	void setUpVisualDebugger();
	void updatePhysX();
	void cleanUpPhysX();

	PxRigidDynamic* addFBXWithConvexCollision(FBXFile* fbxFile, PxTransform transform);
	PxRigidStatic* addStaticFBXWithTriangleCollision(FBXFile* fbxFile, PxTransform transform);
	PxRigidStatic* addStaticHeightMapCollision(PxTransform transform);
	void setFBXTransform(PxTransform transform, SceneNode* sceneNode);
	void renderFBX(SceneNode* sceneNode);

	void addWidget(PxShape* shape, PxActor* actor);
	void addSphere(PxShape* pShape, PxActor* actor);
	void addBox(PxShape* pShape, PxActor* actor);
	void addPlane(PxShape* pShape, PxActor* actor);
	void addCapsule(PxShape* pShape, PxActor* actor);
	void addHeightField(PxShape* pShape, PxActor* actor);

	void controlPlayer(float a_deltaTime);
	void addPlatforms();
	PxCloth* createCloth(const glm::vec3& a_position,
						 unsigned int& a_vertexCount,
						 unsigned int& a_indexCount,
						 const glm::vec3* a_vertices,
						 unsigned int* a_indices);
	void makeCloth();
	void updateCloth();
	void drawCloth(const glm::mat4& a_projectionMatrix, const glm::mat4& a_viewMatrix);
	void destroyCloth();
	void pickingExample1();

	void fire();
	void reset();
	void clearScene();

	void tutorial_1();
	void tutorial_2();
	void tutorial_3();

	PxArticulation* makeRagdoll(RagdollNode** nodeArray, PxTransform worldPos, float scaleFactor,
								PxU32 filterGroup, PxU32 filterMask, const char* name = nullptr);

	float m_lastFireTime = 0.0f;

	float m_fireInterval = 0.25f;
	float m_bulletSpeed = 50.0f;
	float m_bulletDensity = 50.0f;
	float m_bulletSize = 0.5f;

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;

	PxRigidDynamic* m_playerActor = nullptr;
	PxTransform m_startingPlayerPos;

	unsigned int m_shader;

	//very simple scene graph
	std::vector<SceneNode*> m_sceneNodes;

	bool m_reset = false;

	PxCloth*		m_cloth;

	unsigned int	m_clothShader;
	unsigned int	m_clothTexture;

	unsigned int	m_clothIndexCount;
	unsigned int	m_clothVertexCount;
	glm::vec3*		m_clothPositions;

	unsigned int	m_clothVAO, m_clothVBO, m_clothTextureVBO, m_clothIBO;
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

struct StaticObject
{
	PxVec3 centre;
	PxVec3 extents;
	bool trigger;
	char* name;
public:
	StaticObject(float x, float y, float z, float length, float height, float width, bool trigger, char* name)
	{
		centre.x = x;
		centre.y = y;
		centre.z = z;
		extents.x = width;
		extents.y = height;
		extents.z = length;
		this->trigger = trigger;
		this->name = name;
	}
};

StaticObject* gStaticObject[] = {
	new StaticObject(0, 1, 0, 2, 1.0f, 2, 0, "Platform"),
	new StaticObject(7, 1, 0, 2, 1.0f, 2, 0, "Platform"),
	new StaticObject(14, 2, 0, 2, 2.0f, 2, 0, "Platform"),
	new StaticObject(22, 4, 8, 10, 4.0f, 2, 0, "Platform"),
	new StaticObject(22, 10, 17, 0.5f, 2, 0.5f, 1, "Pickup1"),
};
