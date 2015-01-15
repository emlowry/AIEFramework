#pragma once

#include <glm/glm.hpp>

struct LineSegment
{
	glm::vec2	start;
	glm::vec2	end;

	LineSegment(float a_startX, float a_startY, float a_endX, float a_endY);
	LineSegment(const glm::vec2& a_start, const glm::vec2& a_end);
	LineSegment(const LineSegment& a_line);

	bool contains(const glm::vec2& a_point) const;
	bool contains(const LineSegment& a_line) const;
	bool intersects(const glm::vec2& a_point,
					glm::vec2* a_intersection = nullptr) const;
	bool intersects(const LineSegment& a_line,
					LineSegment* a_intersection = nullptr) const;
};

struct Rectangle
{
	enum EdgeIndex { RIGHT = 0, BOTTOM = 1, LEFT = 2, TOP = 3, EDGE_COUNT = 4 };
	enum SharedEdgeFlags
	{
		NONE = 0,
		RIGHT_SHARED = 1 << RIGHT,
		BOTTOM_SHARED = 1 << BOTTOM,
		LEFT_SHARED = 1 << LEFT,
		TOP_SHARED = 1 << TOP,
	};

	glm::vec2	topRight;
	glm::vec2	bottomLeft;

	glm::vec2	topLeft() const		{ return glm::vec2(bottomLeft.x, topRight.y); }
	glm::vec2	bottomRight() const	{ return glm::vec2(topRight.x, bottomLeft.y); }
	glm::vec2	center() const		{ return (topRight + bottomLeft) * 0.5f; }
	glm::vec2	size() const		{ return topRight - bottomLeft; }
	glm::vec2	extents() const		{ return size() * 0.5f; }
	LineSegment	edge(EdgeIndex a_edge) const
	{
		switch (a_edge)
		{
		case RIGHT:		return LineSegment(topRight, bottomRight()); break;
		case BOTTOM:	return LineSegment(bottomRight(), bottomLeft); break;
		case LEFT:		return LineSegment(bottomLeft, topLeft()); break;
		case TOP:		return LineSegment(topLeft(), topRight); break;
		}
		return LineSegment(bottomLeft, topRight);
	}

	Rectangle(float a_centerX, float a_centerY, float a_sizeX, float a_sizeY);
	Rectangle(const glm::vec2& a_center, const glm::vec2& a_size);
	Rectangle(const Rectangle& a_rect);

	bool contains(const glm::vec2& a_point) const;
	bool contains(const LineSegment& a_line) const;
	bool contains(const Rectangle& a_rect) const;
	bool intersects(const glm::vec2& a_point, glm::vec2* a_intersection = nullptr) const;
	bool intersects(const LineSegment& a_line, LineSegment* a_intersection = nullptr) const;
	bool intersects(const Rectangle& a_rect, Rectangle* a_intersection = nullptr) const;

	// (result & 1 << 0) = this rectangle shares its right edge with the other
	// (result & 1 << 1) = this rectangle shares its bottom edge with the other
	// (result & 1 << 2) = this rectangle shares its left edge with the other
	// (result & 1 << 3) = this rectangle shares its top edge with the other
	// (result & 1 << 4) = other rectangle shares its right edge with this one
	// (result & 1 << 5) = other rectangle shares its bottom edge with this one
	// (result & 1 << 6) = other rectangle shares its left edge with this one
	// (result & 1 << 7) = other rectangle shares its top edge with this one
	SharedEdgeFlags sharedEdges(const Rectangle& a_rect) const;
};
