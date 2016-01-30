#include "swatchesrenderer/swatchrendererinterfacefactory.h"
#include "mtap_swatchrenderer.h"


SwatchRendererInterface *SwatchRendererInterfaceFactory::createSwatchRendererInterface(MObject dependNode, MObject renderNode, int imageResolution)
{
	return (SwatchRendererInterface *)new mtap_SwatchRendererInterface(dependNode, renderNode, imageResolution);
}

void SwatchRendererInterfaceFactory::deleteSwatchRendererInterface(SwatchRendererInterface *swInterface)
{
	mtap_SwatchRendererInterface *sr = (mtap_SwatchRendererInterface *)swInterface;
	if (sr != NULL)
		delete sr;
}