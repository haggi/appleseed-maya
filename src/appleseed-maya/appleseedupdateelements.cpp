#include "appleseed.h"

#include <maya/MColor.h>
#include <maya/MFnInstancer.h>

#include "mayascene.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "appleseedutils.h"

static Logging logger;

using namespace AppleRender;

void AppleseedRenderer::updateTransform(sharedPtr<MayaObject> obj)
{
    Logging::debug(MString("AppleseedRenderer::updateTransform: ") + obj->shortName);
}

void AppleseedRenderer::updateShape(sharedPtr<MayaObject> obj)
{
    Logging::debug(MString("AppleseedRenderer::updateShape: ") + obj->shortName);
    //updateGeometry(obj);
}
