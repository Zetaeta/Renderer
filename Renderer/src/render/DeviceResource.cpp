#include "DeviceResource.h"
#include "DeviceTexture.h"

namespace rnd
{

EResourceType IDeviceResource::GetResourceType()
{
	return static_cast<IDeviceTexture*>(this)->Desc.ResourceType;
}

}
