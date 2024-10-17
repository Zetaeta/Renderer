
#include "VertexTypes.h"
#include "VertexAttributes.h"

namespace rnd
{

CREATE_VERTEX_ATTRIBUTE_DESC(FlatVert, {
	return VertexAttributeDesc(
	{
		DataLayoutEntry{ &GetTypeInfo<vec3>(), "Position", offsetof(FlatVert, pos)},
		DataLayoutEntry{ &GetTypeInfo<vec2>(), "TexCoord", offsetof(FlatVert, uv)}
	});
});

}