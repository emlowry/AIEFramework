#include "PhysXTutorials.h"
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
	m_cameraMatrix = glm::inverse( glm::lookAt(glm::vec3(-20,20,-20),glm::vec3(0,0,0), glm::vec3(0,1,0)) );

	// get window dimensions to calculate aspect ratio
	int width = 0, height = 0;
	glfwGetWindowSize(m_window, &width, &height);

	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, width / (float)height, 0.1f, 1000.0f);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	// Set up PhysX
	setUpPhysX();
	tutorial_2();
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

	// add a 20x20 grid on the XZ-plane
	Gizmos::addGrid();

	controlPlayer(a_deltaTime);

	// fire bullet
	if (glfwGetKey(m_window, GLFW_KEY_ENTER) == GLFW_PRESS)
	{
		float time = Utility::getTotalTime();
		if (time - m_lastFireTime >= m_fireInterval)
		{
			fire();
			m_lastFireTime = time;
		}
	}

	// update PhysX
	updatePhysX();

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
}

void PhysXTutorials::onDestroy()
{
	// clean up anything we created
	Gizmos::destroy();
	cleanUpPhysX();
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
	glm::vec4 colour = glm::vec4(1, 0, 0, 1);
	//create our sphere gizmo
	Gizmos::addSphere(position, radius, 8, 16, glm::vec4(0,0,1,1), &M);
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
	glm::vec4 colour = glm::vec4(1, 0, 0, 1);
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
	M = M * glm::mat4(0, 1, 0, 0,
					  1, 0, 0, 0,
					  0, 0, 1, 0,
					  0, 0, 0, 1);
	//create our grid gizmo
	Gizmos::addGrid(position, 100, 1.0f, glm::vec4(0, 1, 0, 1), &M);
}

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

void PhysXTutorials::addCapsule(PxShape* pShape, PxActor* actor)
{
	//creates a gizmo representation of a capsule using 2 spheres and a cylinder

	glm::vec4 colour(0, 0, 1, 1);  //make our capsule blue
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
