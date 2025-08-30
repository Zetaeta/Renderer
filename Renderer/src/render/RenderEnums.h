#pragma once

namespace rnd
{

enum class EPrimitiveTopology : u8
{
	Triangles,
	TriangleStrip
};

enum class EDeviceMeshType : u8
{
	DIRECT,
	INDEXED
};

enum class EShaderType : u8
{
	Vertex,
	Geometry,
	TessControl,
	TessEval,
	Pixel,
	Mesh,
	Amplification,
	Compute,
	Count,

	GraphicsStart = Vertex,
	GraphicsCount = Mesh + 1,
	VertexPipelineCount = Pixel + 1
};
ITER_ENUM(EShaderType);


}
