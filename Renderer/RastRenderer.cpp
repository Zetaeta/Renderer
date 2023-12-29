#include "RastRenderer.h"
#include "Scene.h"
#include <functional>
#include <algorithm>
#include "Camera.h"
#include <utility>

using std::swap;
using std::vector;
using std::array;
using std::begin;
using std::end;
using std::pair;

#define IND(x, y) (x + y * m_Width)

using Plane = vec4;
using ClipSpaceTri = std::array<ClipSpaceVertex, 3>;
u32 ClipTri(ClipSpaceTri const& tri, Plane const& plane, std::array<ClipSpaceTri, 2>& outClippedTris) {
	array<float, 3> signedDist;
	u8 wrongSide = 0;
	for (int i = 0; i < 3; ++i)
	{
		signedDist[i] = (tri[i].pos, plane);
		if (signedDist[i] < 0)
		{
			wrongSide++;
		}
	}
	switch (wrongSide)
	{
		case 0:
			outClippedTris[0] = tri;
			return 1;
		case 1:
		{
			u32 out = find_if(begin(signedDist), end(signedDist), [](float sd)
							{
								return sd < 0;
							})
							- signedDist.begin();
			u32 in1 = (out + 1) % 3, in2 = (out + 2) % 3;
			float l1 = signedDist[in1], l2 = signedDist[in2], lo = -signedDist[out];
			float w1 = lo / (l1 + lo), w2 = lo / (l2 + lo);
			float wo1 = l1 / (l1 + lo), wo2 = l2 / (l2 + lo);
			
			ClipSpaceVertex const v0 = wo1 * tri[out] + w1 * tri[in1];
			ClipSpaceVertex const v3 = wo2 * tri[out] + w2 * tri[in2];
			ClipSpaceVertex const &v1 = tri[in1], v2 = tri[in2];
			// 0 __ 3
			//  /__\
			// 1    2
			//v0.normal = -v0.normal;
			outClippedTris[0] = {v0, v1, v2};
			outClippedTris[1] = {v0, v2, v3};
			return 2;
		}
		case 2:
		{
			u32 in = find_if(begin(signedDist), end(signedDist), [](float sd)
							{
								return sd >= 0;
							})
							- signedDist.begin();
			u32 out1 = (in + 1) % 3, out2 = (in + 2) % 3;
			float l1 = signedDist[out1], l2 = signedDist[out2], li = -signedDist[in];
			float w1 = li / (l1 + li), w2 = li / (l2 + li);
			float wi1 = l1 / (l1 + li), wi2 = l2 / (l2 + li);
			
			ClipSpaceVertex const v1 = wi1 * tri[in] + w1 * tri[out1];
			ClipSpaceVertex const v2 = wi2 * tri[in] + w2 * tri[out2];
			outClippedTris[0] = { tri[in], v1, v2 };
			return 1;
		}
		case 3:
		default:
			return 0;
	}
}


template<u32 NumPlanes, typename TIt, typename TFunc>
inline typename std::enable_if_t<NumPlanes != 0, void> RecursiveClip(TIt const& planeBegin, TIt const& planeEnd, ClipSpaceTri const& verts, TFunc const& func)
{
	Plane const& plane = *planeBegin;
	std::array<ClipSpaceTri, 2> clippedTris;
	u32 triCount = ClipTri(verts, plane, clippedTris);
	for (u32 i = 0; i < triCount; ++i)
	{
		RecursiveClip<NumPlanes - 1, TIt>(planeBegin + 1, planeEnd, clippedTris[i], func);
	}
}

template<u32 NumPlanes, typename TIt, typename TFunc>
inline typename std::enable_if_t<NumPlanes == 0, void> RecursiveClip(TIt const&, TIt const&, ClipSpaceTri const& tri, TFunc const& func) {
	func(tri);
}

//constexpr float const CLIP_X_MAX = 

void RastRenderer::Render(const Scene& scene)
{
	//mat4
	memset(m_Canvas, 0, m_Width * m_Height * sizeof(u32));
	memset(&m_DepthBuffer[0], 0, m_Width * m_Height * sizeof(float));
	mat4 worldToCamera = m_Camera->WorldToCamera();
	m_Projection =
	{ 
		{m_Scale,	0,		0,0},
		{0,			m_Scale,0,0},
		{0,			0,		1,0},
		{0,			0,		1,0},
	};
	m_Projection = glm::transpose(m_Projection);
	mat4 projWorld = m_Projection * worldToCamera;
	m_Scene = &scene;
	for (auto const& mi : scene.m_MeshInstances)
	{
		Transform const& trans = mi.trans;
		Mesh const&		 mesh = scene.m_Meshes[mi.mesh];
		mat4 fullTrans = projWorld * mat4(trans);
		for (auto tri : mesh.triangles)
		{
			
			//vec3 v0 = trans * mesh.vertices[tri[0]], v1 = trans * mesh.vertices[tri[1]], v2 = trans * mesh.vertices[tri[2]];
			Vertex v0 = mesh.vertices[tri[0]], v1 = mesh.vertices[tri[1]], v2 = mesh.vertices[tri[2]];
			array<vec4, 3> verts =	{fullTrans * vec4(v0.pos, 1), fullTrans * vec4(v1.pos, 1), fullTrans * vec4(v2.pos,1)};

			/* DrawLine(Project(worldToCamera * mat4(trans) * vec4(v0, 1)), Project(worldToCamera * mat4(trans) * vec4(v1, 1)));
			DrawLine(Project(worldToCamera * mat4(trans) * vec4(v0,1)), Project(worldToCamera * mat4(trans) * vec4(v2,1)));
			DrawLine(Project(worldToCamera * mat4(trans) * vec4(v1,1)), Project(worldToCamera * mat4(trans) * vec4(v2,1)));*/
			//DrawTri({worldToCamera * vec4(v0,1), worldToCamera * vec4(v1,1), worldToCamera * vec4(v2,1)});
			float  const CLIP_X_MAX = m_Width;
			float  const CLIP_X_MIN = 0;
			float  const CLIP_Y_MAX = m_Height;
			float  const CLIP_Y_MIN = 0;
			float  const CLIP_Z_MAX = 1000;
			float  const CLIP_Z_MIN = 1;

			//array<Plane, NUM_PLANES> clipPlanes =
			Plane clipPlanes[] = 
			{
				vec4{-1,0,0,CLIP_X_MAX},
				vec4{1,0,0,-CLIP_X_MIN},
				vec4{0,-1,0,CLIP_Y_MAX},
				vec4{0,1,0,-CLIP_Y_MIN},
				vec4{0,0,-1,CLIP_Z_MAX},
				vec4{0,0,1,-CLIP_Z_MIN}
			};
			constexpr static u64 const NUM_PLANES = sizeof(clipPlanes)/sizeof(Plane); 
			ClipSpaceTri			   tri
			{
				ClipSpaceVertex {verts[0], v0.normal, v0.uvs, v0.pos},
				ClipSpaceVertex {verts[1], v1.normal, v1.uvs, v1.pos},
				ClipSpaceVertex {verts[2], v2.normal, v2.uvs, v2.pos}
			};

			RecursiveClip<NUM_PLANES>(std::begin(clipPlanes), std::end(clipPlanes), tri, [&] (ClipSpaceTri const& clippedTri) {
				DrawTri(fullTrans,clippedTri, mesh.material);
			});
		}
	}
}


inline int Round(int x) {
	return x;
}

inline int Round(float x) {
	return int(round(x));
}

template<typename TX, typename TY//, typename TOut
>
inline void Interpolate(TX x1, TY f1, TX x2, TY f2, int min, int max, std::function < void(TX, TY)> func) {
	int i1 = Round(x1);
	int i2 = Round(x2);
	if (i1 == i2 && i1 >= min && i2 <= max)
	{
		func(i1, f1);
		return;
	}
	TY slope = (f2 - f1) / float(i2-i1);
	TY f = f1; 
	if (i1 < min)
	{
		f += slope * float(min - i1);
		i1 = min;
	}
	if (i2 > max)
	{
		i2 = max;
	}
	for (int i = i1; i <= i2; ++i)
	{
		func(i, f);
		f += slope;
	}
}

template<typename TIn>
inline void InterpolateToPix(TIn x1, float f1, TIn x2, float f2, int min, int max, std::function<void (int, int)> func) {
	int i1 = Round(x1);
	int i2 = Round(x2);
	if (i1 == i2 && i1 >= min && i2 <= max)
	{
		func(i1, Round(f1));
		return;// {i1,i2};
	}
	float slope = float(f2 - f1) / float(i2-i1);
	float f = f1; 
	if (i1 < min)
	{
		f += (min - i1) * slope;
		i1 = min;
	}
	if (i2 > max)
	{
		i2 = max;
	}
	for (int i = i1; i <= i2; ++i)
	{
		func(i, Round(f));
		f += slope;
	}
//	return {i1,i2};
//	func(i2, Round(f2));
}


void RastRenderer::DrawLine(vec2 a, vec2 b)
{
	//printf("Drawing line from (%f, %f) to (%f,%f)\n", a.x, a.y,b.x,b.y);
	if (abs(a.x - b.x) > abs(a.y - b.y))
	{
		if (a.x > b.x)
		{
			swap(a,b);
		}
		InterpolateToPix(a.x, a.y, b.x, b.y, 0, m_Width, [this](int x, int y) {
			if (x >= 0 && x < m_Width && y >= 0 && y < m_Height)
			{
				m_Canvas[x + y * m_Width] = 0xfffffffff;
			}
		});
	}
	else
	{
		if (a.y > b.y)
		{
			swap(a,b);
		}
		InterpolateToPix(a.y, a.x, b.y, b.x, 0, m_Height, [this](int y, int x) {
			if (x >= 0 && x < m_Width && y >= 0 && y < m_Height)
			{
				m_Canvas[x + y * m_Width] = 0xfffffffff;
			}
		});
	}
}

vec3 Vec3(vec2 v) {
	return vec3(v.x, v.y, 0);
}

template<typename T>
inline T DepthLinearCombo(vec3 bary, vec3 vertIDepths, float idepth, T t0, T t1, T t2)
{
	return (bary[0] * vertIDepths[0] * t0 + bary[1] * vertIDepths[1] * t1 + bary[2] * vertIDepths[2] * t2) / idepth;
}

void RastRenderer::DrawTri(mat4 const& fullTrans, std::array<ClipSpaceVertex, 3> const& verts, MaterialID matId)
{
	std::array<vec2, 3> vs = { vec2(verts[0].pos) / verts[0].pos.w, vec2(verts[1].pos) / verts[1].pos.w, vec2(verts[2].pos) / verts[2].pos.w };
	if (glm::cross(Vec3(vs[2] - vs[0]), Vec3(vs[1] - vs[0])).z < 0)
	{
		return;
	}

	static std::vector<int> x01, x02, x12, x012;
	static vector<vec3> b02, b012;
	x01.clear();
	x02.clear();
	x12.clear();
	b02.clear();
	b012.clear();
	x01.reserve(m_Height);
	x02.reserve(m_Height);
	x12.reserve(m_Height);
	x012.reserve(m_Height);

	std::array<u32, 3> is = {0,1,2};

	constexpr static std::array<vec3,3> const bary = {vec3{ 1.f, 0.f, 0.f }, { 0.f, 1.f, 0.f }, { 0.f, 0.f, 1.f }};

	std::sort(is.begin(), is.end(), [&vs](u32 i, u32 j) {return vs[i].y < vs[j].y;});

	int top = int(round(vs[is[2]].y));
	int mid = int(round(vs[is[1]].y));
	int bot = int(round(vs[is[0]].y));


	InterpolateToPix(vs[is[0]].y, vs[is[0]].x, vs[is[1]].y, vs[is[1]].x, 0, m_Height - 1, [this](int y, int x) {
		x01.push_back(x);
	});
	InterpolateToPix(vs[is[0]].y, vs[is[0]].x, vs[is[2]].y, vs[is[2]].x, 0, m_Height - 1, [this](int y, int x) {
		x02.push_back(x);
	});
	if (x02.empty())
	{
		return;
	}
	InterpolateToPix(vs[is[1]].y, vs[is[1]].x, vs[is[2]].y, vs[is[2]].x, 0, m_Height - 1, [this](int y, int x) {
		x12.push_back(x);
	});

	Interpolate<int, vec3>(Round(vs[is[0]].y), bary[is[0]], Round(vs[is[2]].y), bary[is[2]], 0, m_Height - 1, [](int y, vec3 b) {
		b02.push_back(b);
	});
	Interpolate<int, vec3>(Round(vs[is[0]].y), bary[is[0]], Round(vs[is[1]].y), bary[is[1]], 0, m_Height - 1, [](int y, vec3 b) {
		b012.push_back(b);
	});
	if (!x01.empty() && !x12.empty())
	{
		b012.pop_back();
	}
	Interpolate<int, vec3>(Round(vs[is[1]].y), bary[is[1]], Round(vs[is[2]].y), bary[is[2]], 0, m_Height - 1, [](int y, vec3 b) {
		b012.push_back(b);
	});
	if (x01.size() + x12.size() != x02.size() + 1)
	{
		printf("top %d, mid %d, bot %d\n", top, mid, bot);
		printf("Edge lengths %d, %d, %d\n", x01.size(), x02.size(), x12.size());
	}
	x012.resize(x02.size());
	for (int i = 0; i < x01.size(); ++i)
	{
		x012[i] = x01[i];
	}
	u32 offset = x01.empty() ? 0 : 1;
	for (int i = offset; i < x12.size(); ++i)
	{
		x012[i + x01.size() - offset] = x12[i];
	}
	assert(x02.size() == x012.size() && x02.size() == b012.size() && x02.size() == b02.size());
	u32	 midLength = x02.size()/2;
	bool pointyRight = false;
	if (x012[midLength] >= x02[midLength])
	{
		pointyRight = true;
	}
	//pointyRight = !pointyRight;
	vector<int> const& left = pointyRight ? x02 : x012;
	vector<int> const& right = pointyRight ? x012 : x02;

	vector<vec3> const& leftb = pointyRight ? b02 : b012;
	vector<vec3> const& rightb = pointyRight ? b012 : b02;
	vec3				inverseDepths = { 1/verts[0].pos.w, 1/ verts[1].pos.w, 1/verts[2].pos.w};
	float colourScale = 1.f;
	//if (pointyRight)
	//{
	//	colourScale = .2f;
	//}
	vec3 facenormal = {1,1,1};//glm::normalize( -glm::cross(verts[2].worldPos - verts[0].worldPos, verts[1].worldPos - verts[0].worldPos));
	bool useFaceNorm = false;
	if (verts[0].normal == vec3(0) && verts[1].normal == vec3(0) && verts[2].normal == vec3(0))
	{
		useFaceNorm = true;
	}
	for (int i = 0; i < x02.size(); ++i)
	{
		int y = max(bot,0) + i;
		Interpolate<int,vec3>(left[i], leftb[i], right[i], rightb[i], 0, m_Width - 1, [this, y, &inverseDepths, start = left[i], colourScale, matId, &verts, &facenormal, useFaceNorm
		] (int x, vec3 b) {
			//if (x >= 0 && x < m_Width && y >= 0 && y < m_Height)
			{
				float idepth = glm::dot(b, inverseDepths);
				if (idepth > m_DepthBuffer[x + y * m_Width])
				{
					
					vec3			normal = useFaceNorm ? facenormal : glm::normalize(LinearCombo(b, array<vec3,3>{ verts[0].normal, verts[1].normal, verts[2].normal }));
					vec3 pos = (b[0] * verts[0].worldPos + b[1] * verts[1].worldPos + b[2] * verts[2].worldPos);
					Material const& mat = m_Scene->m_Materials[matId];
					float lighting = ComputeLighting(normal, glm::normalize(pos - m_Camera->position), mat.specularity);
					Colour_t colour = mat.colour;
					if (mat.albedo->IsValid())
					{
						vec2 uvs = LinearCombo(b, array<vec2, 3>{ verts[0].uvs * inverseDepths[0], verts[1].uvs * inverseDepths[1], verts[2].uvs * inverseDepths[2] }) / idepth;
						colour =  mat.albedo[uvs];
					}
					//m_Canvas[x + y * m_Width] = vecToRGBA(colour);
					m_DepthBuffer[IND(x,y)] = idepth;
					//m_Canvas[x + y * m_Width] = vecToRGBA(vec4(b*colourScale, 1));
					//m_Canvas[x + y * m_Width] = vecToRGBA(vec4(max(normal, vec3(0)) * lighting, 1.f));
					//m_Canvas[x + y * m_Width] = vecToRGBA(vec4(max(normal, vec3(0)), 1.f));
//					m_Canvas[x + y * m_Width] = vecToRGBA(vec4(max(normalize(pos - m_Camera->position), vec3(0)), 1.f));
					m_Canvas[x + y * m_Width] = vecToRGBA(max(colour, vec4(0)) * lighting);
				}
			}
		});
//		for (int x = left[i]; x <= right[i]; ++x)
//		{
//			if (x >= 0 && x < m_Width && y >= 0 && y < m_Height)
//			{
//				m_Canvas[x + y * m_Width] = 0xffff00ffff;
//			}
//		}
	}
}

float RastRenderer::ComputeLighting( const vec3& normal, const vec3& rayDir, int specularity)
{
	float intensity = m_Scene->m_AmbientLight;
	for (auto dirLight : m_Scene->m_DirLights) {
		intensity +=  max(dot(-dirLight.dir, normal),0.f) * dirLight.intensity;
		if (true && specularity >= 0) {
			float l = glm::length(dirLight.dir);
			//assert(l < 1.1f);
			vec3 outRay = dirLight.dir - 2 * dot(dirLight.dir, normal) * normal;
			//assert(dot(outRay, -rayDir) < 1.0001f);
			float specLight = pow(max(dot(outRay, -rayDir),0.f), specularity) * dirLight.intensity;
			//return specLight;
			intensity += specLight;
		}
	}
	return min(intensity,1.f);
}

void RastRenderer::Resize(u32 width, u32 height, u32* canvas)
{
	m_Width = width;
	m_Height = height;
	m_Canvas = canvas;
	m_DepthBuffer.resize(width * height);
	m_Dirty = true;
	m_Scale = min(width, height);
}
