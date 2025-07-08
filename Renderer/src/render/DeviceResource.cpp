#include "DeviceResource.h"
#include "DeviceTexture.h"
#include "RenderDevice.h"

namespace rnd
{

IDeviceResource::IDeviceResource(IRenderDevice* device)
	: mDevice(device)
{
	device->RegisterResource(this);
}

IDeviceResource::~IDeviceResource()
{
	mDevice->UnregisterResource(this);
}

EResourceType IDeviceResource::GetResourceType() const
{
	return static_cast<const IDeviceTexture*>(this)->Desc.ResourceType;
}

}
