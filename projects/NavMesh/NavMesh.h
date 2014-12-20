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
		NavNodeTri(const glm::vec3& a_v0,
				   const glm::vec3& a_v1,
				   const glm::vec3& a_v2);
		NavNodeTri(const glm::vec3 a_vertices[3]);
		void setup();
		glm::vec3 toParametric(const glm::vec3& a_world) const;
		glm::vec3 toParametric(float a_worldX,
							   float a_worldY,
							   float a_worldZ = 0) const;
		glm::vec3 toWorld(const glm::vec3& a_parametric) const;
		glm::vec3 toWorld(float a_parametricX,
						  float a_parametricY,
						  float a_parametricZ = 0) const;
		glm::vec3 farthestPointAlongPath(const glm::vec3& a_start,
										 const glm::vec3& a_end) const;
		
		glm::vec3	position;
		glm::vec3	vertices[3];
		glm::vec3	normal;
		NavNodeTri*	edgeTarget[3];

		glm::mat4	parametricToWorld;
		glm::mat4	worldToParametric;
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
		bool lineOfSight(const glm::vec3& a_target) const;
		
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