#include "appleseed.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "utilities/logging.h"
#include "appleseedutils.h"
#include "mayascene.h"
#include <maya/MFnCamera.h>
#include "world.h"

using namespace AppleRender;
//
//	Appleseed does not support more than one camera at the moment, so break after the first one.
//	Cameras are more a mixture between assembly and object. They have a transformation like an assembly but
//	attributes like an object.

void AppleseedRenderer::defineCamera(sharedPtr<MayaObject> cam)
{
    MStatus stat;
    sharedPtr<MayaScene> mayaScene = MayaTo::getWorldPtr()->worldScenePtr;
    sharedPtr<RenderGlobals> renderGlobals = MayaTo::getWorldPtr()->worldRenderGlobalsPtr;

    asr::Camera *camera = project->get_scene()->get_camera();
    if (camera != nullptr)
        Logging::debug("Camera is not null - we already have a camera -> update it.");

    // update the complete camera and place it into the scene
    Logging::debug(MString("Creating camera shape: ") + cam->shortName);
    float horizontalFilmAperture = 24.892f;
    float verticalFilmAperture = 18.669f;
    int width, height;
    renderGlobals->getWidthHeight(width, height);
    float imageAspect = (float)width / (float)height;
    bool dof = renderGlobals->doDof;
    float mtap_cameraType = 0;
    int mtap_diaphragm_blades = 0;
    float mtap_diaphragm_tilt_angle = 0.0;
    float focusDistance = 0.0;
    float fStop = 0.0;

    float focalLength = 35.0f;
    MFnCamera camFn(cam->mobject, &stat);
    asr::ParamArray camParams;

    getFloat(MString("horizontalFilmAperture"), camFn, horizontalFilmAperture);
    getFloat(MString("verticalFilmAperture"), camFn, verticalFilmAperture);
    getFloat(MString("focalLength"), camFn, focalLength);
    getBool(MString("depthOfField"), camFn, dof);
    getFloat(MString("focusDistance"), camFn, focusDistance);
    getFloat(MString("fStop"), camFn, fStop);
    getInt(MString("mtap_diaphragm_blades"), camFn, mtap_diaphragm_blades);
    getFloat(MString("mtap_diaphragm_tilt_angle"), camFn, mtap_diaphragm_tilt_angle);

    // this is a hack because this camera model does not support NON depth of field
    if (!dof)
        fStop *= 10000.0f;

    focusDistance *= renderGlobals->scaleFactor;

    // maya aperture is given in inces so convert to cm and convert to meters
    horizontalFilmAperture = horizontalFilmAperture * 2.54f * 0.01f;
    verticalFilmAperture = verticalFilmAperture * 2.54f * 0.01f;
    verticalFilmAperture = horizontalFilmAperture / imageAspect;
    MString filmBack = MString("") + horizontalFilmAperture + " " + verticalFilmAperture;
    MString focalLen = MString("") + focalLength * 0.001f;

    camParams.insert("film_dimensions", filmBack.asChar());
    camParams.insert("focal_length", focalLen.asChar());
    camParams.insert("focal_distance", (MString("") + focusDistance).asChar());
    camParams.insert("f_stop", (MString("") + fStop).asChar());
    camParams.insert("diaphragm_blades", (MString("") + mtap_diaphragm_blades).asChar());
    camParams.insert("diaphragm_tilt_angle", (MString("") + mtap_diaphragm_tilt_angle).asChar());

    if (!camera)
    {
        asf::auto_release_ptr<asr::Camera> appleCam = asr::ThinLensCameraFactory().create(
            cam->shortName.asChar(),
            camParams);
        project->get_scene()->set_camera(appleCam);
        camera = project->get_scene()->get_camera();
    }


    fillMatrices(cam, camera->transform_sequence());
}

void AppleseedRenderer::defineCamera()
{
    MStatus stat;
    sharedPtr<MayaScene> mayaScene = MayaTo::getWorldPtr()->worldScenePtr;
    sharedPtr<RenderGlobals> renderGlobals = MayaTo::getWorldPtr()->worldRenderGlobalsPtr;

    std::vector<sharedPtr<MayaObject> >::iterator oIt;
    for (oIt = mayaScene->camList.begin(); oIt != mayaScene->camList.end(); oIt++)
    {
        sharedPtr<MayaObject> cam = *oIt;
        if (!isCameraRenderable(cam->mobject) && (!(cam->dagPath == mayaScene->uiCamera)))
        {
            continue;
        }
        defineCamera(cam);
        //asr::Camera *camera = project->get_scene()->get_camera();
        //if (camera != nullptr)
        //	Logging::debug("Camera is not null - we already have a camera -> update it.");

        //// update the complete camera and place it into the scene
        //Logging::debug(MString("Creating camera shape: ") + cam->shortName);
        //float horizontalFilmAperture = 24.892f;
        //float verticalFilmAperture = 18.669f;
        //int width, height;
        //renderGlobals->getWidthHeight(width, height);
        //float imageAspect = (float)width / (float)height;
        //bool dof = renderGlobals->doDof;
        //float mtap_cameraType = 0;
        //int mtap_diaphragm_blades = 0;
        //float mtap_diaphragm_tilt_angle = 0.0;
        //float focusDistance = 0.0;
        //float fStop = 0.0;

        //float focalLength = 35.0f;
        //MFnCamera camFn(cam->mobject, &stat);
        //asr::ParamArray camParams;

        //getFloat(MString("horizontalFilmAperture"), camFn, horizontalFilmAperture);
        //getFloat(MString("verticalFilmAperture"), camFn, verticalFilmAperture);
        //getFloat(MString("focalLength"), camFn, focalLength);
        //getBool(MString("depthOfField"), camFn, dof);
        //getFloat(MString("focusDistance"), camFn, focusDistance);
        //getFloat(MString("fStop"), camFn, fStop);
        //getInt(MString("mtap_diaphragm_blades"), camFn, mtap_diaphragm_blades);
        //getFloat(MString("mtap_diaphragm_tilt_angle"), camFn, mtap_diaphragm_tilt_angle);

        //// this is a hack because this camera model does not support NON depth of field
        //if( !dof )
        //	fStop *= 10000.0f;

        //focusDistance *= renderGlobals->scaleFactor;

        //// maya aperture is given in inces so convert to cm and convert to meters
        //horizontalFilmAperture = horizontalFilmAperture * 2.54f * 0.01f;
        //verticalFilmAperture = verticalFilmAperture * 2.54f * 0.01f;
        //verticalFilmAperture = horizontalFilmAperture / imageAspect;
        //MString filmBack = MString("") + horizontalFilmAperture + " " + verticalFilmAperture;
        //MString focalLen = MString("") + focalLength * 0.001f;
        //
        //camParams.insert("film_dimensions", filmBack.asChar());
        //camParams.insert("focal_length", focalLen.asChar());
        //camParams.insert("focal_distance", (MString("") + focusDistance).asChar());
        //camParams.insert("f_stop",  (MString("") + fStop).asChar());
        //camParams.insert("diaphragm_blades",  (MString("") + mtap_diaphragm_blades).asChar());
        //camParams.insert("diaphragm_tilt_angle",  (MString("") + mtap_diaphragm_tilt_angle).asChar());

        //asf::auto_release_ptr<asr::Camera> appleCam = asr::ThinLensCameraFactory().create(
        //		cam->shortName.asChar(),
        //		camParams);
        //fillTransformMatrices(cam, appleCam.get());
        //project->get_scene()->set_camera(appleCam);
        break; // only one camera is supported at the moment
    }
}

