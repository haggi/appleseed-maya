#include "appleseed.h"
#include "appleseedutils.h"
#include "appleseedmaterial.h"
#include "shadingtools/material.h"
#include "shadingtools/shaderdefs.h"
#include "shadingtools/shadingutils.h"
#include "renderer/modeling/shadergroup/shadergroup.h"
#include "utilities/logging.h"
#include "utilities/tools.h"
#include "utilities/attrtools.h"
#include "osl/oslutils.h"
#include "maya/mfndependencynode.h"
#include "world.h"
#include "mayascene.h"
#include "threads/renderqueueworker.h"

void AppleRender::updateMaterial(MObject materialNode, const asr::Assembly *assembly)
{
	MAYATO_OSLUTIL::OSLUtilClass OSLShaderClass;
	MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
	MString surfaceShaderName = getObjectName(surfaceShaderNode);
	MString shadingGroupName = getObjectName(materialNode);
	ShadingNetwork network(surfaceShaderNode);
	size_t numNodes = network.shaderList.size();

	MString assName = "swatchRenderer_world";
	if (assName == assembly->get_name())
	{
		shadingGroupName = "previewSG";
	}

	MString shaderGroupName = shadingGroupName + "_OSLShadingGroup";

	asr::ShaderGroup *shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());

	if (shaderGroup != nullptr)
	{
		shaderGroup->clear();
	}
	else{
		asf::auto_release_ptr<asr::ShaderGroup> oslShadingGroup = asr::ShaderGroupFactory().create(shaderGroupName.asChar());
		assembly->shader_groups().insert(oslShadingGroup);
		shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());
	}

	OSLShaderClass.group = (OSL::ShaderGroup *)shaderGroup;

	MFnDependencyNode shadingGroupNode(materialNode);
	MPlug shaderPlug = shadingGroupNode.findPlug("surfaceShader");
	OSLShaderClass.createOSLProjectionNodes(shaderPlug);

	for (int shadingNodeId = 0; shadingNodeId < numNodes; shadingNodeId++)
	{
		ShadingNode snode = network.shaderList[shadingNodeId];
		Logging::debug(MString("ShadingNode Id: ") + shadingNodeId + " ShadingNode name: " + snode.fullName);
		if (shadingNodeId == (numNodes - 1))
			Logging::debug(MString("LastNode Surface Shader: ") + snode.fullName);
		OSLShaderClass.createOSLShadingNode(network.shaderList[shadingNodeId]);
		//OSLShaderClass.connectProjectionNodes(network.shaderList[shadingNodeId].mobject);
	}

	OSLShaderClass.cleanupShadingNodeList();
	OSLShaderClass.createAndConnectShaderNodes();

	if (numNodes > 0)
	{
		ShadingNode snode = network.shaderList[numNodes - 1];
		MString layer = (snode.fullName + "_interface");
		Logging::debug(MString("Adding interface shader: ") + layer);
		asr::ShaderGroup *sg = (asr::ShaderGroup *)OSLShaderClass.group;
		sg->add_shader("surface", "surfaceShaderInterface", layer.asChar(), asr::ParamArray());
		const char *srcLayer = snode.fullName.asChar();
		const char *srcAttr = "outColor";
		const char *dstLayer = layer.asChar();
		const char *dstAttr = "inColor";
		Logging::debug(MString("Connecting interface shader: ") + srcLayer + "." + srcAttr + " -> " + dstLayer + "." + dstAttr);
		sg->add_connection(srcLayer, srcAttr, dstLayer, dstAttr);
	}

	MString physicalSurfaceName = shadingGroupName + "_physical_surface_shader";

	// add shaders only if they do not yet exist
	if (assembly->surface_shaders().get_by_name(physicalSurfaceName.asChar()) == nullptr)
	{
		assembly->surface_shaders().insert(
			asr::PhysicalSurfaceShaderFactory().create(
			physicalSurfaceName.asChar(),
			asr::ParamArray()));
	}
	if (assembly->materials().get_by_name(shadingGroupName.asChar()) == nullptr)
	{
		assembly->materials().insert(
			asr::OSLMaterialFactory().create(
			shadingGroupName.asChar(),
			asr::ParamArray()
			.insert("surface_shader", physicalSurfaceName.asChar())
			.insert("osl_surface", shaderGroupName.asChar())));
	}
}

void AppleRender::AppleseedRenderer::updateMaterial(MObject materialNode)
{
	MAYATO_OSLUTIL::OSLUtilClass OSLShaderClass;
	MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
	MString surfaceShaderName = getObjectName(surfaceShaderNode);
	MString shadingGroupName = getObjectName(materialNode);
	MString shaderGroupName = shadingGroupName + "_OSLShadingGroup";
	ShadingNetwork network(surfaceShaderNode);
	size_t numNodes = network.shaderList.size();
	asr::Assembly *assembly = getMasterAssemblyFromProject(this->project.get());
	asr::ShaderGroup *shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());

	if (shaderGroup != nullptr)
	{
		shaderGroup->clear();
	}
	else{
		asf::auto_release_ptr<asr::ShaderGroup> oslShadingGroup = asr::ShaderGroupFactory().create(shaderGroupName.asChar());
		assembly->shader_groups().insert(oslShadingGroup);
		shaderGroup = assembly->shader_groups().get_by_name(shaderGroupName.asChar());
	}

	OSLShaderClass.group = (OSL::ShaderGroup *)shaderGroup;

	MFnDependencyNode shadingGroupNode(materialNode);
	MPlug shaderPlug = shadingGroupNode.findPlug("surfaceShader");
	OSLShaderClass.createOSLProjectionNodes(shaderPlug);

	for (int shadingNodeId = 0; shadingNodeId < numNodes; shadingNodeId++)
	{
		ShadingNode snode = network.shaderList[shadingNodeId];
		Logging::debug(MString("ShadingNode Id: ") + shadingNodeId + " ShadingNode name: " + snode.fullName);
		if (shadingNodeId == (numNodes - 1))
			Logging::debug(MString("LastNode Surface Shader: ") + snode.fullName);
		OSLShaderClass.createOSLShadingNode(network.shaderList[shadingNodeId]);
		//OSLShaderClass.connectProjectionNodes(network.shaderList[shadingNodeId].mobject);
	}

	OSLShaderClass.cleanupShadingNodeList();
	OSLShaderClass.createAndConnectShaderNodes();

	if (numNodes > 0)
	{
		ShadingNode snode = network.shaderList[numNodes - 1];
		MString layer = (snode.fullName + "_interface");
		Logging::debug(MString("Adding interface shader: ") + layer);
		asr::ShaderGroup *sg = (asr::ShaderGroup *)OSLShaderClass.group;
		sg->add_shader("surface", "surfaceShaderInterface", layer.asChar(), asr::ParamArray());
		const char *srcLayer = snode.fullName.asChar();
		const char *srcAttr = "outColor";
		const char *dstLayer = layer.asChar();
		const char *dstAttr = "inColor";
		Logging::debug(MString("Connecting interface shader: ") + srcLayer + "." + srcAttr + " -> " + dstLayer + "." + dstAttr);
		sg->add_connection(srcLayer, srcAttr, dstLayer, dstAttr);
	}


	MString physicalSurfaceName = shadingGroupName + "_physical_surface_shader";
	// add shaders only if they do not yet exist
	if (assembly->surface_shaders().get_by_name(physicalSurfaceName.asChar()) == nullptr)
	{
		assembly->surface_shaders().insert(
			asr::PhysicalSurfaceShaderFactory().create(
			physicalSurfaceName.asChar(),
			asr::ParamArray()));
	}
	if (assembly->materials().get_by_name(shadingGroupName.asChar()) == nullptr)
	{
		assembly->materials().insert(
			asr::OSLMaterialFactory().create(
			shadingGroupName.asChar(),
			asr::ParamArray()
			.insert("surface_shader", physicalSurfaceName.asChar())
			.insert("osl_surface", shaderGroupName.asChar())));
	}
}


asf::StringArray AppleRender::AppleseedRenderer::defineMaterial(std::shared_ptr<mtap_MayaObject> obj)
{
	MStatus status;
	asf::StringArray materialNames;
	getObjectShadingGroups(obj->dagPath, obj->perFaceAssignments, obj->shadingGroups, false);
	std::shared_ptr<MayaScene> mayaScene = MayaTo::getWorldPtr()->worldScenePtr;

	for (uint sgId = 0; sgId < obj->shadingGroups.length(); sgId++)
	{
		MObject materialNode = obj->shadingGroups[sgId];
		MString surfaceShader;
		MObject surfaceShaderNode = getConnectedInNode(materialNode, "surfaceShader");
		MString surfaceShaderName = getObjectName(surfaceShaderNode);
		MFnDependencyNode depFn(surfaceShaderNode);
		MString typeName = depFn.typeName();

		// if the connected surface shader is not supported, use a default material because otherwise osl will crash
		if (!ShaderDefinitions::shadingNodeSupported(typeName))
		{
			Logging::warning(MString("Surface shader type: ") + typeName + " is not supported, using default material.");
			MString objectInstanceName = getObjectInstanceName(obj.get());
			asr::Assembly *ass = getCreateObjectAssembly(obj);
			MString shadingGroupName = getObjectName(materialNode);
			asr::ObjectInstance *objInstance = ass->object_instances().get_by_name(objectInstanceName.asChar());
			objInstance->get_front_material_mappings().insert("slot0", "default");
			objInstance->get_back_material_mappings().insert("slot0", "default");
			continue;
		}
		
		// if we are in IPR mode, save all translated shading nodes to the interactive update list
		if (MayaTo::getWorldPtr()->renderType == MayaTo::MayaToWorld::WorldRenderType::IPRRENDER)
		{
			if (mayaScene)
			{
				InteractiveElement iel;
				iel.mobj = surfaceShaderNode;
				iel.obj = obj;
				iel.name = surfaceShaderName;
				iel.node = materialNode;
				mayaScene->interactiveUpdateMap[mayaScene->interactiveUpdateMap.size()] = iel;

				if (MayaTo::getWorldPtr()->renderState == MayaTo::MayaToWorld::WorldRenderState::RSTATERENDERING)
				{
					RenderQueueWorker::IPRUpdateCallbacks();
				}
			}
		}

		updateMaterial(materialNode);		

		MString objectInstanceName = getObjectInstanceName(obj.get());
		asr::Assembly *ass = getCreateObjectAssembly(obj);
		MString shadingGroupName = getObjectName(materialNode);
		asr::ObjectInstance *objInstance = ass->object_instances().get_by_name(objectInstanceName.asChar());
		objInstance->get_front_material_mappings().insert("slot0", shadingGroupName.asChar());
		objInstance->get_back_material_mappings().insert("slot0", shadingGroupName.asChar());
	}

	return materialNames;

}
