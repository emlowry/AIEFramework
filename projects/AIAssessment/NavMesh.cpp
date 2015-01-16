#include "NavMesh.h"
#include "Gizmos.h"
#include <unordered_map>
#include <set>

//
// NavMeshTile
//

const glm::vec2 NavMeshTile::SIZE(1.0f);

NavMeshTile::NavMeshTile(const glm::vec2& a_center) : rect(a_center, SIZE)
{
	neighbors[0] = neighbors[1] = neighbors[2] = neighbors[3] = nullptr;
}

NavMeshTile::NavMeshTile(const float a_centerX, const float a_centerY)
: rect(glm::vec2(a_centerX, a_centerY), SIZE)
{
	neighbors[0] = neighbors[1] = neighbors[2] = neighbors[3] = nullptr;
}

//
// PathNode
//

PathNode::PathNode(NavMeshTile* a_tile, const glm::vec2& a_end)
	: tile(a_tile), previous(nullptr),
position(nullptr == a_tile ? glm::vec2(0) : a_tile->rect.center())
{
	glm::vec2 displacement = a_end - position;
	dist2toEnd = glm::dot(displacement, displacement);
}

PathNode::PathNode(NavMeshTile* a_tile, const glm::vec2& a_end, const glm::vec2& a_position)
	: tile(a_tile), previous(nullptr), position(a_position)
{
	glm::vec2 displacement = a_end - position;
	dist2toEnd = glm::dot(displacement, displacement);
}

float PathNode::stepCost() const
{
	return nullptr == previous ? 0 : glm::distance(position, previous->position);
}

float PathNode::pathCost() const
{
	return stepCost() + (nullptr == previous ? 0 : previous->pathCost());
}

float PathNode::pathCostFrom(const PathNode* const a_node) const
{
	return nullptr == a_node ? 0 : (glm::distance(position, a_node->position) + a_node->pathCost());
}

//
// NavMesh
//

void NavMesh::addGizmos() const
{
	for each (auto tile in *this)
	{
		if (nullptr == tile) continue;
		Gizmos::addTri(glm::vec3(tile->rect.bottomLeft, 0.1f),
					   glm::vec3(tile->rect.bottomRight(), 0.1f),
					   glm::vec3(tile->rect.topRight, 0.1f),
					   glm::vec4(0, 1, 0, 0.25f));
		Gizmos::addTri(glm::vec3(tile->rect.topRight, 0.1f),
					   glm::vec3(tile->rect.topLeft(), 0.1f),
					   glm::vec3(tile->rect.bottomLeft, 0.1f),
					   glm::vec4(0, 1, 0, 0.25f));
		Gizmos::addLine(glm::vec3(tile->rect.topRight, 0.1f),
						glm::vec3(tile->rect.bottomRight(), 0.1f),
						glm::vec4((nullptr == tile->neighbors[Rectangle::RIGHT] ? 1 : 0), 1, 0, 1));
		Gizmos::addLine(glm::vec3(tile->rect.bottomRight(), 0.1f),
						glm::vec3(tile->rect.bottomLeft, 0.1f),
						glm::vec4((nullptr == tile->neighbors[Rectangle::BOTTOM] ? 1 : 0), 1, 0, 1));
		Gizmos::addLine(glm::vec3(tile->rect.bottomLeft, 0.1f),
						glm::vec3(tile->rect.topLeft(), 0.1f),
						glm::vec4((nullptr == tile->neighbors[Rectangle::LEFT] ? 1 : 0), 1, 0, 1));
		Gizmos::addLine(glm::vec3(tile->rect.topLeft(), 0.1f),
						glm::vec3(tile->rect.topRight, 0.1f),
						glm::vec4((nullptr == tile->neighbors[Rectangle::TOP] ? 1 : 0), 1, 0, 1));
	}
}

void NavMesh::deleteTiles()
{
	while (!empty())
	{
		auto tile = back();
		pop_back();
		if (nullptr != tile) delete tile;
	}
}

void NavMesh::linkTiles()
{
	// remove existing links
	for each(auto tile in *this)
	{
		if (nullptr != tile)
		{
			tile->neighbors[0] = tile->neighbors[1] = tile->neighbors[2] = tile->neighbors[3] = nullptr;
		}
	}

	// link tiles
	for (unsigned int i = 0; i < size(); ++i)
	{
		if (nullptr == at(i)) continue;

		for (unsigned int j = i + 1; j <size(); ++j)
		{
			if (nullptr == at(j)) continue;

			// get shared edges
			Rectangle::SharedEdgeFlags sharedEdges = at(i)->rect.sharedEdges(at(j)->rect);

			// set second tile as neighbor of first
			if (sharedEdges & Rectangle::RIGHT_SHARED)
				at(i)->neighbors[Rectangle::RIGHT] = at(j);
			else if (sharedEdges & Rectangle::BOTTOM_SHARED)
				at(i)->neighbors[Rectangle::BOTTOM] = at(j);
			else if (sharedEdges & Rectangle::LEFT_SHARED)
				at(i)->neighbors[Rectangle::LEFT] = at(j);
			else if (sharedEdges & Rectangle::TOP_SHARED)
				at(i)->neighbors[Rectangle::TOP] = at(j);

			// set first tile as neighbor of second
			sharedEdges = (Rectangle::SharedEdgeFlags)(sharedEdges >> 4);
			if (sharedEdges & Rectangle::RIGHT_SHARED)
				at(j)->neighbors[Rectangle::RIGHT] = at(i);
			else if (sharedEdges & Rectangle::BOTTOM_SHARED)
				at(j)->neighbors[Rectangle::BOTTOM] = at(i);
			else if (sharedEdges & Rectangle::LEFT_SHARED)
				at(j)->neighbors[Rectangle::LEFT] = at(i);
			else if (sharedEdges & Rectangle::TOP_SHARED)
				at(j)->neighbors[Rectangle::TOP] = at(i);
		}
	}
}

bool NavMesh::calculatePath(const glm::vec2& a_start,
							const glm::vec2& a_end,
							Path& a_path) const
{
	// make sure path is empty
	a_path.clear();

	// trivial cases
	if (a_start == a_end)
	{
		a_path.push_back(a_end);
		return true;
	}
	if (lineOfSight(a_start, a_end))
	{
		a_path.push_back(a_start);
		a_path.push_back(a_end);
		return true;
	}

	// create initial pathfinding node
	PathNode::Lookup pathNodes;
	NavMeshTile* startTile = getTile(a_start);
	if (nullptr == startTile) return false;
	pathNodes[startTile] = PathNode(startTile, a_end, a_start);
	PathNode* start = &pathNodes[startTile];

	// create final pathfinding node
	NavMeshTile* endTile = getTile(a_end);
	pathNodes[endTile] = PathNode(endTile, a_end, a_end);
	if (nullptr == endTile) return false;
	PathNode* end = &pathNodes[endTile];

	// A* pathfinding - stop if unsuccessful
	if (!aStar(start, end, pathNodes)) return false;

	// otherwise, smooth path
	smoothPath(start, end);

	// convert path nodes into simple vector of points
	for (PathNode* current = end; nullptr != current; current = current->previous)
		a_path.insert(a_path.begin(), current->position);

	// indicate success
	return true;
}

NavMeshTile* NavMesh::getTile(const glm::vec2& a_point) const
{
	for each (auto tile in *this)
	{
		if (nullptr != tile && tile->rect.contains(a_point))
			return tile;
	}
	return nullptr;
}

bool NavMesh::lineOfSight(const glm::vec2& a_start,
						  const glm::vec2& a_end,
						  NavMeshTile* a_startTile,
						  NavMeshTile* a_endTile) const
{
	// trivial case
	if (a_start == a_end)
		return true;

	// if start and end tiles aren't provided, find them
	if (nullptr == a_startTile)
		a_startTile = getTile(a_start);
	if (nullptr == a_endTile)
		a_endTile = getTile(a_end);

	// if navmesh has no tile for start and/or end point, no line-of-sight
	if (nullptr == a_startTile || nullptr == a_endTile)
		return false;

	// if start and end tiles are the same or neighbors, line-of-sight
	if (a_startTile == a_endTile ||
		a_startTile == a_endTile->neighbors[0] ||
		a_startTile == a_endTile->neighbors[1] ||
		a_startTile == a_endTile->neighbors[2] ||
		a_startTile == a_endTile->neighbors[3])
	{
		return true;
	}

	// check that sight line only intersects linked tiles
	NavMeshTile* current = a_startTile;
	float deltaX = a_end.x - a_start.x;
	float deltaY = a_end.y - a_start.y;
	LineSegment sightLine(a_start, a_end);
	while (nullptr != current && a_endTile != current)
	{
		if (deltaX > 0 && sightLine.intersects(current->rect.edge(Rectangle::RIGHT)))
			current = current->neighbors[Rectangle::RIGHT];
		else if (deltaY < 0 && sightLine.intersects(current->rect.edge(Rectangle::BOTTOM)))
			current = current->neighbors[Rectangle::BOTTOM];
		else if (deltaX < 0 && sightLine.intersects(current->rect.edge(Rectangle::LEFT)))
			current = current->neighbors[Rectangle::LEFT];
		else if (deltaY > 0 && sightLine.intersects(current->rect.edge(Rectangle::TOP)))
			current = current->neighbors[Rectangle::TOP];
		else
			break;
	}
	return (a_endTile == current);
}

bool NavMesh::aStar(PathNode* a_start, PathNode* a_end,
					PathNode::Lookup& a_lookup) const
{
	// set up A*
	std::set<PathNode*> open;
	open.insert(a_start);
	std::set<NavMeshTile*> closed;

	// iterate while open set isn't empty and doesn't contain end node
	while (!open.empty() && 0 == open.count(a_end))
	{
		// remove closest node to end from open set
		PathNode* current = nullptr;
		for each (auto node in open)
		{
			if (nullptr == current || node->dist2toEnd < current->dist2toEnd)
				current = node;
		}
		open.erase(current);
		closed.insert(current->tile);

		// check neighbors
		for each (auto neighborTile in current->tile->neighbors)
		{
			if (nullptr == neighborTile) continue;

			// skip neighbors that have already been checked
			if (0 < closed.count(neighborTile)) continue;

			// get path node for neighbor
			if (0 == a_lookup.count(neighborTile))
				a_lookup[neighborTile] = PathNode(neighborTile, a_end->position);
			PathNode* neighbor = &a_lookup[neighborTile];

			// if this node makes for a better previous node on the path to the
			// neighbor, update
			if (nullptr == neighbor->previous ||
				(current != neighbor->previous &&
				neighbor->pathCostFrom(current) < neighbor->pathCost()))
			{
				neighbor->previous = current;
			}

			// add neighbor to open set
			open.insert(neighbor);
		}
	}

	// return true if path reached end
	return (nullptr != a_end->previous);
}

void NavMesh::smoothPath(PathNode* a_start, PathNode* a_end) const
{
	if (nullptr != a_end->previous->previous)
	{
		// make a list of nodes to check for smoothing to later nodes
		std::vector<PathNode*> toCheck;
		for (PathNode* current = a_end->previous->previous; nullptr != current; current = current->previous)
			toCheck.insert(toCheck.begin(), current);

		// while there are still nodes to check,
		for (PathNode* current = a_end; nullptr != current->previous && !toCheck.empty(); current = current->previous)
		{
			// get the number of nodes in the list of nodes to check
			unsigned int end = toCheck.size();

			// starting with the beginning of the list,
			for (unsigned int i = 0; i != end; ++i)
			{
				// check to see if you can reach the current node
				// from there
				if (lineOfSight(toCheck[i]->position, current->position, toCheck[i]->tile, current->tile))
				{
					// if so, link the nodes
					current->previous = toCheck[i];

					// and note that the new previous node and any later nodes no longer need to be
					// part of the list
					end = i;
					break;
				}
			}

			// the last node in the list of those to be checked (after removal of smoothed-away
			// nodes) will be the current previous node of the new previous node of the current
			// node, so it doesn't need to be checked.
			if (0 < end)
				--end;

			// remove nodes that the next current node can't be smoothed to from the list
			do
			{
				toCheck.pop_back();
			} while (toCheck.size() > end);
		}
	}
}
