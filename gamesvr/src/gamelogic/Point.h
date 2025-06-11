#pragma once
#include <cstdint>

using PosType = int16_t;

struct Point {
	PosType x, y;
	Point(PosType x, PosType y) : x(x), y(y) {}
	bool operator==(const Point& other) const {
		return x == other.x && y == other.y;
	}
};
template<>
struct std::hash<Point> {
	size_t operator()(const Point& p) const noexcept
	{
		return (static_cast<size_t>(p.x) << 16) ^ p.y;
	}
};
