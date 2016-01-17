#include "appleseed.h"

#include <maya/MColor.h>
#include <maya/MFnInstancer.h>

#include "mayaScene.h"
#include "utilities/tools.h"
#include "utilities/attrTools.h"
#include "utilities/logging.h"
#include "appleseedUtils.h"

static Logging logger;

using namespace AppleRender;

void AppleseedRenderer::updateTransform(std::shared_ptr<MayaObject> obj)
{
	Logging::debug(MString("AppleseedRenderer::updateTransform: ") + obj->shortName);
}

void AppleseedRenderer::updateShape(std::shared_ptr<MayaObject> obj)
{
	Logging::debug(MString("AppleseedRenderer::updateShape: ") + obj->shortName);
	//updateGeometry(obj);
}

