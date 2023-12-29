#include "Texture.h"
#include "stb_image.h"
std::shared_ptr<Texture> const Texture::EMPTY = Create(0,0,nullptr);

TextureRef Texture::LoadFrom(char const* fileName)
{
	int x, y, comp;
	unsigned char* data = stbi_load(fileName, &x, &y, &comp, 4);
	fprintf(stdout, "%s: width: %d, height: %d, comp: %d\n", fileName, x, y, comp);
	TextureRef result = (x > 0 && y > 0) ? Texture::Create(x,y,data) : Texture::EMPTY;
	return result;
}
