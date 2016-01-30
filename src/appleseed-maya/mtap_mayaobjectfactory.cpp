#include "mayaobjectfactory.h"
#include "mtap_mayaobject.h"


namespace MayaTo
{
    sharedPtr<MayaObject> MayaObjectFactory::createMayaObject(MObject& mobject)
    {
        return sharedPtr<MayaObject>(new mtap_MayaObject(mobject));
    }
    sharedPtr<MayaObject> MayaObjectFactory::createMayaObject(MDagPath& objPath)
    {
        return sharedPtr<MayaObject>(new mtap_MayaObject(objPath));
    }
};
