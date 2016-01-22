#include "mayaobjectfactory.h"
#include "mtap_mayaobject.h"

namespace MayaTo
{
	std::shared_ptr<MayaObject> MayaObjectFactory::createMayaObject(MObject& mobject)
	{
		return std::shared_ptr<MayaObject>(new mtap_MayaObject(mobject));
	}
	std::shared_ptr<MayaObject> MayaObjectFactory::createMayaObject(MDagPath& objPath)
	{
		return std::shared_ptr<MayaObject>(new mtap_MayaObject(objPath));
	}
};