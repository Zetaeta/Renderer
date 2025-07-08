#pragma once

namespace rnd
{

enum class EPrimitiveTopology : u8
{
	TRIANGLES,
	TRIANGLE_STRIP
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
	Compute,
	Count,

	GraphicsStart = Vertex,
	GraphicsCount = Pixel + 1,
	VertexPipelineCount = Pixel + 1
};
ITER_ENUM(EShaderType);


}
