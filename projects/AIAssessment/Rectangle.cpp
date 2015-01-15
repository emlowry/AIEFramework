#include "Rectangle.h"

//
// LineSegment
//

LineSegment::LineSegment(float a_startX, float a_startY, float a_endX, float a_endY)
	: start(a_startX, a_startY), end(a_endX, a_endY) {}

LineSegment::LineSegment(const glm::vec2& a_start, const glm::vec2& a_end)
	: start(a_start), end(a_end) {}

LineSegment::LineSegment(const LineSegment& a_line)
	: start(a_line.start), end(a_line.end) {}

bool LineSegment::contains(const glm::vec2& a_point) const
{
	// trivial case
	if (start == end)
		return (a_point == start);

	// if point is outside line dimensions, no need for more complex math
	if (fmin(start.x, end.x) > a_point.x || a_point.x > fmax(start.x, end.x) ||
		fmin(start.y, end.y) > a_point.y || a_point.y > fmax(start.y, end.y))
		return false;
	
	double a = end.y - start.y;
	double b = start.x - end.x;
	double c = a*start.x + b*start.y;
	
	// vertical line
	if (0 == b)
	{
		return a_point.x == (c - b*a_point.y) / a;
	}

	return a_point.y == (c - a*a_point.x) / b;
}

bool LineSegment::contains(const LineSegment& a_line) const
{
	return contains(a_line.start) && contains(a_line.end);
}

bool LineSegment::intersects(const glm::vec2& a_point, glm::vec2* a_intersection) const
{
	bool result = contains(a_point);
	if (result && nullptr != a_intersection)
	{
		*a_intersection = a_point;
	}
	return result;
}

bool LineSegment::intersects(const LineSegment& a_line, LineSegment* a_intersection) const
{
	// if given a point
	if (a_line.start == a_line.end)
	{
		bool result = contains(a_line.start);
		if (result && nullptr != a_intersection)
			*a_intersection = a_line;
		return result;
	}

	double a1 = end.y - start.y;
	double b1 = start.x - end.x;
	double c1 = a1*start.x + b1*start.y;

	double a2 = a_line.end.y - a_line.start.y;
	double b2 = a_line.start.x - a_line.end.x;
	double c2 = a2*a_line.start.x + b2*a_line.start.y;

	double det = a1*b2 - a2*b1;

	// if parallel, return center of any overlapping area as intersection
	if (0 == det)
	{
		LineSegment line((a_line.contains(start) ? start : a_line.start),
						 (a_line.contains(end) ? end : a_line.end));
		bool result = contains(line.start) || contains(line.end);
		if (result && nullptr != a_intersection)
			*a_intersection = line;
		return result;
	}

	// calculate intersection
	glm::vec2 intersection = glm::vec2((b2*c1 - b1*c2) / det, (a1*c2 - a2*c1) / det);

	// if intersection is outside line dimensions, return false
	if (fmin(start.x, end.x) > intersection.x || intersection.x > fmax(start.x, end.x) ||
		fmin(start.y, end.y) > intersection.y || intersection.y > fmax(start.y, end.y) ||
		fmin(a_line.start.x, a_line.end.x) > intersection.x ||
		intersection.x > fmax(a_line.start.x, a_line.end.x) ||
		fmin(a_line.start.y, a_line.end.y) > intersection.y || 
		intersection.y > fmax(a_line.start.y, a_line.end.y))
		return false;

	// otherwise, return intersection
	if (nullptr != a_intersection)
		*a_intersection = LineSegment(intersection, intersection);
	return true;
}

Rectangle::Rectangle(float a_centerX, float a_centerY, float a_sizeX, float a_sizeY)
	: topRight(a_centerX + a_sizeX*0.5f, a_centerY + a_sizeY*0.5f),
	  bottomLeft(a_centerX - a_sizeX*0.5f, a_centerY - a_sizeY*0.5f) {}

Rectangle::Rectangle(const glm::vec2& a_center, const glm::vec2& a_size)
	: topRight(a_center + a_size*0.5f), bottomLeft(a_center - a_size*0.5f) {}

Rectangle::Rectangle(const Rectangle& a_rect)
	: topRight(a_rect.topRight), bottomLeft(a_rect.bottomLeft) {}

bool Rectangle::contains(const glm::vec2& a_point) const
{
	return	bottomLeft.x <= a_point.x && a_point.x <= topRight.x &&
			bottomLeft.y <= a_point.y && a_point.y <= topRight.y;
}

bool Rectangle::contains(const LineSegment& a_line) const
{
	return	contains(a_line.start) && contains(a_line.end);
}

bool Rectangle::contains(const Rectangle& a_rect) const
{
	return	contains(a_rect.topRight) && contains(a_rect.bottomLeft);
}

bool Rectangle::intersects(const glm::vec2& a_point, glm::vec2* a_intersection) const
{
	bool result = contains(a_point);
	if (result && nullptr != a_intersection)
		*a_intersection = a_point;
	return result;
}

bool containsEntireIntersection(const LineSegment& a_line1, const LineSegment& a_line2,
								glm::vec2& a_endChecked, LineSegment* a_intersection)
{
	LineSegment intersection = a_line1;
	if (a_line1.intersects(a_line2, &intersection))
	{
		if (intersection.start != intersection.end)
		{
			if (nullptr != a_intersection)
				*a_intersection = intersection;
			return true;
		}
		a_endChecked = intersection.start;
	}
	return false;
}

bool containsEntireIntersection(const Rectangle& a_rect, const LineSegment& a_line,
								glm::vec2& a_endChecked, LineSegment* a_intersection)
{
	return !a_rect.contains(a_endChecked) &&
		((a_endChecked.x >= a_rect.topRight.x &&
		  containsEntireIntersection(a_rect.edge(Rectangle::RIGHT), a_line,
									 a_endChecked, a_intersection)) ||
		 (a_endChecked.y <= a_rect.bottomLeft.y &&
		  containsEntireIntersection(a_rect.edge(Rectangle::BOTTOM), a_line,
									 a_endChecked, a_intersection)) ||
		 (a_endChecked.x <= a_rect.bottomLeft.x &&
		  containsEntireIntersection(a_rect.edge(Rectangle::LEFT), a_line,
									 a_endChecked, a_intersection)) ||
		 (a_endChecked.y >= a_rect.topRight.y &&
		  containsEntireIntersection(a_rect.edge(Rectangle::TOP), a_line,
									 a_endChecked, a_intersection)));
}

bool Rectangle::intersects(const LineSegment& a_line, LineSegment* a_intersection) const
{
	// trivial cases
	if (contains(a_line))
	{
		if (nullptr != a_intersection)
			*a_intersection = a_line;
		return true;
	}
	else if (a_line.start == a_line.end)
	{
		return false;
	}

	// calculate intersection
	LineSegment intersection = a_line;
	if (containsEntireIntersection(*this, a_line, intersection.start, a_intersection) ||
		containsEntireIntersection(*this, a_line, intersection.end, a_intersection))
		return true;

	// return results
	bool result = contains(intersection);
	if (result && nullptr != a_intersection)
		*a_intersection = intersection;
	return result;
}

bool Rectangle::intersects(const Rectangle& a_rect, Rectangle* a_intersection) const
{
	bool result = bottomLeft.x <= a_rect.topRight.x &&
				  a_rect.bottomLeft.x <= topRight.x &&
				  bottomLeft.y <= a_rect.topRight.y &&
				  a_rect.bottomLeft.y <= topRight.y;
	if (result && nullptr != a_intersection)
	{
		a_intersection->bottomLeft = glm::vec2(fmin(bottomLeft.x, a_rect.bottomLeft.x),
											   fmin(bottomLeft.y, a_rect.bottomLeft.y));
		a_intersection->topRight = glm::vec2(fmax(topRight.x, a_rect.topRight.x),
											 fmax(topRight.y, a_rect.topRight.y));
	}
	return result;
}

// (result & 1 << 0) = this rectangle shares its right edge with the other
// (result & 1 << 1) = this rectangle shares its bottom edge with the other
// (result & 1 << 2) = this rectangle shares its left edge with the other
// (result & 1 << 3) = this rectangle shares its top edge with the other
// (result & 1 << 4) = other rectangle shares its right edge with this one
// (result & 1 << 5) = other rectangle shares its bottom edge with this one
// (result & 1 << 6) = other rectangle shares its left edge with this one
// (result & 1 << 7) = other rectangle shares its top edge with this one
Rectangle::SharedEdgeFlags Rectangle::sharedEdges(const Rectangle& a_rect) const
{
	// This rectangle shares its right edge
	if ((topRight == a_rect.topRight && bottomRight() == a_rect.bottomRight()) ||
		(topRight == a_rect.bottomRight() && bottomRight() == a_rect.topRight))
	{
		// other rectangle shares its right edge
		return (SharedEdgeFlags)(RIGHT_SHARED | (RIGHT_SHARED << EDGE_COUNT));
	}
	if ((topRight == a_rect.bottomRight() && bottomRight() == a_rect.bottomLeft) ||
		(topRight == a_rect.bottomLeft && bottomRight() == a_rect.bottomRight()))
	{
		// other rectangle shares its bottom edge
		return (SharedEdgeFlags)(RIGHT_SHARED | (BOTTOM_SHARED << EDGE_COUNT));
	}
	if ((topRight == a_rect.bottomLeft && bottomRight() == a_rect.topLeft()) ||
		(topRight == a_rect.topLeft() && bottomRight() == a_rect.bottomLeft))
	{
		// other rectangle shares its left edge
		return (SharedEdgeFlags)(RIGHT_SHARED | (LEFT_SHARED << EDGE_COUNT));
	}
	if ((topRight == a_rect.topLeft() && bottomRight() == a_rect.topRight) ||
		(topRight == a_rect.topRight && bottomRight() == a_rect.topLeft()))
	{
		// other rectangle shares its top edge
		return (SharedEdgeFlags)(RIGHT_SHARED | (TOP_SHARED << EDGE_COUNT));
	}

	// This rectangle shares its bottom edge
	if ((bottomRight() == a_rect.topRight && bottomLeft == a_rect.bottomRight()) ||
		(bottomRight() == a_rect.bottomRight() && bottomLeft == a_rect.topRight))
	{
		// other rectangle shares its right edge
		return (SharedEdgeFlags)(BOTTOM_SHARED | (RIGHT_SHARED << EDGE_COUNT));
	}
	if ((bottomRight() == a_rect.bottomRight() && bottomLeft == a_rect.bottomLeft) ||
		(bottomRight() == a_rect.bottomLeft && bottomLeft == a_rect.bottomRight()))
	{
		// other rectangle shares its bottom edge
		return (SharedEdgeFlags)(BOTTOM_SHARED | (BOTTOM_SHARED << EDGE_COUNT));
	}
	if ((bottomRight() == a_rect.bottomLeft && bottomLeft == a_rect.topLeft()) ||
		(bottomRight() == a_rect.topLeft() && bottomLeft == a_rect.bottomLeft))
	{
		// other rectangle shares its left edge
		return (SharedEdgeFlags)(BOTTOM_SHARED | (LEFT_SHARED << EDGE_COUNT));
	}
	if ((bottomRight() == a_rect.topLeft() && bottomLeft == a_rect.topRight) ||
		(bottomRight() == a_rect.topRight && bottomLeft == a_rect.topLeft()))
	{
		// other rectangle shares its top edge
		return (SharedEdgeFlags)(BOTTOM_SHARED | (TOP_SHARED << EDGE_COUNT));
	}

	// This rectangle shares its left edge
	if ((bottomLeft == a_rect.topRight && topLeft() == a_rect.bottomRight()) ||
		(bottomLeft == a_rect.bottomRight() && topLeft() == a_rect.topRight))
	{
		// other rectangle shares its right edge
		return (SharedEdgeFlags)(LEFT_SHARED | (RIGHT_SHARED << EDGE_COUNT));
	}
	if ((bottomLeft == a_rect.bottomRight() && topLeft() == a_rect.bottomLeft) ||
		(bottomLeft == a_rect.bottomLeft && topLeft() == a_rect.bottomRight()))
	{
		// other rectangle shares its bottom edge
		return (SharedEdgeFlags)(LEFT_SHARED | (BOTTOM_SHARED << EDGE_COUNT));
	}
	if ((bottomLeft == a_rect.bottomLeft && topLeft() == a_rect.topLeft()) ||
		(bottomLeft == a_rect.topLeft() && topLeft() == a_rect.bottomLeft))
	{
		// other rectangle shares its left edge
		return (SharedEdgeFlags)(LEFT_SHARED | (LEFT_SHARED << EDGE_COUNT));
	}
	if ((bottomLeft == a_rect.topLeft() && topLeft() == a_rect.topRight) ||
		(bottomLeft == a_rect.topRight && topLeft() == a_rect.topLeft()))
	{
		// other rectangle shares its top edge
		return (SharedEdgeFlags)(LEFT_SHARED | (TOP_SHARED << EDGE_COUNT));
	}

	// This rectangle shares its top edge
	if ((topLeft() == a_rect.topRight && topRight == a_rect.bottomRight()) ||
		(topLeft() == a_rect.bottomRight() && topRight == a_rect.topRight))
	{
		// other rectangle shares its right edge
		return (SharedEdgeFlags)(TOP_SHARED | (RIGHT_SHARED << EDGE_COUNT));
	}
	if ((topLeft() == a_rect.bottomRight() && topRight == a_rect.bottomLeft) ||
		(topLeft() == a_rect.bottomLeft && topRight == a_rect.bottomRight()))
	{
		// other rectangle shares its bottom edge
		return (SharedEdgeFlags)(TOP_SHARED | (BOTTOM_SHARED << EDGE_COUNT));
	}
	if ((topLeft() == a_rect.bottomLeft && topRight == a_rect.topLeft()) ||
		(topLeft() == a_rect.topLeft() && topRight == a_rect.bottomLeft))
	{
		// other rectangle shares its left edge
		return (SharedEdgeFlags)(TOP_SHARED | (LEFT_SHARED << EDGE_COUNT));
	}
	if ((topLeft() == a_rect.topLeft() && topRight == a_rect.topRight) ||
		(topLeft() == a_rect.topRight && topRight == a_rect.topLeft()))
	{
		// other rectangle shares its top edge
		return (SharedEdgeFlags)(TOP_SHARED | (TOP_SHARED << EDGE_COUNT));
	}

	// no shared edges
	return NONE;
}	// Rectangle::sharedEdges
