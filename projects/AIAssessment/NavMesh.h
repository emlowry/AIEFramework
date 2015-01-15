#pragma once

#include <vector>
#include <unordered_map>
#include <glm/glm.hpp>
#include "Rectangle.h"

struct NavMeshTile
{
	static const glm::vec2 SIZE;

	Rectangle rect;
	NavMeshTile*	neighbors[4];	// neighbor[Rectangle::RIGHT] shares right edge, etc.

	NavMeshTile(const glm::vec2& a_center);
	NavMeshTile(const float a_centerX, const float a_centerY);
};

// nodes used for A* pathfinding
struct PathNode
{
	typedef std::unordered_map<NavMeshTile*, PathNode> Lookup;

	NavMeshTile* tile;
	PathNode* previous;
	glm::vec2 position;
	float dist2toEnd;

	PathNode() {}
	PathNode(NavMeshTile* a_tile, const glm::vec2& a_end);
	PathNode(NavMeshTile* a_tile, const glm::vec2& a_end, const glm::vec2& a_position);

	float stepCost() const;
	float pathCost() const;
	float pathCostFrom(const PathNode* const a_node) const;
};
typedef std::vector<glm::vec2> Path;

class NavMesh : public std::vector<NavMeshTile*>
{
public:

	NavMesh() {}
	virtual ~NavMesh() {}

	void addGizmos() const;
	void deleteTiles();
	void linkTiles();
	bool calculatePath(const glm::vec2& a_start,
					   const glm::vec2& a_end,
					   Path& a_path) const;
	NavMeshTile* getTile(const glm::vec2& a_point) const;
	bool lineOfSight(const glm::vec2& a_start,
					 const glm::vec2& a_end,
					 NavMeshTile* a_startTile = nullptr,
					 NavMeshTile* a_endTile = nullptr) const;

protected:

	bool aStar(PathNode* a_start, PathNode* a_end,
			   PathNode::Lookup& a_lookup) const;
	void smoothPath(PathNode* a_start, PathNode* a_end) const;
};
