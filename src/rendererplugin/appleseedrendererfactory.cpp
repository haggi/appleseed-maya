#include "world.h"
#include "rendering/rendererfactory.h"
#include "rendering/renderer.h"
#include "appleseed.h"

namespace MayaTo
{
	void MayaRendererFactory::createRenderer()
	{
		getWorldPtr()->worldRendererPtr = sharedPtr<Renderer>(new AppleRender::AppleseedRenderer());
	}
	sharedPtr<Renderer> MayaRendererFactory::getRenderer()
	{
		return getWorldPtr()->worldRendererPtr;
	}
	void MayaRendererFactory::deleteRenderer()
	{
		getWorldPtr()->worldRendererPtr.reset();
	}
};