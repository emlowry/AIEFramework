#ifndef __NavMesh_H_
#define __NavMesh_H_

#include "Application.h"
#include <glm/glm.hpp>

#include <FBXFile.h>
#include <set>

// Derived application class that wraps up all globals neatly
class NavMesh : public Application
{
public:

	NavMesh();
	virtual ~NavMesh();

protected:

	virtual bool onCreate(int a_argc, char* a_argv[]);
	virtual void onUpdate(float a_deltaTime);
	virtual void onDraw();
	virtual void onDestroy();

	struct NavNodeTri
	{
		glm::vec3	position;
		glm::vec3	vertices[3];
		NavNodeTri*	edgeTarget[3];
	};

	struct PathNode
	{
		PathNode(NavNodeTri* a_node = nullptr,
				 PathNode* a_previous = nullptr)
			: node(a_node), previous(a_previous) {}

		float pathCost() const
		{
			float totalCost = 0.0f;
			for (const PathNode* current = this;
				 nullptr != current->previous;
				 current = current->previous)
			{
				totalCost += costFrom(previous);
			}
		}

		float costFrom(PathNode* a_node) const
		{
			if (nullptr == a_node || nullptr == a_node->node || nullptr == node)
			{
				return 0;
			}
			return glm::distance(node->position, a_node->node->position);
		}

		float pathCostFrom(PathNode* a_node) const
		{
			if (nullptr == a_node)
			{
				return 0;
			}
			return costFrom(a_node) + a_node->pathCost();
		}
		
		NavNodeTri* node = nullptr;
		PathNode* previous = nullptr;
	};

	std::vector<NavNodeTri*>	m_graph;

	void	buildNavMesh(FBXMeshNode* a_mesh,
						 std::vector<NavNodeTri*>& a_graph);
	bool	findPath(NavNodeTri* a_start,
					 NavNodeTri* a_end,
					 const std::vector<NavNodeTri*>& a_graph,
					 std::vector<NavNodeTri*>& a_path);
	bool	computingPathStep(PathNode* a_endNode,
							  std::set<PathNode*>& a_open,
							  std::set<PathNode*>& a_closed,
							  std::unordered_map<NavNodeTri*, PathNode>& a_nodes);

	void	createOpenGLBuffers(FBXFile* a_fbx);
	void	cleanupOpenGLBuffers(FBXFile* a_fbx);

	struct GLData
	{
		unsigned int	vao, vbo, ibo;
	};

	FBXFile*	m_sponza;
	FBXFile*	m_navMesh;

	unsigned int	m_shader;

	glm::mat4	m_cameraMatrix;
	glm::mat4	m_projectionMatrix;
};

#endif // __NavMesh_H_