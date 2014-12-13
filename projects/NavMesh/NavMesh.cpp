#include "NavMesh.h"
#include "Gizmos.h"
#include "Utilities.h"
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/ext.hpp>
#include <unordered_map>

#define DEFAULT_SCREENWIDTH 1280
#define DEFAULT_SCREENHEIGHT 720

NavMesh::NavMesh()
{

}

NavMesh::~NavMesh()
{

}

bool NavMesh::onCreate(int a_argc, char* a_argv[]) 
{
	// initialise the Gizmos helper class
	Gizmos::create();

	// create a world-space matrix for a camera
	m_cameraMatrix = glm::inverse( glm::lookAt(glm::vec3(10,10,0),glm::vec3(0,0,0), glm::vec3(0,1,0)) );
	
	// create a perspective projection matrix with a 90 degree field-of-view and widescreen aspect ratio
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f, DEFAULT_SCREENWIDTH/(float)DEFAULT_SCREENHEIGHT, 0.1f, 1000.0f);

	// set the clear colour and enable depth testing and backface culling
	glClearColor(0.25f,0.25f,0.25f,1);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);

	m_sponza = new FBXFile();
	m_sponza->load("models/SponzaSimple.fbx", FBXFile::UNITS_CENTIMETER);
	createOpenGLBuffers(m_sponza);

	m_navMesh = new FBXFile();
	m_navMesh->load("models/SponzaSimpleNavMesh.fbx", FBXFile::UNITS_CENTIMETER);
//	createOpenGLBuffers(m_navMesh);

	buildNavMesh(m_navMesh->getMeshByIndex(0), m_graph);

	unsigned int vs = Utility::loadShader("shaders/sponza.vert", GL_VERTEX_SHADER);
	unsigned int fs = Utility::loadShader("shaders/sponza.frag", GL_FRAGMENT_SHADER);
	m_shader = Utility::createProgram(vs,0,0,0,fs);
	glDeleteShader(vs);
	glDeleteShader(fs);

	return true;
}

void NavMesh::onUpdate(float a_deltaTime) 
{
	// update our camera matrix using the keyboard/mouse
	Utility::freeMovement( m_cameraMatrix, a_deltaTime, 10 );

	// clear all gizmos from last frame
	Gizmos::clear();
	
	// add an identity matrix gizmo
	Gizmos::addTransform( glm::mat4(1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1) );

	// add a 20x20 grid on the Z-plane
	for ( int i = 0 ; i < 21 ; ++i )
	{
		Gizmos::addLine( glm::vec3(-10 + i, 0, 10), glm::vec3(-10 + i, 0, -10), 
						 i == 10 ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1) );
		
		Gizmos::addLine( glm::vec3(10, 0, -10 + i), glm::vec3(-10, 0, -10 + i), 
						 i == 10 ? glm::vec4(1,1,1,1) : glm::vec4(0,0,0,1) );
	}

	// draw navmesh
	for (auto node : m_graph)
	{
		// draw node center
		Gizmos::addAABBFilled(node->position + glm::vec3(0, 0.05f, 0),
							  glm::vec3(0.05f), glm::vec4(1, 0, 0, 1));

		// draw node area
		Gizmos::addTri(node->vertices[0] + glm::vec3(0, 0.05f, 0),
					   node->vertices[1] + glm::vec3(0, 0.05f, 0),
					   node->vertices[2] + glm::vec3(0, 0.05f, 0),
					   glm::vec4(0, 1, 0, 0.25f));
		Gizmos::addLine(node->vertices[0] + glm::vec3(0, 0.05f, 0),
						node->vertices[1] + glm::vec3(0, 0.05f, 0),
						glm::vec4(0, 1, 0, 1));
		Gizmos::addLine(node->vertices[1] + glm::vec3(0, 0.05f, 0),
						node->vertices[2] + glm::vec3(0, 0.05f, 0),
						glm::vec4(0, 1, 0, 1));
		Gizmos::addLine(node->vertices[2] + glm::vec3(0, 0.05f, 0),
						node->vertices[0] + glm::vec3(0, 0.05f, 0),
						glm::vec4(0, 1, 0, 1));

		// draw connections to adjacent nodes
		if (nullptr != node->edgeTarget[0])
		{
			Gizmos::addLine(node->position + glm::vec3(0, 0.1f, 0),
							node->edgeTarget[0]->position + glm::vec3(0, 0.1f, 0),
							glm::vec4(1, 1, 0, 1));
		}
		if (nullptr != node->edgeTarget[1])
		{
			Gizmos::addLine(node->position + glm::vec3(0, 0.1f, 0),
				node->edgeTarget[1]->position + glm::vec3(0, 0.1f, 0),
				glm::vec4(1, 1, 0, 1));
		}
		if (nullptr != node->edgeTarget[2])
		{
			Gizmos::addLine(node->position + glm::vec3(0, 0.1f, 0),
				node->edgeTarget[2]->position + glm::vec3(0, 0.1f, 0),
				glm::vec4(1, 1, 0, 1));
		}
	}

	// quit our application when escape is pressed
	if (glfwGetKey(m_window,GLFW_KEY_ESCAPE) == GLFW_PRESS)
		quit();
}

void NavMesh::onDraw() 
{
	// clear the backbuffer
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	// get the view matrix from the world-space camera matrix
	glm::mat4 viewMatrix = glm::inverse( m_cameraMatrix );

	glUseProgram(m_shader);

	int location = glGetUniformLocation(m_shader, "projectionView");
	glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr( m_projectionMatrix * viewMatrix ));

	unsigned int count = m_sponza->getMeshCount();
	for (unsigned int i = 0 ; i < count ; ++i )
	{
		FBXMeshNode* mesh = m_sponza->getMeshByIndex(i);

		GLData* data = (GLData*)mesh->m_userData;

		location = glGetUniformLocation(m_shader, "model");
		glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr( mesh->m_globalTransform ));

		location = glGetUniformLocation(m_shader, "invTransposeModel");
		glUniformMatrix4fv(location, 1, GL_TRUE, glm::value_ptr( glm::inverse( mesh->m_globalTransform ) ));

		glBindVertexArray(data->vao);
		glDrawElements(GL_TRIANGLES, mesh->m_indices.size(), GL_UNSIGNED_INT, nullptr);
	}
	
	// draw the gizmos from this frame
	Gizmos::draw(m_projectionMatrix, viewMatrix);
}

void NavMesh::onDestroy()
{
	cleanupOpenGLBuffers(m_sponza);
	//cleanupOpenGLBuffers(m_navMesh);

	delete m_navMesh;
	delete m_sponza;

	glDeleteProgram(m_shader);

	// clean up anything we created
	Gizmos::destroy();
}

// main that controls the creation/destruction of an application
int main(int argc, char* argv[])
{
	// explicitly control the creation of our application
	Application* app = new NavMesh();
	
	if (app->create("AIE - NavMesh",DEFAULT_SCREENWIDTH,DEFAULT_SCREENHEIGHT,argc,argv) == true)
		app->run();
		
	// explicitly control the destruction of our application
	delete app;

	return 0;
}

void NavMesh::createOpenGLBuffers(FBXFile* a_fbx)
{
	// create the GL VAO/VBO/IBO data for meshes
	for ( unsigned int i = 0 ; i < a_fbx->getMeshCount() ; ++i )
	{
		FBXMeshNode* mesh = a_fbx->getMeshByIndex(i);

		// storage for the opengl data in 3 unsigned int
		GLData* glData = new GLData();

		glGenVertexArrays(1, &glData->vao);
		glBindVertexArray(glData->vao);

		glGenBuffers(1, &glData->vbo);
		glGenBuffers(1, &glData->ibo);

		glBindBuffer(GL_ARRAY_BUFFER, glData->vbo);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, glData->ibo);

		glBufferData(GL_ARRAY_BUFFER, mesh->m_vertices.size() * sizeof(FBXVertex), mesh->m_vertices.data(), GL_STATIC_DRAW);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, mesh->m_indices.size() * sizeof(unsigned int), mesh->m_indices.data(), GL_STATIC_DRAW);

		glEnableVertexAttribArray(0); // position
		glEnableVertexAttribArray(1); // normal
		glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char*)FBXVertex::PositionOffset );
		glVertexAttribPointer(1, 4, GL_FLOAT, GL_FALSE, sizeof(FBXVertex), (char*)FBXVertex::NormalOffset );

		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

		mesh->m_userData = glData;
	}
}

void NavMesh::cleanupOpenGLBuffers(FBXFile* a_fbx)
{
	// bind our vertex array object and draw the mesh
	for ( unsigned int i = 0 ; i < a_fbx->getMeshCount() ; ++i )
	{
		FBXMeshNode* mesh = a_fbx->getMeshByIndex(i);

		GLData* glData = (GLData*)mesh->m_userData;

		glDeleteVertexArrays(1, &glData->vao);
		glDeleteBuffers(1, &glData->vbo);
		glDeleteBuffers(1, &glData->ibo);

		delete[] glData;
	}
}

void NavMesh::buildNavMesh(FBXMeshNode* a_mesh, std::vector<NavNodeTri*>& a_graph)
{
	unsigned int triCount = a_mesh->m_indices.size() / 3;

	// create nodes
	for (unsigned int tri = 0; tri < triCount; ++tri)
	{
		NavNodeTri*node = new NavNodeTri();

		// edge [ABC]
		// [AB] = 0, [BC] = 1, [CA] = 2

		node->edgeTarget[0] = nullptr;
		node->edgeTarget[1] = nullptr;
		node->edgeTarget[2] = nullptr;

		node->vertices[0] = a_mesh->m_vertices[a_mesh->m_indices[tri * 3 + 0]].position.xyz;
		node->vertices[1] = a_mesh->m_vertices[a_mesh->m_indices[tri * 3 + 1]].position.xyz;
		node->vertices[2] = a_mesh->m_vertices[a_mesh->m_indices[tri * 3 + 2]].position.xyz;

		node->position = (node->vertices[0] + node->vertices[1] + node->vertices[2]) / 3.0f;

		a_graph.push_back(node);
	}

	// connect nodes
	for (auto start : a_graph)
	{
		for (auto end : a_graph)
		{
			if (start == end)
				continue;

			// ABC XYZ
			
			// AB == XY || AB == YZ || AB == ZX
			// AB == YX || AB == ZY || AB == XZ
			if ((start->vertices[0] == end->vertices[0] && start->vertices[1] == end->vertices[1]) ||
				(start->vertices[0] == end->vertices[1] && start->vertices[1] == end->vertices[2]) ||
				(start->vertices[0] == end->vertices[2] && start->vertices[1] == end->vertices[0]) ||
				(start->vertices[0] == end->vertices[1] && start->vertices[1] == end->vertices[0]) ||
				(start->vertices[0] == end->vertices[2] && start->vertices[1] == end->vertices[1]) ||
				(start->vertices[0] == end->vertices[0] && start->vertices[1] == end->vertices[2]))
			{
				start->edgeTarget[0] = end;
			}

			// BC == XY || BC == YZ || BC == ZX
			// BC == YX || BC == ZY || BC == XZ
			if ((start->vertices[1] == end->vertices[0] && start->vertices[2] == end->vertices[1]) ||
				(start->vertices[1] == end->vertices[1] && start->vertices[2] == end->vertices[2]) ||
				(start->vertices[1] == end->vertices[2] && start->vertices[2] == end->vertices[0]) ||
				(start->vertices[1] == end->vertices[1] && start->vertices[2] == end->vertices[0]) ||
				(start->vertices[1] == end->vertices[2] && start->vertices[2] == end->vertices[1]) ||
				(start->vertices[1] == end->vertices[0] && start->vertices[2] == end->vertices[2]))
			{
				start->edgeTarget[1] = end;
			}

			// CA == XY || CA == YZ || CA == ZX
			// CA == YX || CA == ZY || CA == XZ
			if ((start->vertices[2] == end->vertices[0] && start->vertices[0] == end->vertices[1]) ||
				(start->vertices[2] == end->vertices[1] && start->vertices[0] == end->vertices[2]) ||
				(start->vertices[2] == end->vertices[2] && start->vertices[0] == end->vertices[0]) ||
				(start->vertices[2] == end->vertices[1] && start->vertices[0] == end->vertices[0]) ||
				(start->vertices[2] == end->vertices[2] && start->vertices[0] == end->vertices[1]) ||
				(start->vertices[2] == end->vertices[0] && start->vertices[0] == end->vertices[2]))
			{
				start->edgeTarget[2] = end;
			}
		}
	}
}

bool NavMesh::findPath(NavNodeTri* a_start, NavNodeTri* a_end,
	const std::vector<NavNodeTri*>& a_graph,
	std::vector<NavNodeTri*>& a_path)
{
	// no path needed or possible
	if (nullptr == a_start || nullptr == a_end)
		return false;
	if (a_start == a_end)
		return true;

	// create path nodes and parameters for A* iterations
	std::unordered_map<NavNodeTri*, PathNode> nodes;
	std::set<PathNode*> open, closed;
	PathNode* end = nullptr;
	for (auto node : a_graph)
	{
		nodes[node] = PathNode(node);
		if (node == a_start)
		{
			open.insert(&nodes[node]);
		}
		if (node == a_end)
		{
			end = &nodes[node];
		}
	}

	// iterate
	while (computingPathStep(end, open, closed, nodes));

	// no path found
	if (nullptr == end->previous)
	{
		return false;
	}

	// return result
	for (PathNode* current = end; nullptr != current; current = current->previous)
	{
		a_path.insert(a_path.begin(), current);
	}
	return true;
}

bool NavMesh::computingPathStep(PathNode* a_end,
								std::set<PathNode*>& a_open,
								std::set<PathNode*>& a_closed,
								std::unordered_map<NavNodeTri*, PathNode>& a_nodes)
{
	// no more to do if no open nodes
	if (a_open.empty())
	{
		return false;
	}

	// select optimum open path node
	PathNode* current = *a_open.begin();
	for (auto node : a_open)
	{
		// if end node is one of the open nodes, we're done
		if (a_end == node)
		{
			return false;
		}

		// otherwise, the optimum open node is the one closest to the end
		if (current != node)
		{
			glm::vec3 currentDisplacement = a_end->node->position - current->node->position;
			glm::vec3 newDisplacement = a_end->node->position - node->node->position;
			if (glm::dot(newDisplacement, newDisplacement) <
				glm::dot(currentDisplacement, currentDisplacement))
			{
				current = node;
			}
		}
	}
	a_open.erase(current);
	a_closed.insert(current);

	// check neighbors
	for (auto edgeTarget : current->node->edgeTarget)
	{
		if (nullptr == edgeTarget)
		{
			continue;
		}
		PathNode* neighbor = &a_nodes[edgeTarget];
		
		// if neighbor has already been checked, no need to check again
		if (a_closed.end() == a_closed.find(neighbor))
		{
			continue;
		}

		// if neighbor can be reached more quickly via current node, set previous
		float cost = glm::distance(neighbor->node->position, current->node->position);
		if (nullptr == neighbor->previous ||
			neighbor->pathCostFrom(current) < neighbor->pathCost())
		{
			neighbor->previous = current;
		}

		// if neighbor is end node, we're done
		if (a_end == neighbor)
		{
			return false;
		}

		// neighbor can be checked in later iterations
		a_open.insert(neighbor);
	}

	return true;
}