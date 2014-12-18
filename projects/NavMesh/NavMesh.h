#ifndef __NavMesh_H_
#define __NavMesh_H_

#include "Application.h"
#include <glm/glm.hpp>

#include <FBXFile.h>
#include <set>
#include <unordered_map>

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

		glm::vec3 closestPointTo(const glm::vec3& a_target) const;
		void closestPointTo(const glm::vec3& a_target, glm::vec3& a_point) const;
		bool intersections(const glm::vec3& a_start, const glm::vec3& a_end,
						   glm::vec3* a_entrance = nullptr,
						   glm::vec3* a_exit = nullptr);
		bool intersections(const glm::vec3& a_start, const glm::vec3& a_end,
						   NavNodeTri*& a_previous, NavNodeTri*& a_next,
						   glm::vec3* a_entrance = nullptr,
						   glm::vec3* a_exit = nullptr);
	};

	struct PathNode
	{
		PathNode(NavNodeTri* a_node = nullptr,
				 PathNode* a_previous = nullptr);
		PathNode(const glm::vec3& a_position,
				 NavNodeTri* a_node = nullptr,
				 PathNode* a_previous = nullptr);

		float pathCost() const;
		float costFrom(PathNode* a_node) const;
		float pathCostFrom(PathNode* a_node) const;
		
		NavNodeTri* node = nullptr;
		PathNode* previous = nullptr;
		glm::vec3 position;
	};

	std::vector<NavNodeTri*>	m_graph;

	std::vector<PathNode>	m_path;

	void	buildNavMesh(FBXMeshNode* a_mesh,
						 std::vector<NavNodeTri*>& a_graph);
	bool	findPath(NavNodeTri* a_start,
					 NavNodeTri* a_end,
					 const std::vector<NavNodeTri*>& a_graph,
					 std::vector<PathNode>& a_path);
	bool	computingPathStep(PathNode* a_endNode,
							  std::set<PathNode*>& a_open,
							  std::set<PathNode*>& a_closed,
							  std::unordered_map<NavNodeTri*, PathNode>& a_nodes);
	void	smoothPath(std::vector<PathNode>& a_path);

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