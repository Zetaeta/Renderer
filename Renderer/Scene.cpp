#include "Scene.h"

/* Mesh Mesh::MakeCube(vec3 origin, mat3 rotation, MaterialID mat)
{
	vec3 xax = rotation[0];
	vec3 yax = rotation[1];
	vec3 zax = rotation[2];
	std::vector<vec3> vertices = {origin, origin + xax, origin + yax, origin + xax + yax,
		origin + zax, origin + zax + xax, origin + zax + yax, origin + zax + xax + yax};
	std::vector<IndexedTri> tris;

	return Mesh(vertices, tris,mat);
}*/

using std::vector;

#define SQUARE(a, b, c, d) \
	{a,b,d},                      \
	{a,d,c}
#define SQUARE2(a,b,c,d) \
	{a,d,b},                       \
	{a,c,d}

#define SQUAREVERTS(start, localx, localy, normal, uv0, uvx, uvy) \
	Vertex{start, normal, uv0},\
	{start + localx, normal, uv0 + uvx},\
	{start + localy, normal, uv0 + uvy},\
	{start + localx + localy, normal, uv0 + uvx + uvy},

#define SV(start, localx, localy, normal) \
	SQUAREVERTS((start), (localx), (localy), (normal), (vec2{ 0, 0 }), (vec2{ 1, 0 }), (vec2{0,1})) 

inline vector<Vertex> Square(vec3 start, vec3 localx, vec3 localy, vec3 normal, vec2 uv0, vec2 uvx, vec2 uvy)
{
	return {Vertex{start, normal, uv0},
	{start + localx, normal, uv0 + uvx},
	{start + localy, normal, uv0 + uvy},
	{start + localx + localy, normal, uv0 + uvx + uvy}};
}

inline vector<Vertex> Square(vec3 start, vec3 localx, vec3 localy, vec3 normal)
{
	return Square(start, localx, localy, normal, { 0, 0 }, { 1, 0 }, {0,1});
}


Mesh Mesh::Cube(MaterialID m /*= 0*/)
{
	//std::vector<Vertex> verts = {
	//	Vertex{vec3{ -.5, -.5, -.5 }},
	//	Vertex{vec3{ .5, -.5, -.5 }},
	//	Vertex{vec3{ -.5, .5, -.5 }},
	//	Vertex{vec3{ .5, .5, -.5 }},
	//	Vertex{vec3{ -.5, -.5, .5 }},
	//	Vertex{vec3{ .5, -.5, .5 }},
	//	Vertex{vec3{ -.5, .5, .5 }},
	//	Vertex{vec3{ .5, .5, .5 }},
	//};
	std::vector<Vertex> verts = Flatten<Vertex>({
		Square({ -.5, -.5, -.5 },{ 1, 0, 0 }, { 0, 1, 0 }, { 0, 0, -1 }),
		Square({ .5, -.5, .5 }, { -1, 0, 0 }, { 0, 1, 0 }, { 0, 0, 1 }),
		//top
		Square({ -.5, .5, -.5 }, { 1, 0, 0 }, { 0, 0, 1 }, { 0, 1, 0 }),
		Square({ -.5, -.5, .5 }, { 1, 0, 0 }, { 0, 0, -1 }, { 0, -1, 0 }),
		//left
		Square({ -.5, -.5, .5 },{ 0, 0, -1 }, { 0, 1, 0 }, { -1, 0, 0 }),
		Square({ .5, -.5, -.5 },{ 0, 0, 1 }, { 0, 1, 0 }, { 1, 0, 0 }),
	});
#define SQ(x) \
	{x,x+3,x+1},        \
	{x,x+2,x+3}
	std::vector<IndexedTri> tris = {
		SQ(0),
		SQ(4),
		SQ(8),
		SQ(12),
		SQ(16),
		SQ(20),
	};
	return Mesh(std::move(verts), std::move(tris), "cube", m);
}
