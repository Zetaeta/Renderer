#include "VertexAttributes.h"

namespace rnd
{

VertexAttributeDesc::Registry::Registry()
{
	InitSemanticMap();
}

SmallMap<Name, VertexAttributeMask> VertexAttributeDesc::SemanticMap;

void VertexAttributeDesc::InitSemanticMap()
{
	SemanticMap["Position"] = VA_Position;
	SemanticMap["Normal"] = VA_Normal;
	SemanticMap["Tangent"] = VA_Tangent;
	SemanticMap["TexCoord"] = VA_TexCoord;
}

}

