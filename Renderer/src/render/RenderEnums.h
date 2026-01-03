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

#define FOR_EACH_SHADER_FREQ(X)\
	X(Vertex)\
	X(Geometry)\
	X(TessControl)\
	X(TessEval)\
	X(Pixel)\
	X(Mesh)\
	X(Amplification)\
	X(Compute)\
	

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
	GraphicsCount = Amplification + 1,
	VertexPipelineCount = Pixel + 1
};
ITER_ENUM(EShaderType);


enum class EShaderTypeMask : u32
{
	#define MAKE_SHADER_TYPE_MASK(Freq) Freq = 1 << (u8) EShaderType::Freq,
	FOR_EACH_SHADER_FREQ(MAKE_SHADER_TYPE_MASK)
	#undef MAKE_SHADER_TYPE_MASK
};
FLAG_ENUM(EShaderTypeMask);

}
