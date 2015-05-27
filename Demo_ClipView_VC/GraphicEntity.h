#pragma once
#include "stdafx.h"
#include <vector>
using std::vector;
struct Line
{
public:
	CPoint startpoint;
	CPoint endpoint;
};

struct Circle
{
public:
	CPoint center;
	int radius;
};

struct Boundary
{
public:
	vector<CPoint> vertexs;
};