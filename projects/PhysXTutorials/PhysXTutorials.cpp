#include "PhysXTutorials.h"
#include "PxToolkit.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <vector>
#include <iostream>
#include <functional>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

using namespace physx;

PxFoundation* g_PhysicsFoundation = nullptr;
PxPhysics* g_Physics = nullptr;
PxScene* g_PhysicsScene = nullptr;

PxDefaultErrorCallback gDefaultErrorCallback;
PxDefaultAllocator gDefaultAllocatorCallback;
PxSimulationFilterShader gDefaultFilterShader = PxDefaultSimulationFilterShader;
PxMaterial* g_PhysicsMaterial = nullptr;
PxCooking* g_PhysicsCooker = nullptr;
std::vector<PxRigidActor*> g_PhysXActors;

struct FilterGroup
{
	enum Enum
	{
		ePLAYER = (1 << 0),
		ePLATFORM = (1 << 1),
		eGROUND = (1 << 2)
	};
};


//helper function to set up filtering
void setupFiltering(PxRigidActor* actor, PxU32 filterGroup, PxU32 filterMask)
{
	PxFilterData filterData;
	filterData.word0 = filterGroup; // word0 = own ID
	filterData.word1 = filterMask;  // word1 = ID mask to filter pairs that trigger a contact callback;
	const PxU32 numShapes = actor->getNbShapes();
	PxShape** shapes = (PxShape**)_aligned_malloc(sizeof(PxShape*)*numShapes, 16);
	actor->getShapes(shapes, numShapes);
	for (PxU32 i = 0; i < numShapes; i++)
	{
		PxShape* shape = shapes[i];
		shape->setSimulationFilterData(filterData);
	}
	_aligned_free(shapes);
}

//derived class to overide the call backs we are interested in...
class MycollisionCallBack : public PxSimulationEventCallback
{
	typedef std::function<void(void)> Callback;
	virtual void MycollisionCallBack::onContact(const PxContactPairHeader& pairHeader, const PxContactPair* pairs, PxU32 nbPairs)
	{
		bool reset = false;
		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxContactPair& cp = pairs[i];
			if (cp.events & PxPairFlag::eNOTIFY_TOUCH_FOUND)
			{
				reset = true;
				std::cout << "collision: " << pairHeader.actors[0]->getName() << " : " << pairHeader.actors[1]->getName() << std::endl;
			}
		}
		/*if (reset)
			m_reset();/**/
	}
	//we have to create versions of the following functions even though we don't do anything with them...
	virtual void    onTrigger(PxTriggerPair* pairs, PxU32 nbPairs)
	{
		bool reset = false;
		for (PxU32 i = 0; i < nbPairs; i++)
		{
			const PxTriggerPair& cp = pairs[i];
			if (cp.status & PxPairFlag::eNOTIFY_TOUCH_FOUND)
			{
				reset = true;
				std::cout << "trigger ";
				if (nullptr != cp.triggerShape && nullptr != cp.triggerShape->getName())
					std::cout << cp.triggerShape->getName() << " ";
				std::cout << "tripped";
				if (nullptr != cp.otherShape && nullptr != cp.otherShape->getName())
					std::cout << " by " << cp.otherShape->getName();
				std::cout << std::endl;
			}
		}
		if (reset)
			m_reset();
	};
	virtual void	onConstraintBreak(PxConstraintInfo*, PxU32){};
	virtual void	onWake(PxActor**, PxU32){};
	virtual void	onSleep(PxActor**, PxU32){};

	Callback m_reset;

public:

	MycollisionCallBack(Callback a_reset) : m_reset(a_reset) {}
};


glm::mat4 Px2Glm(const PxMat44& a_m)
{
	return glm::mat4(a_m.column0.x, a_m.column0.y, a_m.column0.z, a_m.column0.w,
		a_m.column1.x, a_m.column1.y, a_m.column1.z, a_m.column1.w,
		a_m.column2.x, a_m.column2.y, a_m.column2.z, a_m.column2.w,
		a_m.column3.x, a_m.column3.y, a_m.column3.z, a_m.column3.w);
}

glm::vec3 Px2GlV3(const PxVec3& a_v)
{
	return glm::vec3(a_v.x, a_v.y, a_v.z);
}


PhysXTutorials::PhysXTutorials()
{

}

PhysXTutorials::~PhysXTutorials()
{

}

bool PhysXTutorials::onCreate(int a_argc, char* a_argv[]) 
{
	// initialise the Gizmos helper class
	Gizmos::create();

	// create a world-space matrix for a camera
	m_cameraMatrix = glm::inverse( glm::lookAt(glm::vec3(3.5,1.25,2),glm::vec3(0,0.75,0.5), glm::vec3(0,1,0)) );

	// get window dimensions to calculate aspect ratio
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);

	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, width / (float)height, 0.1f, 1000.0f);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// load the shader
	const char* aszInputs[] = { "Position", "Normals", "TexCoord" };
	const char* aszOutputs[] = { "FragColour" };

	// load shader internally calls glCreateShader...
	GLuint vshader = Utility::loadShader("./shaders/shader.vert", GL_VERTEX_SHADER);
	GLuint pshader = Utility::loadShader("./shaders/shader.frag", GL_FRAGMENT_SHADER);

	m_shader = Utility::createProgram(vshader, 0, 0, 0, pshader, 3, aszInputs, 1, aszOutputs);

	// free our shader once we built our program
	glDeleteShader(vshader);
	glDeleteShader(pshader);

	// Set up PhysX
	setUpPhysX();
	tutorial_3();
	setUpVisualDebugger();

	return true;
}

void PhysXTutorials::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement( m_cameraMatrix, a_deltaTime, 10 );

	// clear all gizmos from last frame
	Gizmos::clear();
	
	// add an identity matrix gizmo
	Gizmos::addTransform( glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1) );

	/*controlPlayer(a_deltaTime);

	// fire bullet
	if (glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS)
	{
		float time = Utility::getTotalTime();
		if (time - m_lastFireTime >= m_fireInterval)
		{
			fire();
			m_lastFireTime = time;
		}
	}/**/

	// update PhysX
	updatePhysX();
	pickingExample1();

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

void PhysXTutorials::onDraw() 
{
	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse( m_cameraMatrix );

	// draw the gizmos from this frame
	Gizmos::draw(m_projectionMatrix, viewMatrix);

	// get window dimensions for 2D orthographic projection
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);
	Gizmos::draw2D(glm::ortho<float>(0.0f, (float)width, 0.0f, (float)height, -1.0f, 1.0f));

	for (auto node : m_sceneNodes)
		renderFBX(node);
}

void PhysXTutorials::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();
	cleanUpPhysX();

	glDeleteShader(m_shader);

	for (auto node : m_sceneNodes)
	{
		DestroyFBXSceneResource(node->fbxFile);
		node->fbxFile->unload();
		delete node->fbxFile;
		node->fbxFile = NULL;
		delete node;
	}
	m_sceneNodes.clear();
}

void PhysXTutorials::InitFBXSceneResource(FBXFile *a_pScene)
{
	// how manu meshes and materials are stored within the fbx file
	unsigned int meshCount = a_pScene->getMeshCount();
	unsigned int matCount = a_pScene->getMaterialCount();

	// loop through each mesh
	for (unsigned int i = 0; i<meshCount; ++i)
	{
		// get the current mesh
		FBXMeshNode *pMesh = a_pScene->getMeshByIndex(i);

		// genorate our OGL_FBXRenderData for storing the meshes VBO, IBO and VAO
		// and assign it to the meshes m_userData pointer so that we can retrive 
		// it again within the render function
		OGL_FBXRenderData *ro = new OGL_FBXRenderData();
		pMesh->m_userData = ro;

		// OPENGL: genorate the VBO, IBO and VAO
		glGenBuffers(1, &ro->VBO);
		glGenBuffers(1, &ro->IBO);
		glGenVertexArrays(1, &ro->VAO);

		// OPENGL: Bind  VAO, and then bind the VBO and IBO to the VAO
		glBindVertexArray(ro->VAO);
		glBindBuffer(GL_ARRAY_BUFFER, ro->VBO);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ro->IBO);

		// Send the vertex data to the VBO
		glBufferData(GL_ARRAY_BUFFER, pMesh->m_vertices.size() * sizeof(FBXVertex), pMesh->m_vertices.data(), GL_STATIC_DRAW);

		// send the index data to the IBO
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, pMesh->m_indices.size() * sizeof(unsigned int), pMesh->m_indices.data(), GL_STATIC_DRAW);

		// enable the attribute locations that will be used on our shaders
		glEnableVertexAttribArray(0);
		glEnableVertexAttribArray(1);
		glEnableVertexAttribArray(2);

		// tell our shaders where the information within our buffers lie
		// eg: attrubute 0 is expected to be the verticy's position. it should be 4 floats, representing xyzw
		// eg: attrubute 1 is expected to be the verticy's normal. it should be 4 floats, representing xyzw
		// eg: attrubute 2 is expected to be the verticy's texture coordinate. it should be 2 floats, representing U and V
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::PositionOffset);
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::NormalOffset);
		glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char *)FBXVertex::TexCoord1Offset);

		// finally, where done describing our mesh to the shader
		// we can describe the next mesh
		glBindVertexArray(0);
	}
}

void PhysXTutorials::DestroyFBXSceneResource(FBXFile *a_pScene)
{
	// how manu meshes and materials are stored within the fbx file
	unsigned int meshCount = a_pScene->getMeshCount();
	unsigned int matCount = a_pScene->getMaterialCount();

	// remove meshes
	for (unsigned int i = 0; i<meshCount; i++)
	{
		// Get the current mesh and retrive the render data we assigned to m_userData
		FBXMeshNode* pMesh = a_pScene->getMeshByIndex(i);
		OGL_FBXRenderData *ro = (OGL_FBXRenderData *)pMesh->m_userData;

		// delete the buffers and free memory from the graphics card
		glDeleteBuffers(1, &ro->VBO);
		glDeleteBuffers(1, &ro->IBO);
		glDeleteVertexArrays(1, &ro->VAO);

		// this is memory we created earlier in the InitFBXSceneResources function
		// make sure to destroy it
		delete ro;

	}

	// loop through each of the materials
	for (unsigned int i = 0; i<matCount; i++)
	{
		// get the current material
		FBXMaterial *pMaterial = a_pScene->getMaterialByIndex(i);
		if (nullptr != pMaterial)
		{
			for (int j = 0; j<FBXMaterial::TextureTypes_Count; j++)
			{
				// delete the texture if it was loaded
				if (nullptr != pMaterial->textures[j] && pMaterial->textures[j]->handle != 0)
					glDeleteTextures(1, &(pMaterial->textures[j]->handle));
			}
		}
	}
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new PhysXTutorials();
	
	if (app->create("AIE - PhysXTutorials",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}

PxFilterFlags myFliterShader(
	PxFilterObjectAttributes attributes0, PxFilterData filterData0,
	PxFilterObjectAttributes attributes1, PxFilterData filterData1,
	PxPairFlags& pairFlags, const void* constantBlock, PxU32 constantBlockSize)
{
	// let triggers through
	if (PxFilterObjectIsTrigger(attributes0) || PxFilterObjectIsTrigger(attributes1))
	{
		pairFlags = PxPairFlag::eTRIGGER_DEFAULT;
		return PxFilterFlag::eDEFAULT;
	}
	// generate contacts for all that were not filtered above
	pairFlags = PxPairFlag::eCONTACT_DEFAULT;
	// trigger the contact callback for pairs (A,B) where
	// the filtermask of A contains the ID of B and vice versa.
	if ((filterData0.word0 & filterData1.word1) && (filterData1.word0 & filterData0.word1))
		pairFlags |= PxPairFlag::eNOTIFY_TOUCH_FOUND;

	return PxFilterFlag::eDEFAULT;
}

void PhysXTutorials::setUpPhysX()
{
	PxAllocatorCallback *myCallback = new myAllocator();
	g_PhysicsFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, *myCallback, gDefaultErrorCallback);
	g_Physics = PxCreatePhysics(PX_PHYSICS_VERSION, *g_PhysicsFoundation, PxTolerancesScale());
	g_PhysicsCooker = PxCreateCooking(PX_PHYSICS_VERSION, *g_PhysicsFoundation, PxCookingParams(PxTolerancesScale()));
	PxInitExtensions(*g_Physics);
	//create physics material
	g_PhysicsMaterial = g_Physics->createMaterial(0.5f, 0.5f, 0.6f);
	PxSceneDesc sceneDesc(g_Physics->getTolerancesScale());
	sceneDesc.gravity = PxVec3(0, -10.0f, 0);
	sceneDesc.filterShader = myFliterShader;
	sceneDesc.cpuDispatcher = PxDefaultCpuDispatcherCreate(1);
	g_PhysicsScene = g_Physics->createScene(sceneDesc);

	if (g_PhysicsScene)
	{
		std::cout << "start physx scene2";
	}

	PxSimulationEventCallback* mycollisionCallBack =
		new MycollisionCallBack([this] { this->reset(); });  //instantiate our class to overload call backs
	g_PhysicsScene->setSimulationEventCallback(mycollisionCallBack); //tell the scene to use our call back class
}

void PhysXTutorials::setUpVisualDebugger()
{
	// check if PvdConnection manager is available on this platform
	if (NULL == g_Physics->getPvdConnectionManager())
		return;
	// setup connection parameters
	const char*     pvd_host_ip = "127.0.0.1";  // IP of the PC which is running PVD
	int             port = 5425;         // TCP port to connect to, where PVD is listening
	unsigned int    timeout = 100;          // timeout in milliseconds to wait for PVD to respond,
	// consoles and remote PCs need a higher timeout.
	PxVisualDebuggerConnectionFlags connectionFlags = PxVisualDebuggerConnectionFlag::Debug
		| PxVisualDebuggerConnectionFlag::Profile | PxVisualDebuggerConnectionFlag::Memory;
	// and now try to connect
	PxVisualDebuggerExt::createConnection(g_Physics->getPvdConnectionManager(),
		pvd_host_ip, port, timeout, connectionFlags);
	//    pvd_host_ip, port, timeout, connectionFlags));
}

void PhysXTutorials::cleanUpPhysX()
{
	g_PhysicsCooker->release();
	g_PhysicsScene->release();
	g_Physics->release();
	g_PhysicsFoundation->release();
	m_playerActor = nullptr;
	g_PhysXActors.clear();
}

void PhysXTutorials::updatePhysX()
{
	g_PhysicsScene->simulate(1 / 60.f);
	while (g_PhysicsScene->fetchResults() == false)
	{
		// don’t need to do anything here yet but we still need to do the fetch
	}

	// Add widgets to represent all the phsyX actors which are in the scene
	for (auto actor : g_PhysXActors)
	{
		PxU32 nShapes = actor->getNbShapes();
		PxShape** shapes = new PxShape*[nShapes];
		actor->getShapes(shapes, nShapes);
		// Render all the shapes in the physx actor (for early tutorials there is just one)
		while (nShapes--)
		{
			addWidget(shapes[nShapes], actor);
		}
		delete[] shapes;
	}
}

PxConvexMesh* makeConvexMesh(FBXMeshNode* mesh)
{
	int numberVerts = mesh->m_vertices.size();
	PxVec3 *verts = new PxVec3[numberVerts];
	//unfortunately we need to convert verts from glm to Px format
	for (int vertIDX = 0; vertIDX< numberVerts; vertIDX++)
	{
		glm::vec4 temp = mesh->m_vertices[vertIDX].position;
		verts[vertIDX] = PxVec3(temp.x, temp.y, temp.z) * .01f;  //scale the mesh because it's way too big
	}
	//control structure for convex mesh cooker
	PxConvexMeshDesc convexDesc;
	convexDesc.points.count = numberVerts;
	convexDesc.points.stride = sizeof(PxVec3);
	convexDesc.points.data = verts;
	convexDesc.flags = PxConvexFlag::eCOMPUTE_CONVEX;
	PxToolkit::MemoryOutputStream buf;
	//cook the mesh
	if (!g_PhysicsCooker->cookConvexMesh(convexDesc, buf))
		return NULL;
	PxToolkit::MemoryInputData input(buf.getData(), buf.getSize());
	PxConvexMesh* result = g_Physics->createConvexMesh(input);
	delete(verts);
	return result;
}

PxRigidDynamic* PhysXTutorials::addFBXWithConvexCollision(FBXFile* fbxFile, PxTransform transform)
{
	//create a phsyics material for our tank wiuth low friction
	//because we want to push it around
	PxMaterial*  tankMaterial = g_Physics->createMaterial(0.2f, 0.2f, 0.2f);    //static friction, dynamic friction, restitution
	//load and cook a mesh
	PxRigidDynamic*dynamicActor;
	float density = 10;
	if (0 < fbxFile->getMeshCount())
	{
		PxConvexMesh* convexMesh = makeConvexMesh(fbxFile->getMeshByIndex(0));
		dynamicActor = PxCreateDynamic(*g_Physics, transform, (PxConvexMeshGeometry)convexMesh, *tankMaterial, density);
	}
	else
	{
		PxBoxGeometry box(1, 1, 1);
		dynamicActor = PxCreateDynamic(*g_Physics, transform, box, *tankMaterial, density);
	}
	dynamicActor->setLinearDamping(.5f);
	dynamicActor->setAngularDamping(100);
	for (unsigned int i = 1; i < fbxFile->getMeshCount(); ++i)
	{
		PxConvexMesh* convexMesh = makeConvexMesh(fbxFile->getMeshByIndex(i));
		PxTransform pose = PxTransform(PxVec3(0.0f, 0, 0.0f), PxQuat(0, PxVec3(0.0f, 0.0f, 1.0f)));
		dynamicActor->createShape((PxConvexMeshGeometry)convexMesh, *g_PhysicsMaterial, pose);
		//add it to the physX scene
	}
	SceneNode* sceneNode = new SceneNode();
	sceneNode->fbxFile = fbxFile; //set up the FBXFile
	sceneNode->physXActor = dynamicActor; //link our scene node to the physics Object
	m_sceneNodes.push_back(sceneNode); //add it to the scene graph
	g_PhysicsScene->addActor(*dynamicActor);
	return dynamicActor;
}

PxTriangleMesh* makeTriangleMesh(FBXMeshNode* mesh)
{
	int numberVerts = mesh->m_vertices.size();
	PxVec3 *verts = new PxVec3[numberVerts];
	for (int vertIDX = 0; vertIDX< numberVerts; vertIDX++)
	{
		glm::vec4 temp = mesh->m_vertices[vertIDX].position;
		verts[vertIDX] = PxVec3(temp.x, temp.y, temp.z) * .01f;  //scale the mesh because it's way too big
	}
	PxTriangleMeshDesc meshDesc;
	meshDesc.points.count = numberVerts;
	meshDesc.points.stride = sizeof(PxVec3);
	meshDesc.points.data = verts;
	int triCount = mesh->m_indices.size() / 3;
	meshDesc.triangles.count = triCount;
	meshDesc.triangles.stride = 3 * sizeof(PxU32);
	//unsigned int* indices32	=		mesh->m_indices.data();
	meshDesc.triangles.data = mesh->m_indices.data();
	PxToolkit::MemoryOutputStream buf;
	if (!g_PhysicsCooker->cookTriangleMesh(meshDesc, buf))
		return nullptr;
	PxToolkit::MemoryInputData input(buf.getData(), buf.getSize());
	PxTriangleMesh* result = g_Physics->createTriangleMesh(input);
	delete(verts);
	return result;
}

PxRigidStatic* PhysXTutorials::addStaticFBXWithTriangleCollision(FBXFile* fbxFile, PxTransform transform)
{
	//load and cook a mesh
	PxRigidStatic *staticObject;
	if (0 < fbxFile->getMeshCount())
	{
		PxTriangleMesh* triangleMesh = makeTriangleMesh(fbxFile->getMeshByIndex(0));
		staticObject = PxCreateStatic(*g_Physics, transform, (PxTriangleMeshGeometry)triangleMesh, *g_PhysicsMaterial);
	}
	else
	{
		PxBoxGeometry box(1, 1, 1);
		staticObject = PxCreateStatic(*g_Physics, transform, box, *g_PhysicsMaterial);
	}
	for (unsigned int i = 0; i < fbxFile->getMeshCount(); ++i)
	{
		PxTriangleMesh* triangleMesh = makeTriangleMesh(fbxFile->getMeshByIndex(i));
		PxTransform pose = PxTransform(PxVec3(0.0f, 0, 0.0f), PxQuat(0, PxVec3(0.0f, 0.0f, 1.0f)));
		staticObject->createShape((PxTriangleMeshGeometry)triangleMesh, *g_PhysicsMaterial, pose);
	}
	SceneNode* sceneNode = new SceneNode();
	sceneNode->fbxFile = fbxFile; //set up the FBXFile
	sceneNode->physXActor = staticObject; //link our scene node to the physics Object
	m_sceneNodes.push_back(sceneNode); //add it to the scene graph
	//add it to the physX scene
	g_PhysicsScene->addActor(*staticObject);
	return staticObject;
}

PxRigidStatic* PhysXTutorials::addStaticHeightMapCollision(PxTransform transform)
{
	unsigned int numRows = 150;
	unsigned int numCols = 150;
	float heightScale = .0006f;
	float rowScale = 5;
	float colScale = 5;
	glm::vec3 center = Px2GlV3(transform.p);
	PxHeightFieldSample* samples = (PxHeightFieldSample*)_aligned_malloc(sizeof(PxHeightFieldSample)*(numRows*numCols), 16);
	bool enabled = true;
	//make height map
	PxHeightFieldSample* samplePtr = samples;
	for (unsigned int row = 0; row<numRows; ++row)
	{
		for (unsigned int col = 0; col<numCols; ++col)
		{
			float height = sin(row / 10.0f) * cos(col / 10.0f);
			samplePtr->height = height * 30000.0f;
			samplePtr->materialIndex1 = 0;
			samplePtr->materialIndex0 = 0;
			samplePtr->clearTessFlag();
			++samplePtr;
		}
	}
	PxHeightFieldDesc hfDesc;
	hfDesc.format = PxHeightFieldFormat::eS16_TM;
	hfDesc.nbColumns = numCols;
	hfDesc.nbRows = numRows;
	hfDesc.samples.data = samples;
	hfDesc.samples.stride = sizeof(PxHeightFieldSample);

	PxHeightField* aHeightField = g_Physics->createHeightField(hfDesc);
	PxHeightFieldGeometry hfGeom(aHeightField, PxMeshGeometryFlags(), heightScale, rowScale, colScale);
	PxRigidStatic* staticObject = PxCreateStatic(*g_Physics, transform, (PxHeightFieldGeometry)hfGeom, *g_PhysicsMaterial);
	g_PhysicsScene->addActor(*staticObject);
	g_PhysXActors.push_back(staticObject);
	return staticObject;
}

void PhysXTutorials::setFBXTransform(PxTransform transform, SceneNode* sceneNode)
{
	PxMat44 m(transform); //convert tranform to a 4X4 matrix
	//then convert it to a glm matrix... 
	glm::mat4 M(m.column0.x, m.column0.y, m.column0.z, m.column0.w,
				m.column1.x, m.column1.y, m.column1.z, m.column1.w,
				m.column2.x, m.column2.y, m.column2.z, m.column2.w,
				m.column3.x, m.column3.y, m.column3.z, m.column3.w);
	sceneNode->transform = M;  //set the meshes global tranform to this
}

void PhysXTutorials::renderFBX(SceneNode* sceneNode)
{
	//get a pointer to the physX actor for this scene node
	PxRigidActor* thisActor = sceneNode->physXActor;
	//set the transform for this scene node to be that of the physx actor
	setFBXTransform(thisActor->getGlobalPose(), sceneNode);
	//we store the transform in the scene node just in case we need it later
	FBXFile *m_file = sceneNode->fbxFile;
	if (m_file == NULL) //if we haven't loaded our model then don't render it
		return;
	glm::mat4 viewMatrix = glm::inverse(m_cameraMatrix);
	//use the selected shader program
	glUseProgram(m_shader);
	// send across our uniform variables, 
	//a combined projection * view matrix, and a world matrix

	//set up various uniforms in the shader so the FBX renders correctly
	unsigned int projectionView = glGetUniformLocation(m_shader, "projectionView");
	unsigned int world = glGetUniformLocation(m_shader, "world");
	unsigned int diffuseMap = glGetUniformLocation(m_shader, "diffuseMap");
	//unsigned int colour = glGetUniformLocation(m_shader, "colour");
	unsigned int ambient = glGetUniformLocation(m_shader, "ambientLight");
	unsigned int  lightDir = glGetUniformLocation(m_shader, "lightDir");

	glUniformMatrix4fv(projectionView, 1, false, glm::value_ptr(m_projectionMatrix * viewMatrix));

	glm::vec4 currentLightDir = glm::normalize(glm::vec4(-1, -1, 0, 0));
	//itterate through all the submeshes in each tank
	for (unsigned int i = 0; i < m_file->getMeshCount(); ++i)
	{
		FBXMeshNode* mesh = m_file->getMeshByIndex(i);

		// get the render data attached to the m_userData pointer for this mesh
		OGL_FBXRenderData *ro = (OGL_FBXRenderData *)mesh->m_userData;
		
		// Bind the texture to one of the ActiveTextures
		glActiveTexture(GL_TEXTURE1);
		glBindTexture(GL_TEXTURE_2D, mesh->m_material->textures[FBXMaterial::DiffuseTexture]->handle);

		// reset back to the default active texture
		glActiveTexture(GL_TEXTURE0);

		// tell the shader which texture to use
		glUniform1i(diffuseMap, 1);

		//set up the model matrix by multiplying the scene node transform by the meshes global transform
		glm::mat4 modelMatrix = (sceneNode->transform * mesh->m_globalTransform);
		glUniformMatrix4fv(world, 1, false, glm::value_ptr(modelMatrix));
		//glUniform4fv(colour, 1, glm::value_ptr(mesh->m_material->diffuse));
		glUniform4fv(ambient, 1, glm::value_ptr(glm::vec4(0.2f, 0.2f, 0.2f, 1)));
		glUniform4fv(lightDir, 1, glm::value_ptr(currentLightDir));
		
		// bind our vertex array object
		// remember in the initialise function, we bound the VAO and IBO to the VAO
		// so when we bind the VAO, openGL knows what what vertices,
		// indices and vertex attributes to send to the shader
		glBindVertexArray(ro->VAO);
		glDrawElements(GL_TRIANGLES, (unsigned int)mesh->m_indices.size(), GL_UNSIGNED_INT, 0);
	}
	glUseProgram(0);
}

void PhysXTutorials::addWidget(PxShape* shape, PxActor* actor)
{
	PxGeometryType::Enum type = shape->getGeometryType();
	switch (type)
	{
	case PxGeometryType::ePLANE:
		addPlane(shape, actor);
		break;
	case PxGeometryType::eBOX:
		addBox(shape, actor);
		break;
	case PxGeometryType::eSPHERE:
		addSphere(shape,actor);
		break;
	case PxGeometryType::eCAPSULE:
		addCapsule(shape, actor);
		break;
	case PxGeometryType::eHEIGHTFIELD:
		addHeightField(shape, actor);
		break;
	default:
		break;
	}
}

void PhysXTutorials::addSphere(PxShape* pShape, PxActor* actor)
{
	//get the geometry for this PhysX collision volume
	PxSphereGeometry geometry;
	float radius = 0.5f;
	bool status = pShape->getSphereGeometry(geometry);
	if (status)
	{
		radius = geometry.radius;
	}
	//get the transform for this PhysX collision volume
	PxMat44 m(PxShapeExt::getGlobalPose(*pShape));
	glm::mat4 M(m.column0.x, m.column0.y, m.column0.z, m.column0.w,
		m.column1.x, m.column1.y, m.column1.z, m.column1.w,
		m.column2.x, m.column2.y, m.column2.z, m.column2.w,
		m.column3.x, m.column3.y, m.column3.z, m.column3.w);
	glm::vec3 position;
	//get the position out of the transform
	position.x = m.getPosition().x;
	position.y = m.getPosition().y;
	position.z = m.getPosition().z;
	glm::vec4 colour = glm::vec4(0, 0, 1, 0.75);
	//create our sphere gizmo
	Gizmos::addSphere(position, radius, 8, 16, colour, &M);
}

void PhysXTutorials::addBox(PxShape* pShape, PxActor* actor)
{
	//get the geometry for this PhysX collision volume
	PxBoxGeometry geometry;
	float width = 1, height = 1, length = 1;
	bool status = pShape->getBoxGeometry(geometry);
	if (status)
	{
		width = geometry.halfExtents.x;
		height = geometry.halfExtents.y;
		length = geometry.halfExtents.z;
	}
	//get the transform for this PhysX collision volume
	PxMat44 m(PxShapeExt::getGlobalPose(*pShape));
	glm::mat4 M(m.column0.x, m.column0.y, m.column0.z, m.column0.w,
		m.column1.x, m.column1.y, m.column1.z, m.column1.w,
		m.column2.x, m.column2.y, m.column2.z, m.column2.w,
		m.column3.x, m.column3.y, m.column3.z, m.column3.w);
	glm::vec3 position;
	//get the position out of the transform
	position.x = m.getPosition().x;
	position.y = m.getPosition().y;
	position.z = m.getPosition().z;
	glm::vec3 extents = glm::vec3(width, height, length);
	glm::vec4 colour = glm::vec4(1, 0, 0, 0.75);
	//create our box gizmo
	Gizmos::addAABBFilled(position, extents, colour, &M);
}

void PhysXTutorials::addPlane(PxShape* pShape, PxActor* actor)
{
	//get the transform for this PhysX collision volume
	PxMat44 m(PxShapeExt::getGlobalPose(*pShape));
	glm::mat4 M(m.column0.x, m.column0.y, m.column0.z, m.column0.w,
		m.column1.x, m.column1.y, m.column1.z, m.column1.w,
		m.column2.x, m.column2.y, m.column2.z, m.column2.w,
		m.column3.x, m.column3.y, m.column3.z, m.column3.w);
	glm::vec3 position;
	//get the position out of the transform
	position.x = m.getPosition().x;
	position.y = m.getPosition().y;
	position.z = m.getPosition().z;
	//unrotated Gizmo is xz-plane, but unrotated PhysX plane is yz-plane
	M = M * glm::mat4(0, 0, 1, 0,
					  0, 1, 0, 0,
					  1, 0, 0, 0,
					  0, 0, 0, 1);
	//create our grid gizmo
	Gizmos::addGrid(position, 100, 1.0f, glm::vec4(0, 1, 0, 1), &M);
}

void PhysXTutorials::addCapsule(PxShape* pShape, PxActor* actor)
{
	//creates a gizmo representation of a capsule using 2 spheres and a cylinder

	glm::vec4 colour(0, 0, 1, 0.75);  //make our capsule blue
	PxCapsuleGeometry capsuleGeometry;
	float radius = 1; //temporary values whilst we try and get the real value from PhysX
	float halfHeight = 1;;
	//get the geometry for this PhysX collision volume
	bool status = pShape->getCapsuleGeometry(capsuleGeometry);
	if (status)
	{
		//this should always happen but just to be safe we check the status flag
		radius = capsuleGeometry.radius; //copy out capsule radius
		halfHeight = capsuleGeometry.halfHeight; //copy out capsule half length
	}
	//get the world transform for the centre of this PhysX collision volume
	PxTransform transform = PxShapeExt::getGlobalPose(*pShape);
	//use it to create a matrix
	PxMat44 m(transform);
	//convert it to an open gl matrix for adding our gizmos
	glm::mat4 M = Px2Glm(m);
	//get the world position from the PhysX tranform
	glm::vec3 position = Px2GlV3(transform.p);
	glm::vec4 axis(halfHeight, 0, 0, 0);	//axis for the capsule
	axis = M * axis; //rotate axis to correct orientation
	//add our 2 end cap spheres...
	Gizmos::addSphere(position + axis.xyz, radius, 10, 10, colour);
	Gizmos::addSphere(position - axis.xyz, radius, 10, 10, colour);
	//the cylinder gizmo is oriented 90 degrees to what we want so we need to change the rotation matrix...
	glm::mat4 m2 = glm::rotate(M, 11 / 7.0f, glm::vec3(0.0f, 0.0f, 1.0f)); //adds an additional rotation onto the matrix
	//now we can use this matrix and the other data to create the cylinder...
	Gizmos::addCylinderFilled(position, radius, halfHeight, 10, colour, &m2);
}

void PhysXTutorials::addHeightField(PxShape* pShape, PxActor* actor)
{
	glm::vec4 colour(0, 1, 0, 1);

	// get height field information
	PxHeightFieldGeometry heightFieldGeometry;
	bool status = pShape->getHeightFieldGeometry(heightFieldGeometry);
	unsigned int rows = 0;
	unsigned int columns = 0;
	glm::vec3 scale = glm::vec3(0);
	PxHeightFieldSample* samples = nullptr;
	if (status)
	{
		PxHeightField* heightField = heightFieldGeometry.heightField;
		rows = heightField->getNbRows();
		columns = heightField->getNbColumns();
		scale = glm::vec3(heightFieldGeometry.rowScale,
						  heightFieldGeometry.heightScale,
						  heightFieldGeometry.columnScale);
		unsigned int size = sizeof(PxHeightFieldSample)*(rows*columns);
		samples = (PxHeightFieldSample*)_aligned_malloc(size, 16);
		heightField->saveCells((void*)samples, size);
	}

	//get the world transform for the centre of this PhysX collision volume
	PxTransform transform = PxShapeExt::getGlobalPose(*pShape);
	//use it to create a matrix
	PxMat44 m(transform);
	//convert it to an open gl matrix for adding our gizmos
	glm::mat4 M = Px2Glm(m);
	//get the world position from the PhysX tranform
	//glm::vec3 position = Px2GlV3(transform.p);

	// generate points
	typedef std::pair<glm::vec3, glm::vec3> line;
	std::vector<std::pair<glm::vec3, glm::vec3>> lines;
	glm::vec3* points = new glm::vec3[rows*columns];
	bool* flags = new bool[rows*columns];
	for (unsigned int row = 0; row < rows; ++row)
	{
		for (unsigned int col = 0; col < columns; ++col)
		{
			unsigned int index = row*columns + col;
			points[index] = (M * glm::vec4(glm::vec3(row, samples[index].height, col) * scale, 1)).xyz();
			flags[index] = samples[index].tessFlag();
		}
	}
	_aligned_free((void*)samples);

	// draw lines
	for (unsigned int row = 0; row < rows - 1; ++row)
	{
		for (unsigned int col = 0; col < columns - 1; ++col)
		{
			unsigned int topLeft = row*columns + col;
			unsigned int topRight = topLeft + 1;
			unsigned int bottomLeft = topLeft + columns;
			unsigned int bottomRight = bottomLeft + 1;
			if (0 == row)
				Gizmos::addLine(points[topLeft], points[topRight], colour);
			if (0 == col)
				Gizmos::addLine(points[topLeft], points[bottomLeft], colour);
			if (flags[topLeft])
				Gizmos::addLine(points[topLeft], points[bottomRight], colour);
			else
				Gizmos::addLine(points[bottomLeft], points[topRight], colour);
			Gizmos::addLine(points[bottomLeft], points[bottomRight], colour);
			Gizmos::addLine(points[topRight], points[bottomRight], colour);
		}
	}
	delete[] points;
	delete[] flags;
}

void PhysXTutorials::fire()
{
	//add a sphere
	PxSphereGeometry sphere(m_bulletSize);
	PxTransform transform(PxVec3(m_cameraMatrix[3].x, m_cameraMatrix[3].y, m_cameraMatrix[3].z));
	PxRigidDynamic* dynamicActor = PxCreateDynamic(*g_Physics, transform, sphere, *g_PhysicsMaterial, m_bulletDensity);
	dynamicActor->setLinearVelocity(PxVec3(-m_cameraMatrix[2].x * m_bulletSpeed,
										   -m_cameraMatrix[2].y * m_bulletSpeed,
										   -m_cameraMatrix[2].z * m_bulletSpeed));
	//add it to the physX scene
	g_PhysicsScene->addActor(*dynamicActor);
	//add it to our copy of the scene
	g_PhysXActors.push_back(dynamicActor);
}

void PhysXTutorials::reset()
{
	if (nullptr != m_playerActor)
	{
		m_playerActor->setGlobalPose(m_startingPlayerPos);
		m_playerActor->setLinearVelocity(PxVec3(0, 0, 0));
		m_playerActor->setAngularVelocity(PxVec3(0, 0, 0));
	}
	m_cameraMatrix = glm::inverse(glm::lookAt(glm::vec3(-20, 20, -20), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0)));
}

void PhysXTutorials::controlPlayer(float a_deltaTime)
{
	PxTransform pose = m_playerActor->getGlobalPose(); //get the pose from PhysX
	pose.q = PxQuat(11 / 7.0f, PxVec3(0, 0, 1));  //force the actor rotation to vertical
	m_playerActor->setGlobalPose(pose); //reset the actor pose
	//set linear damping on our actor so it slows down when we stop pressing the key
	m_playerActor->setLinearDamping(1);

	if (glfwGetKey(m_window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		glm::vec3 force = glm::vec3(m_cameraMatrix[2].x, 0, m_cameraMatrix[2].z);
		if (0.0f == force.x && 0.0f == force.z)
			force = glm::vec3(m_cameraMatrix[1].x, 0, m_cameraMatrix[1].z);
		force = glm::normalize(force) * -100.0f;
		m_playerActor->addForce(PxVec3(force.x, 0, force.z));
	}
	if (glfwGetKey(m_window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		glm::vec3 force = glm::vec3(m_cameraMatrix[2].x, 0, m_cameraMatrix[2].z);
		if (0.0f == force.x && 0.0f == force.z)
			force = glm::vec3(m_cameraMatrix[1].x, 0, m_cameraMatrix[1].z);
		force = glm::normalize(force) * 100.0f;
		m_playerActor->addForce(PxVec3(force.x, 0, force.z));
	}
	if (glfwGetKey(m_window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		glm::vec3 force = glm::vec3(m_cameraMatrix[0].x, 0, m_cameraMatrix[0].z);
		if (0.0f == force.x && 0.0f == force.z)
		{
			force = glm::vec3(m_cameraMatrix[1].x, 0, m_cameraMatrix[1].z);
			if (0 > glm::dot(m_cameraMatrix[1].xyz(), glm::vec3(0, 1, 0)))
				force *= -1.0f;
		}
		force = glm::normalize(force) * 100.0f;
		m_playerActor->addForce(PxVec3(force.x, 0, force.z));
	}
	if (glfwGetKey(m_window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		glm::vec3 force = glm::vec3(m_cameraMatrix[0].x, 0, m_cameraMatrix[0].z);
		if (0.0f == force.x && 0.0f == force.z)
		{
			force = glm::vec3(m_cameraMatrix[1].x, 0, m_cameraMatrix[1].z);
			if (0 > glm::dot(m_cameraMatrix[1].xyz(), glm::vec3(0, 1, 0)))
				force *= -1.0f;
		}
		force = glm::normalize(force) * -100.0f;
		m_playerActor->addForce(PxVec3(force.x, 0, force.z));
	}
	if (glfwGetKey(m_window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		m_playerActor->addForce(PxVec3(0, 200, 0));
	}
}

PxVec3 glm2Px(const glm::vec3& a_v)
{
	return PxVec3(a_v.x, a_v.y, a_v.z);
}

void PhysXTutorials::pickingExample1()
{
	GLFWwindow* window = glfwGetCurrentContext();
	if (glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_1) == GLFW_PRESS)
	{
		//get the camera position from the camera matrix
		glm::vec3 position(m_cameraMatrix[3]);
		//get the camera rotationfrom the camera matrix
		glm::vec3 direction(glm::normalize(m_cameraMatrix[2]));
		PxVec3 origin = glm2Px(position);                 // [in] Ray origin
		PxVec3 unitDir = glm2Px(-direction);                // [in] Normalized ray direction
		PxReal maxDistance = 1000;            // [in] Raycast max distance
		PxRaycastHit hit;                 // [out] Raycast results

		// Raycast against all static & dynamic objects (no filtering)
		// The main result from this call is the boolean 'status'
		g_PhysicsScene->raycastSingle(origin, unitDir, PX_MAX_F32, PxSceneQueryFlag::eIMPACT, hit, PxSceneQueryFilterData());
		if (hit.shape)
		{
			std::cout << "hit shape: ";
			const char* shapeName = hit.shape->getName();
			if (shapeName)
			{
				//printf("Picked shape name: %s\n", shapeName);
				std::cout << shapeName << std::endl;
			}
			std::cout << std::endl;
		}
		else
		{
			std::cout << "no hits" << std::endl;
		}
	}
}

void PhysXTutorials::tutorial_1()
{
	//add a plane
	PxTransform pose = PxTransform(PxVec3(0.0f, 0, 0.0f), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
	PxRigidStatic* plane = PxCreateStatic(*g_Physics, pose, PxPlaneGeometry(), *g_PhysicsMaterial);
	//add it to the physX scene
	g_PhysicsScene->addActor(*plane);
	//add it to our copy of the scene
	g_PhysXActors.push_back(plane);

	//add a box
	float density = 10;
	PxBoxGeometry box(2, 2, 2);
	PxTransform transform(PxVec3(0, 20, 0));
	PxRigidDynamic* dynamicActor = PxCreateDynamic(*g_Physics, transform, box, *g_PhysicsMaterial, density);
	//add it to the physX scene
	g_PhysicsScene->addActor(*dynamicActor);
	//add it to our copy of the scene
	g_PhysXActors.push_back(dynamicActor);

	//add a sphere
	PxSphereGeometry sphere(2);
	PxTransform transform2(PxVec3(-5, 20, 0));
	PxRigidDynamic* dynamicActor2 = PxCreateDynamic(*g_Physics, transform2, sphere, *g_PhysicsMaterial, density);
	//add it to the physX scene
	g_PhysicsScene->addActor(*dynamicActor2);
	//add it to our copy of the scene
	g_PhysXActors.push_back(dynamicActor2);
}

void PhysXTutorials::tutorial_2()
{
	//add a plane
	PxTransform pose = PxTransform(PxVec3(0.0f, 0, 0.0f), PxQuat(PxHalfPi, PxVec3(0.0f, 0.0f, 1.0f)));
	PxRigidStatic* plane = PxCreateStatic(*g_Physics, pose, PxPlaneGeometry(), *g_PhysicsMaterial);
	//add it to the physX scene
	g_PhysicsScene->addActor(*plane);
	//add it to our copy of the scene
	g_PhysXActors.push_back(plane);
	plane->setName("Ground");
	setupFiltering(plane, FilterGroup::eGROUND, FilterGroup::ePLAYER);// | FilterGroup::ePLATFORM);

	PxCapsuleGeometry capsule(1, 2);
	//set it's initial transform and set it vertical.
	m_startingPlayerPos = PxTransform(PxVec3(0, 10, 0), PxQuat(11 / 7.0f, PxVec3(0, 0, 1)));
	m_playerActor = PxCreateDynamic(*g_Physics, m_startingPlayerPos, capsule, *g_PhysicsMaterial, 1);
	//add it to the physX scene
	g_PhysicsScene->addActor(*m_playerActor);
	g_PhysXActors.push_back(m_playerActor);
	m_playerActor->setName("Player");

	setupFiltering(m_playerActor, FilterGroup::ePLAYER, FilterGroup::eGROUND);// | FilterGroup::ePLATFORM);  //set up the collision filtering for our player

	addPlatforms();
}

void PhysXTutorials::addPlatforms()
{
	for (auto staticObjectPtr : gStaticObject)
	{
		PxBoxGeometry box(staticObjectPtr->extents);
		PxTransform pose = PxTransform(staticObjectPtr->centre);
		PxRigidStatic*staticObject = PxCreateStatic(*g_Physics, pose, box, *g_PhysicsMaterial);
		if (staticObjectPtr->trigger)
		{
			PxShape* objectShape;
			staticObject->getShapes(&objectShape, 1);
			objectShape->setFlag(PxShapeFlag::eSIMULATION_SHAPE, false);
			objectShape->setFlag(PxShapeFlag::eTRIGGER_SHAPE, true);
		}
		//add it to the physX scene
		g_PhysicsScene->addActor(*staticObject);
		//setupFiltering(staticObject, FilterGroup::ePLATFORM, FilterGroup::eGROUND | FilterGroup::ePLAYER);

		g_PhysXActors.push_back(staticObject);
		staticObject->setName(staticObjectPtr->name);

	}
}

void PhysXTutorials::tutorial_3()
{
	PxTransform pose = PxTransform(PxVec3(-375.0f, -5.0f, -375.0f));// , PxQuat(PxHalfPi * 1, PxVec3(0.0f, 0.0f, 1.0f)));
	addStaticHeightMapCollision(pose);
	/*PxRigidStatic* plane = PxCreateStatic(*g_Physics, pose, PxPlaneGeometry(), *g_PhysicsMaterial);
	//add it to the physX scene
	g_PhysicsScene->addActor(*plane);
	plane->setName("Ground");
	//add it to our copy of the scene
	g_PhysXActors.push_back(plane);/**/

	FBXFile* fbxFile = new FBXFile();
	fbxFile->load("./models/tanks/battle_tank.fbx", FBXFile::UNITS_METER);
	fbxFile->initialiseOpenGLTextures();
	InitFBXSceneResource(fbxFile);

	addFBXWithConvexCollision(fbxFile, PxTransform(PxVec3(0, 0, 0)));

	/*
	float density = 10;
	//create our first box for the tank body
	PxBoxGeometry box(1, 0.35, 1.6);
	PxTransform transform(PxVec3(0, 0, 0));
	PxRigidDynamic* dynamicActor = PxCreateDynamic(*g_Physics, transform, box, *g_PhysicsMaterial, density);

	//reposition the first box so it's in the right place
	//find out how many shapes are in the actor
	const PxU32 numShapes = dynamicActor->getNbShapes();
	//reserve space for them
	PxShape** shapes = (PxShape**)_aligned_malloc(sizeof(PxShape*)*numShapes, 16);
	//get them into our buffer
	dynamicActor->getShapes(shapes, numShapes);
	//reset the local transform for the first one (our box)
	shapes[0]->setLocalPose(PxTransform(PxVec3(0, 0.35, -0.125))); //reposition it
	//give it a name
	shapes[0]->setName("Tank hull");

	//create our second box for the turret
	PxBoxGeometry turret(0.8, 0.2, 0.8);
	PxTransform turretPose = PxTransform(PxVec3(0.0f, 0.925f, -0.3f));
	PxShape* actor = dynamicActor->createShape(turret, *g_PhysicsMaterial, turretPose);
	actor->setName("Tank turret");

	PxCapsuleGeometry fuelTank(0.175, 0.5);
	PxTransform fuelTankPose = PxTransform(PxVec3(0.0f, 0.775f, -1.9f));
	actor = dynamicActor->createShape(fuelTank, *g_PhysicsMaterial, fuelTankPose);
	actor->setName("Tank gun barrel");

	PxCapsuleGeometry gunBarrel(0.05, 1.0625);
	PxTransform gunBarrelPose = PxTransform(PxVec3(0.015f, 0.84375f, 1.6125f), PxQuat(PxHalfPi, PxVec3(0, 1, 0)));
	actor = dynamicActor->createShape(gunBarrel, *g_PhysicsMaterial, gunBarrelPose);
	actor->setName("Tank fuel tank");

	//add the actor to the PhysX scene
	g_PhysicsScene->addActor(*dynamicActor);
	//add it to our copy of the scene
	g_PhysXActors.push_back(dynamicActor);
	//create a scene node
	SceneNode* sceneNode = new SceneNode();
	sceneNode->fbxFile = fbxFile; //set up the FBXFile
	sceneNode->physXActor = dynamicActor; //link our scene node to the physics Object
	m_sceneNodes.push_back(sceneNode); //add it to the scene graph
	dynamicActor->userData = (void*)sceneNode;  //Link the dynamic actor to the scene node
	/**/
}
