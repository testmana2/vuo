/**
 * @file
 * VuoSceneObject implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "type.h"
#include "VuoSceneObject.h"
#include "VuoList_VuoImage.h"
#include "VuoList_VuoSceneObject.h"
#include "VuoBoolean.h"

/// @{
#ifdef VUO_COMPILER
VuoModuleMetadata({
					 "title" : "Scene Object",
					 "description" : "A 3D Object: visible (mesh), or virtual (group, light, camera).",
					 "keywords" : [ ],
					 "version" : "1.0.0",
					 "dependencies" : [
						 "c",
						 "json"
					 ]
				 });
#endif
/// @}


/**
 * Creates a new, empty scene object.
 */
VuoSceneObject VuoSceneObject_makeEmpty(void)
{
	VuoSceneObject o;

	o.mesh = NULL;
	o.shader = NULL;
	o.isRealSize = false;

	o.childObjects = NULL;

	o.cameraType = VuoSceneObject_NotACamera;

	o.lightType = VuoSceneObject_NotALight;

	o.name = NULL;
	o.transform = VuoTransform_valueFromJson(NULL);

	return o;
}

/**
 * Creates a visible (mesh) scene object.
 */
VuoSceneObject VuoSceneObject_make(VuoMesh mesh, VuoShader shader, VuoTransform transform, VuoList_VuoSceneObject childObjects)
{
	VuoSceneObject o;

	o.mesh = mesh;
	o.shader = shader;
	o.isRealSize = false;

	o.childObjects = childObjects;

	o.cameraType = VuoSceneObject_NotACamera;

	o.lightType = VuoSceneObject_NotALight;

	o.name = NULL;
	o.transform = transform;

	return o;
}

/**
 * Returns a scene object that renders a quad with the specified shader.
 *
 * The quad does not include normals, tangents, or bitangents.
 *
 * @param shader The shader used to render the object.
 * @param center The object's center, specified in scene coordinates.
 * @param rotation The object's rotation, specified in degrees.
 * @param width The object's width, specified in scene coordinates.
 * @param height The object's height, specified in scene coordinates.
 * @return The quad scene object.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeQuad(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height)
{
	return VuoSceneObject_make(
				VuoMesh_makeQuadWithoutNormals(),
				shader,
				VuoTransform_makeEuler(
					center,
					VuoPoint3d_multiply(rotation, M_PI/180.),
					VuoPoint3d_make(width,height,1)
				),
				NULL
			);
}

/**
 * Returns a scene object that renders a quad with the specified shader.
 *
 * The quad includes normals, tangents, or bitangents.
 *
 * @param shader The shader used to render the object.
 * @param center The object's center, specified in scene coordinates.
 * @param rotation The object's rotation, specified in degrees.
 * @param width The object's width, specified in scene coordinates.
 * @param height The object's height, specified in scene coordinates.
 * @return The quad scene object.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeQuadWithNormals(VuoShader shader, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal height)
{
	return VuoSceneObject_make(
				VuoMesh_makeQuad(),
				shader,
				VuoTransform_makeEuler(
					center,
					VuoPoint3d_multiply(rotation, M_PI/180.),
					VuoPoint3d_make(width,height,1)
				),
				NULL
			);
}

/**
 * Returns an unlit scene object with the specified @c image.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha)
{
	if (!image)
		return VuoSceneObject_makeEmpty();

	VuoSceneObject object = VuoSceneObject_makeQuad(
				VuoShader_makeUnlitImageShader(image, alpha),
				center,
				rotation,
				width,
				image->pixelsHigh * width/image->pixelsWide
			);

	return object;
}

/**
 * Returns a lit scene object with the specified @c image.
 *
 * @threadAnyGL
 */
VuoSceneObject VuoSceneObject_makeLitImage(VuoImage image, VuoPoint3d center, VuoPoint3d rotation, VuoReal width, VuoReal alpha, VuoColor highlightColor, VuoReal shininess)
{
	if (!image)
		return VuoSceneObject_makeEmpty();

	return VuoSceneObject_makeQuadWithNormals(
				VuoShader_makeLitImageShader(image, alpha, highlightColor, shininess),
				center,
				rotation,
				width,
				image->pixelsHigh * width/image->pixelsWide
			);
}

/**
 * Returns a scene object consisting of 6 child objects (square quads), each with its own shader.
 */
VuoSceneObject VuoSceneObject_makeCube(VuoTransform transform, VuoShader frontShader, VuoShader leftShader, VuoShader rightShader, VuoShader backShader, VuoShader topShader, VuoShader bottomShader)
{
	VuoList_VuoSceneObject cubeChildObjects = VuoListCreate_VuoSceneObject();

	VuoMesh quadMesh = VuoMesh_makeQuad();

	// Front Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					frontShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,0,.5), VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Left Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					leftShader,
					VuoTransform_makeEuler(VuoPoint3d_make(-.5,0,0), VuoPoint3d_make(0,-M_PI/2.,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Right Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					rightShader,
					VuoTransform_makeEuler(VuoPoint3d_make(.5,0,0), VuoPoint3d_make(0,M_PI/2.,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Back Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					backShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,0,-.5), VuoPoint3d_make(0,M_PI,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Top Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					topShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,.5,0), VuoPoint3d_make(-M_PI/2.,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	// Bottom Face
	{
		VuoSceneObject so = VuoSceneObject_make(
					quadMesh,
					bottomShader,
					VuoTransform_makeEuler(VuoPoint3d_make(0,-.5,0), VuoPoint3d_make(M_PI/2.,0,0), VuoPoint3d_make(1,1,1)),
					NULL
					);
		VuoListAppendValue_VuoSceneObject(cubeChildObjects, so);
	}

	return VuoSceneObject_make(NULL, NULL, transform, cubeChildObjects);
}

/**
 * Returns a perspective camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makePerspectiveCamera(VuoText name, VuoTransform transform, float fieldOfView, float distanceMin, float distanceMax)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.name = name;
	o.transform = transform;
	o.cameraType = VuoSceneObject_PerspectiveCamera;
	o.cameraFieldOfView = fieldOfView;
	o.cameraDistanceMin = distanceMin;
	o.cameraDistanceMax = distanceMax;
	return o;
}

/**
 * Returns a stereoscopic camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makeStereoCamera(VuoText name, VuoTransform transform, VuoReal fieldOfView, VuoReal distanceMin, VuoReal distanceMax, VuoReal confocalDistance, VuoReal intraocularDistance)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.name = name;
	o.transform = transform;
	o.cameraType = VuoSceneObject_StereoCamera;
	o.cameraFieldOfView = fieldOfView;
	o.cameraDistanceMin = distanceMin;
	o.cameraDistanceMax = distanceMax;
	o.cameraConfocalDistance = confocalDistance;
	o.cameraIntraocularDistance = intraocularDistance;
	return o;
}

/**
 * Returns an orthographic camera having the position and negative-rotation specified by @c transform (its scale is ignored).
 */
VuoSceneObject VuoSceneObject_makeOrthographicCamera(VuoText name, VuoTransform transform, float width, float distanceMin, float distanceMax)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.name = name;
	o.transform = transform;
	o.cameraType = VuoSceneObject_OrthographicCamera;
	o.cameraWidth = width;
	o.cameraDistanceMin = distanceMin;
	o.cameraDistanceMax = distanceMax;
	return o;
}

/**
 * Returns a perspective camera at (0,0,1), facing along -z, 90 degree FOV, and clip planes at 0.1 and 10.0.
 */
VuoSceneObject VuoSceneObject_makeDefaultCamera(void)
{
	VuoTransform transform = VuoTransform_makeEuler(
				VuoPoint3d_make(0,0,1),
				VuoPoint3d_make(0,0,0),
				VuoPoint3d_make(1,1,1)
			);
	return VuoSceneObject_makePerspectiveCamera(
				VuoText_make("default camera"),
				transform,
				90,
				0.1,
				10.0
				);
}

/**
 * Searches the scenegraph (depth-first) for a scene object with the given name.
 *
 * @param so The root object of the scenegraph to search.
 * @param nameToMatch The name to search for.
 * @param[out] ancestorObjects The ancestors of @a foundObject, starting with the root of the scenegraph.
 * @param[out] foundObject The first matching scene object found.
 * @return True if a matching scene object was found.
 */
bool VuoSceneObject_find(VuoSceneObject so, VuoText nameToMatch, VuoList_VuoSceneObject ancestorObjects, VuoSceneObject *foundObject)
{
	if (VuoText_areEqual(so.name, nameToMatch))
	{
		*foundObject = so;
		return true;
	}

	VuoListAppendValue_VuoSceneObject(ancestorObjects, so);

	unsigned long childObjectCount = (so.childObjects ? VuoListGetCount_VuoSceneObject(so.childObjects) : 0);
	for (unsigned long i = 1; i <= childObjectCount; ++i)
	{
		VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
		if (VuoSceneObject_find(childObject, nameToMatch, ancestorObjects, foundObject))
			return true;
	}

	VuoListRemoveLastValue_VuoSceneObject(ancestorObjects);

	return false;
}

/**
 * Helper for @ref VuoSceneObject_apply.
 */
VuoSceneObject VuoSceneObject_findCameraInternal(VuoSceneObject so, VuoText nameToMatch, bool *foundCamera, float modelviewMatrix[16])
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(so.transform, localModelviewMatrix);
	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);

	if (so.cameraType != VuoSceneObject_NotACamera && strstr(so.name,nameToMatch))
	{
		*foundCamera = true;
		so.transform = VuoTransform_makeFromMatrix4x4(compositeModelviewMatrix);
		return so;
	}

	if (so.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
			bool foundChildCamera;
			VuoSceneObject childCamera = VuoSceneObject_findCameraInternal(childObject, nameToMatch, &foundChildCamera, compositeModelviewMatrix);
			if (foundChildCamera)
			{
				*foundCamera = true;
				return childCamera;
			}
			else
			{
				VuoSceneObject_retain(childCamera);
				VuoSceneObject_release(childCamera);
			}
		}
	}

	*foundCamera = false;
	return VuoSceneObject_makeDefaultCamera();
}

/**
 * Performs a depth-first search of the scenegraph.
 * Returns the first camera whose name contains @c nameToMatch (or, if @c nameToMatch is emptystring, just returns the first camera),
 * with its transform altered to incorporate the transforms of its ancestor objects.
 * Output paramater @c foundCamera indicates whether a camera was found.
 * If no camera was found, returns VuoSceneObject_makeDefaultCamera().
 */
VuoSceneObject VuoSceneObject_findCamera(VuoSceneObject so, VuoText nameToMatch, bool *foundCamera)
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);

	return VuoSceneObject_findCameraInternal(so, nameToMatch, foundCamera, localModelviewMatrix);
}

/**
 * @ingroup VuoSceneObject
 * Returns the @c VuoSceneObject_CameraType corresponding with the string @c cameraTypeString.  If none matches, returns VuoSceneObject_NotACamera.
 */
VuoSceneObject_CameraType VuoSceneObject_cameraTypeFromCString(const char *cameraTypeString)
{
	if (strcmp(cameraTypeString,"perspective")==0)
		return VuoSceneObject_PerspectiveCamera;
	else if (strcmp(cameraTypeString,"stereo")==0)
		return VuoSceneObject_StereoCamera;
	else if (strcmp(cameraTypeString,"orthographic")==0)
		return VuoSceneObject_OrthographicCamera;

	return VuoSceneObject_NotACamera;
}

/**
 * @ingroup VuoSceneObject
 * Returns a string constant representing @c cameraType.
 */
const char * VuoSceneObject_cStringForCameraType(VuoSceneObject_CameraType cameraType)
{
	switch (cameraType)
	{
		case VuoSceneObject_PerspectiveCamera:
			return "perspective";
		case VuoSceneObject_StereoCamera:
			return "stereo";
		case VuoSceneObject_OrthographicCamera:
			return "orthographic";
		default:
			return "notACamera";
	}
}

/**
 * @ingroup VuoSceneObject
 * Returns an ambient light with the specified @c color and @c brightness (typically between 0 and 1).
 */
VuoSceneObject VuoSceneObject_makeAmbientLight(VuoColor color, float brightness)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.lightType = VuoSceneObject_AmbientLight;
	o.lightColor = color;
	o.lightBrightness = brightness;
	return o;
}

/**
 * @ingroup VuoSceneObject
 * Returns a point light (uniform emission in all directions).
 *
 * @param color The light's color.
 * @param brightness The light's brightness multiplier (typically between 0 and 1).
 * @param position The light's position.
 * @param range The distance (in local coordinates) the light reaches.
 * @param sharpness The sharpness of the light's distance falloff.  0 means the light starts fading at distance 0 and ends at 2*lightRange.  1 means the falloff is instant.
 */
VuoSceneObject VuoSceneObject_makePointLight(VuoColor color, float brightness, VuoPoint3d position, float range, float sharpness)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.lightType = VuoSceneObject_PointLight;
	o.lightColor = color;
	o.lightBrightness = brightness;
	o.lightRange = range;
	o.lightSharpness = MAX(MIN(sharpness,1),0);
	o.transform = VuoTransform_makeEuler(position, VuoPoint3d_make(0,0,0), VuoPoint3d_make(1,1,1));
	return o;
}

/**
 * @ingroup VuoSceneObject
 * Returns a spot light (emists only in the specified direction).
 *
 * @param color The light's color.
 * @param brightness The light's brightness multiplier (typically between 0 and 1).
 * @param transform The position and direction of the light.  (The transform's scale is ignored.)
 * @param cone Width (in radians) of the light's cone.
 * @param range The distance (in local coordinates) the light reaches.
 * @param sharpness The sharpness of the light's distance/cone falloff.  0 means the light starts fading at distance/angle 0 and ends at 2*range or 2*cone.  1 means the falloff is instant.
 */
VuoSceneObject VuoSceneObject_makeSpotlight(VuoColor color, float brightness, VuoTransform transform, float cone, float range, float sharpness)
{
	VuoSceneObject o = VuoSceneObject_makeEmpty();
	o.lightType = VuoSceneObject_Spotlight;
	o.lightColor = color;
	o.lightBrightness = brightness;
	o.lightCone = cone;
	o.lightRange = range;
	o.lightSharpness = MAX(MIN(sharpness,1),0);
	o.transform = transform;
	return o;
}

/**
 * @ingroup VuoSceneObject
 * Returns the @c VuoSceneObject_LightType corresponding with the string @c lightTypeString.  If none matches, returns VuoSceneObject_NotALight.
 */
VuoSceneObject_LightType VuoSceneObject_lightTypeFromCString(const char *lightTypeString)
{
	if (strcmp(lightTypeString,"ambient")==0)
		return VuoSceneObject_AmbientLight;
	else if (strcmp(lightTypeString,"point")==0)
		return VuoSceneObject_PointLight;
	else if (strcmp(lightTypeString,"spot")==0)
		return VuoSceneObject_Spotlight;

	return VuoSceneObject_NotALight;
}

/**
 * @ingroup VuoSceneObject
 * Returns a string constant representing @c lightType.
 */
const char * VuoSceneObject_cStringForLightType(VuoSceneObject_LightType lightType)
{
	switch (lightType)
	{
		case VuoSceneObject_AmbientLight:
			return "ambient";
		case VuoSceneObject_PointLight:
			return "point";
		case VuoSceneObject_Spotlight:
			return "spot";
		default:
			return "notALight";
	}
}

/**
 * Helper for @ref VuoSceneObject_findLights.
 */
static void VuoSceneObject_findLightsRecursive(VuoSceneObject so, float modelviewMatrix[16], VuoList_VuoColor ambientColors, float *ambientBrightness, VuoList_VuoSceneObject pointLights, VuoList_VuoSceneObject spotLights)
{
	switch (so.lightType)
	{
		case VuoSceneObject_AmbientLight:
			VuoListAppendValue_VuoColor(ambientColors, so.lightColor);
			*ambientBrightness += so.lightBrightness;
			return;
		case VuoSceneObject_PointLight:
		{
			float localModelviewMatrix[16];
			VuoTransform_getMatrix(so.transform, localModelviewMatrix);
			float compositeModelviewMatrix[16];
			VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);
			so.transform = VuoTransform_makeFromMatrix4x4(compositeModelviewMatrix);

			VuoListAppendValue_VuoSceneObject(pointLights, so);
			return;
		}
		case VuoSceneObject_Spotlight:
		{
			float localModelviewMatrix[16];
			VuoTransform_getMatrix(so.transform, localModelviewMatrix);
			float compositeModelviewMatrix[16];
			VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);
			so.transform = VuoTransform_makeFromMatrix4x4(compositeModelviewMatrix);

			VuoListAppendValue_VuoSceneObject(spotLights, so);
			return;
		}
		default:
			break;
	}

	if (so.childObjects)
	{
		float localModelviewMatrix[16];
		VuoTransform_getMatrix(so.transform, localModelviewMatrix);
		float compositeModelviewMatrix[16];
		VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);

		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject childObject = VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i);
			VuoSceneObject_findLightsRecursive(childObject, compositeModelviewMatrix, ambientColors, ambientBrightness, pointLights, spotLights);
		}
	}
}

/**
 * Finds and returns all the lights in the scene.
 *
 * If there are multiple ambient lights, returns the weighted (by alpha) average color and summed brightness.
 *
 * If there are no lights in the scene, returns some default lights.
 */
void VuoSceneObject_findLights(VuoSceneObject so, VuoColor *ambientColor, float *ambientBrightness, VuoList_VuoSceneObject *pointLights, VuoList_VuoSceneObject *spotLights)
{
	VuoList_VuoColor ambientColors = VuoListCreate_VuoColor();
	VuoRetain(ambientColors);

	*ambientBrightness = 0;
	*pointLights = VuoListCreate_VuoSceneObject();
	*spotLights = VuoListCreate_VuoSceneObject();

	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);

	VuoSceneObject_findLightsRecursive(so, localModelviewMatrix, ambientColors, ambientBrightness, *pointLights, *spotLights);

	if (!VuoListGetCount_VuoColor(ambientColors)
			&& !VuoListGetCount_VuoSceneObject(*pointLights)
			&& !VuoListGetCount_VuoSceneObject(*spotLights))
	{
		*ambientColor = VuoColor_makeWithRGBA(1,1,1,1);
		*ambientBrightness = 0.05;

		// https://en.wikipedia.org/wiki/Three-point_lighting

		VuoSceneObject keyLight = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .70, VuoPoint3d_make(-1,1,1), 5, .5);
		VuoListAppendValue_VuoSceneObject(*pointLights, keyLight);

		VuoSceneObject fillLight = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .2, VuoPoint3d_make(.5,0,1), 5, 0);
		VuoListAppendValue_VuoSceneObject(*pointLights, fillLight);

		VuoSceneObject backLight = VuoSceneObject_makePointLight(VuoColor_makeWithRGBA(1,1,1,1), .15, VuoPoint3d_make(1,.75,-.5), 5, 0);
		VuoListAppendValue_VuoSceneObject(*pointLights, backLight);
	}
	else
		*ambientColor = VuoColor_average(ambientColors);

	VuoRelease(ambientColors);
}

/**
 * Applies @c function to @c object and its child objects,
 * without preserving changes to objects.
 */
void VuoSceneObject_visit(const VuoSceneObject object, void (^function)(const VuoSceneObject currentObject))
{
	function(object);

	if (object.childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(object.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject o = VuoListGetValueAtIndex_VuoSceneObject(object.childObjects, i);
			VuoSceneObject_visit(o, function);
		}
	}
}

/**
 * Helper for @ref VuoSceneObject_apply.
 */
void VuoSceneObject_applyInternal(VuoSceneObject *object, void (^function)(VuoSceneObject *currentObject, float modelviewMatrix[16]), float modelviewMatrix[16])
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(object->transform, localModelviewMatrix);
	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, modelviewMatrix, compositeModelviewMatrix);

	function(object, compositeModelviewMatrix);

	if (object->childObjects)
	{
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(object->childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
		{
			VuoSceneObject o = VuoListGetValueAtIndex_VuoSceneObject(object->childObjects, i);
			VuoSceneObject_applyInternal(&o, function, compositeModelviewMatrix);
			VuoListSetValueAtIndex_VuoSceneObject(object->childObjects, o, i);
		}
	}
}

/**
 * Applies @c function to @c object and its child objects,
 * and outputs the modified @c object.
 *
 * The value `modelviewMatrix` (which `VuoSceneObject_apply` passes to `function`)
 * is the cumulative transformation matrix (from `object` down to the `currentObject`).
 */
void VuoSceneObject_apply(VuoSceneObject *object, void (^function)(VuoSceneObject *currentObject, float modelviewMatrix[16]))
{
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(VuoTransform_makeIdentity(), localModelviewMatrix);

	VuoSceneObject_applyInternal(object, function, localModelviewMatrix);
}

/**
 * Sets the @c faceCullingMode on @c object and its child objects.
 *
 * @c faceCullingMode can be @c GL_NONE (show both front and back faces),
 * @c GL_BACK (show only front faces),
 * or @c GL_FRONT (show only back faces).
 */
void VuoSceneObject_setFaceCullingMode(VuoSceneObject *object, unsigned int faceCullingMode)
{
	VuoSceneObject_apply(object, ^(VuoSceneObject *currentObject, float modelviewMatrix[16]){
							 if (currentObject->mesh)
								 for (unsigned long i = 0; i < currentObject->mesh->submeshCount; ++i)
									 currentObject->mesh->submeshes[i].faceCullingMode = faceCullingMode;
						 });
}

/**
 * Makes a deep copy of @c object.
 * Each mesh is copied (see @ref VuoMesh_copy),
 * and each child object is copied.
 *
 * You can change attributes on the copy without affecting the original.
 *
 * @todo The shaders are not copied, so changes to the copy's shaders will affect both the original and the copy.
 */
VuoSceneObject VuoSceneObject_copy(const VuoSceneObject object)
{
	VuoSceneObject copiedObject = object;

	copiedObject.mesh = VuoMesh_copy(object.mesh);

	if (object.childObjects)
	{
		copiedObject.childObjects = VuoListCreate_VuoSceneObject();
		unsigned long childObjectCount = VuoListGetCount_VuoSceneObject(object.childObjects);
		for (unsigned long i = 1; i <= childObjectCount; ++i)
			VuoListAppendValue_VuoSceneObject(copiedObject.childObjects, VuoSceneObject_copy(VuoListGetValueAtIndex_VuoSceneObject(object.childObjects, i)));
	}

	copiedObject.name = VuoText_make(object.name);

	return copiedObject;
}

/**
 * Returns the bounds of a sceneobject including it's children.
 */
bool VuoSceneObject_boundsRecursive(const VuoSceneObject so, VuoBox *bounds, float matrix[16])
{
	// matrix parameter is the trickle down transformations
	float localModelviewMatrix[16];
	VuoTransform_getMatrix(so.transform, localModelviewMatrix);

	float compositeModelviewMatrix[16];
	VuoTransform_multiplyMatrices4x4(localModelviewMatrix, matrix, compositeModelviewMatrix);

	bool foundBounds = VuoSceneObject_meshBounds(so, bounds, compositeModelviewMatrix);

	if(so.childObjects != NULL)
	{
		for(int i = 0; i < VuoListGetCount_VuoSceneObject(so.childObjects); i++)
		{
			VuoBox childBounds;
			bool childBoundsFound = VuoSceneObject_boundsRecursive(VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i+1), &childBounds, compositeModelviewMatrix);

			if(childBoundsFound)
			{
				*bounds = foundBounds ? VuoBox_encapsulate(*bounds, childBounds) : childBounds;
				foundBounds = true;
			}
		}
	}

	return foundBounds;
}

/**
  *	Get the axis aligned bounding box of this sceneobject and it's children (and it's children's children).
  */
VuoBox VuoSceneObject_bounds(const VuoSceneObject so)
{
	VuoBox bounds;

	float identity[16] =
	{
		1, 0, 0, 0,
		0, 1, 0, 0,
		0, 0, 1, 0,
		0, 0, 0, 1
	};

	bool success = VuoSceneObject_boundsRecursive(so, &bounds, identity);

	if(success)
		return bounds;
	else
		return VuoBox_make( (VuoPoint3d){0,0,0}, (VuoPoint3d){0,0,0} );
}

/**
 *	Bounding box of the vertices for this SceneObject (taking into account transform).
 */
bool VuoSceneObject_meshBounds(const VuoSceneObject so, VuoBox *bounds, float matrix[16])
{
	if (VuoSceneObject_getVertexCount(so) < 1)
		return false;

	if (so.isRealSize)
		// We don't know what the actual rendered size of the realSize layer will be,
		// but we can at least include its center point.
		*bounds = VuoBox_make(VuoPoint3d_make(matrix[12], matrix[13], matrix[14]), VuoPoint3d_make(0,0,0));
	else
		*bounds = VuoMesh_bounds(so.mesh, matrix);

	return true;
}

/**
 *	Make the bounds center of all vertices == {0,0,0}
 */
void VuoSceneObject_center(VuoSceneObject *so)
{
	VuoBox bounds = VuoSceneObject_bounds(*so);
	so->transform.translation = VuoPoint3d_subtract(so->transform.translation, bounds.center);
}

/**
 * Change the root sceneobject's transform such that the entire scenegraph renders within a 1x1x1 axis-aligned cube.
 * If the scenegraph has zero size (e.g., if it is empty, or if it consists entirely of Real Size Layers),
 * the transform is left unchanged.
 */
void VuoSceneObject_normalize(VuoSceneObject *so)
{
	VuoBox bounds = VuoSceneObject_bounds(*so);

	float scale = fmax(fmax(bounds.size.x, bounds.size.y), bounds.size.z);
	if (fabs(scale) < 0.00001)
		return;

	so->transform.scale       = VuoPoint3d_multiply(so->transform.scale,       1./scale);
	so->transform.translation = VuoPoint3d_multiply(so->transform.translation, 1./scale);
}
/**
 * @ingroup VuoSceneObject
 * Decodes the JSON object @c js to create a new value.
 *
 * @eg{
 *   {
 *     "mesh" : ... ,
 *     "shader" : ... ,
 *     "isRealSize" : false,
 *     "childObjects" : ...,
 *     "transform" : ...
 *   }
 * }
 *
 * @eg{
 *   {
 *     "cameraType" : "perspective",
 *     "cameraFieldOfView" : 90.0,
 *     "cameraDistanceMin" : 0.1,
 *     "cameraDistanceMax" : 10.0,
 *     "name" : ...,
 *     "transform" : ...
 *   }
 * }
 */
VuoSceneObject VuoSceneObject_valueFromJson(json_object * js)
{
	json_object *o = NULL;

	VuoMesh mesh = NULL;
	if (json_object_object_get_ex(js, "mesh", &o))
		mesh = VuoMesh_valueFromJson(o);

	VuoShader shader = NULL;
	if (json_object_object_get_ex(js, "shader", &o))
		shader = VuoShader_valueFromJson(o);

	bool isRealSize = false;
	if (json_object_object_get_ex(js, "isRealSize", &o))
		isRealSize = VuoBoolean_valueFromJson(o);

	VuoList_VuoSceneObject childObjects = NULL;
	if (json_object_object_get_ex(js, "childObjects", &o))
		childObjects = VuoList_VuoSceneObject_valueFromJson(o);

	VuoSceneObject_CameraType cameraType = VuoSceneObject_NotACamera;
	if (json_object_object_get_ex(js, "cameraType", &o))
		cameraType = VuoSceneObject_cameraTypeFromCString(json_object_get_string(o));

	float cameraFieldOfView;
	if (json_object_object_get_ex(js, "cameraFieldOfView", &o))
		cameraFieldOfView = json_object_get_double(o);

	float cameraWidth;
	if (json_object_object_get_ex(js, "cameraWidth", &o))
		cameraWidth = json_object_get_double(o);

	float cameraDistanceMin;
	if (json_object_object_get_ex(js, "cameraDistanceMin", &o))
		cameraDistanceMin = json_object_get_double(o);

	float cameraDistanceMax;
	if (json_object_object_get_ex(js, "cameraDistanceMax", &o))
		cameraDistanceMax = json_object_get_double(o);

	float cameraConfocalDistance;
	if (json_object_object_get_ex(js, "cameraConfocalDistance", &o))
		cameraConfocalDistance = json_object_get_double(o);

	float cameraIntraocularDistance;
	if (json_object_object_get_ex(js, "cameraIntraocularDistance", &o))
		cameraIntraocularDistance = json_object_get_double(o);

	VuoSceneObject_LightType lightType = VuoSceneObject_NotALight;
	if (json_object_object_get_ex(js, "lightType", &o))
		lightType = VuoSceneObject_lightTypeFromCString(json_object_get_string(o));

	VuoColor lightColor;
	if (json_object_object_get_ex(js, "lightColor", &o))
		lightColor = VuoColor_valueFromJson(o);

	float lightBrightness;
	if (json_object_object_get_ex(js, "lightBrightness", &o))
		lightBrightness = json_object_get_double(o);

	float lightCone;
	if (json_object_object_get_ex(js, "lightCone", &o))
		lightCone = json_object_get_double(o);

	float lightRange;
	if (json_object_object_get_ex(js, "lightRange", &o))
		lightRange = json_object_get_double(o);

	float lightSharpness;
	if (json_object_object_get_ex(js, "lightSharpness", &o))
		lightSharpness = json_object_get_double(o);

	VuoText name = NULL;
	if (json_object_object_get_ex(js, "name", &o))
		name = VuoText_valueFromJson(o);

	json_object_object_get_ex(js, "transform", &o);
	VuoTransform transform = VuoTransform_valueFromJson(o);


	if (cameraType == VuoSceneObject_PerspectiveCamera)
		return VuoSceneObject_makePerspectiveCamera(
					name,
					transform,
					cameraFieldOfView,
					cameraDistanceMin,
					cameraDistanceMax
					);
	else if (cameraType == VuoSceneObject_StereoCamera)
		return VuoSceneObject_makeStereoCamera(
					name,
					transform,
					cameraFieldOfView,
					cameraDistanceMin,
					cameraDistanceMax,
					cameraConfocalDistance,
					cameraIntraocularDistance
					);
	else if (cameraType == VuoSceneObject_OrthographicCamera)
		return VuoSceneObject_makeOrthographicCamera(
					name,
					transform,
					cameraWidth,
					cameraDistanceMin,
					cameraDistanceMax
					);
	else if (lightType == VuoSceneObject_AmbientLight)
		return VuoSceneObject_makeAmbientLight(lightColor, lightBrightness);
	else if (lightType == VuoSceneObject_PointLight)
		return VuoSceneObject_makePointLight(lightColor, lightBrightness, transform.translation, lightRange, lightSharpness);
	else if (lightType == VuoSceneObject_Spotlight)
		return VuoSceneObject_makeSpotlight(lightColor, lightBrightness, transform, lightCone, lightRange, lightSharpness);
	else
	{
		VuoSceneObject o = VuoSceneObject_make(mesh, shader, transform, childObjects);
		o.isRealSize = isRealSize;
		o.name = name;
		return o;
	}
}

/**
 * @ingroup VuoSceneObject
 * Encodes @c value as a JSON object.
 */
json_object * VuoSceneObject_jsonFromValue(const VuoSceneObject value)
{
	json_object *js = json_object_new_object();

	if (value.cameraType != VuoSceneObject_NotACamera)
	{
		json_object_object_add(js, "cameraType", json_object_new_string(VuoSceneObject_cStringForCameraType(value.cameraType)));
		json_object_object_add(js, "cameraDistanceMin", json_object_new_double(value.cameraDistanceMin));
		json_object_object_add(js, "cameraDistanceMax", json_object_new_double(value.cameraDistanceMax));

		if (value.cameraType == VuoSceneObject_PerspectiveCamera
		 || value.cameraType == VuoSceneObject_StereoCamera)
			json_object_object_add(js, "cameraFieldOfView", json_object_new_double(value.cameraFieldOfView));

		if (value.cameraType == VuoSceneObject_StereoCamera)
		{
			json_object_object_add(js, "cameraConfocalDistance", json_object_new_double(value.cameraConfocalDistance));
			json_object_object_add(js, "cameraIntraocularDistance", json_object_new_double(value.cameraIntraocularDistance));
		}

		if (value.cameraType == VuoSceneObject_OrthographicCamera)
			json_object_object_add(js, "cameraWidth", json_object_new_double(value.cameraWidth));
	}

	if (value.lightType != VuoSceneObject_NotALight)
	{
		json_object_object_add(js, "lightType", json_object_new_string(VuoSceneObject_cStringForLightType(value.lightType)));
		json_object_object_add(js, "lightColor", VuoColor_jsonFromValue(value.lightColor));
		json_object_object_add(js, "lightBrightness", json_object_new_double(value.lightBrightness));

		if (value.lightType == VuoSceneObject_PointLight
				|| value.lightType == VuoSceneObject_Spotlight)
		{
			json_object_object_add(js, "lightRange", json_object_new_double(value.lightRange));
			json_object_object_add(js, "lightSharpness", json_object_new_double(value.lightSharpness));
		}
		if (value.lightType == VuoSceneObject_Spotlight)
		{
			json_object_object_add(js, "lightCone", json_object_new_double(value.lightCone));
		}
	}

	if (value.cameraType == VuoSceneObject_NotACamera
			&& value.lightType == VuoSceneObject_NotALight)
	{
		// visible or group scene object

		if (value.mesh)
		{
			json_object *meshObject = VuoMesh_jsonFromValue(value.mesh);
			json_object_object_add(js, "mesh", meshObject);
		}

		if (value.shader)
		{
			json_object *shaderObject = VuoShader_jsonFromValue(value.shader);
			json_object_object_add(js, "shader", shaderObject);
		}

		json_object *isRealSizeObject = VuoBoolean_jsonFromValue(value.isRealSize);
		json_object_object_add(js, "isRealSize", isRealSizeObject);

		if (value.childObjects)
		{
			json_object *childObjectsObject = VuoList_VuoSceneObject_jsonFromValue(value.childObjects);
			json_object_object_add(js, "childObjects", childObjectsObject);
		}
	}

	if (value.name)
	{
		json_object *nameObject = VuoText_jsonFromValue(value.name);
		json_object_object_add(js, "name", nameObject);
	}

	if (value.lightType != VuoSceneObject_AmbientLight)
	{
		json_object *transformObject = VuoTransform_jsonFromValue(value.transform);
		json_object_object_add(js, "transform", transformObject);
	}

	return js;
}

/**
 * Returns the total number of vertices in the scene object (but not its descendants).
 */
unsigned long VuoSceneObject_getVertexCount(const VuoSceneObject value)
{
	if (!value.mesh)
		return 0;

	unsigned long vertexCount = 0;
	for (unsigned int i = 0; i < value.mesh->submeshCount; ++i)
		vertexCount += value.mesh->submeshes[i].vertexCount;

	return vertexCount;
}

/**
 * Returns the total number of element in the scene object (but not its descendants).
 */
unsigned long VuoSceneObject_getElementCount(const VuoSceneObject value)
{
	if (!value.mesh)
		return 0;

	unsigned long elementCount = 0;
	for (unsigned int i = 0; i < value.mesh->submeshCount; ++i)
		elementCount += value.mesh->submeshes[i].elementCount;

	return elementCount;
}

/**
 * Traverses the specified scenegraph and returns statistics about it.
 */
void VuoSceneObject_getStatistics(const VuoSceneObject value, unsigned long *descendantCount, unsigned long *totalVertexCount, unsigned long *totalElementCount)
{
	unsigned long childObjectCount = 0;
	if (value.childObjects)
		childObjectCount = VuoListGetCount_VuoSceneObject(value.childObjects);
	*descendantCount += childObjectCount;
	*totalVertexCount += VuoSceneObject_getVertexCount(value);
	*totalElementCount += VuoSceneObject_getElementCount(value);

	for (unsigned long i = 1; i <= childObjectCount; ++i)
		VuoSceneObject_getStatistics(VuoListGetValueAtIndex_VuoSceneObject(value.childObjects, i), descendantCount, totalVertexCount, totalElementCount);
}

/**
 * Returns a list of all unique shader names in the sceneobject and its descendants.
 */
VuoList_VuoText VuoSceneRenderer_findShaderNames(VuoSceneObject object)
{
	// Exploit json_object's set-containing-only-unique-items data structure.
	__block json_object *names = json_object_new_object();
	VuoSceneObject_visit(object, ^(const VuoSceneObject currentObject){
							 if (currentObject.shader)
								json_object_object_add(names, currentObject.shader->name, NULL);
						 });

	VuoList_VuoText nameList = VuoListCreate_VuoText();
	json_object_object_foreach(names, key, val)
		VuoListAppendValue_VuoText(nameList, VuoText_make(key));
	json_object_put(names);
	return nameList;
}

/**
 * @ingroup VuoSceneObject
 * Produces a brief human-readable summary of @c value.
 */
char * VuoSceneObject_summaryFromValue(const VuoSceneObject value)
{
	if (value.cameraType != VuoSceneObject_NotACamera)
	{
		const char *cameraType = VuoSceneObject_cStringForCameraType(value.cameraType);

		float cameraViewValue = 0;
		const char *cameraViewString = "";
		if (value.cameraType == VuoSceneObject_PerspectiveCamera)
		{
			cameraViewValue = value.cameraFieldOfView;
			cameraViewString = "° field of view";
		}
		else if (value.cameraType == VuoSceneObject_StereoCamera)
		{
			cameraViewValue = value.cameraFieldOfView;
			cameraViewString = "° field of view (stereoscopic)";
		}
		else if (value.cameraType == VuoSceneObject_OrthographicCamera)
		{
			cameraViewValue = value.cameraWidth;
			cameraViewString = " unit width";
		}

		char *translationString = VuoPoint3d_summaryFromValue(value.transform.translation);

		const char *rotationLabel;
		char *rotationString;
		if (value.transform.type == VuoTransformTypeEuler)
		{
			rotationLabel = "rotated";
			rotationString = VuoPoint3d_summaryFromValue(VuoPoint3d_multiply(value.transform.rotationSource.euler, -180.f/M_PI));
		}
		else
		{
			rotationLabel = "target";
			rotationString = VuoPoint3d_summaryFromValue(value.transform.rotationSource.target);
		}

		char *valueAsString = VuoText_format("%s camera<br>at (%s)<br>%s (%s)<br>%g%s<br>shows objects between depth %g and %g",
											 cameraType, translationString, rotationLabel, rotationString, cameraViewValue, cameraViewString, value.cameraDistanceMin, value.cameraDistanceMax);
		free(rotationString);
		free(translationString);
		return valueAsString;
	}

	if (value.lightType != VuoSceneObject_NotALight)
	{
		const char *lightType = VuoSceneObject_cStringForLightType(value.lightType);
		const char *space = value.lightType == VuoSceneObject_Spotlight ? "" : " ";
		char *colorString = VuoColor_summaryFromValue(value.lightColor);

		char *positionRangeString;
		if (value.lightType == VuoSceneObject_PointLight
				|| value.lightType == VuoSceneObject_Spotlight)
		{
			char *positionString = VuoPoint3d_summaryFromValue(value.transform.translation);

			positionRangeString = VuoText_format("<br>position (%s)<br>range %g units (%g sharpness)",
												 positionString, value.lightRange, value.lightSharpness);

			free(positionString);
		}
		else
			positionRangeString = strdup("");

		char *directionConeString;
		if (value.lightType == VuoSceneObject_Spotlight)
		{
			VuoPoint3d direction = VuoTransform_getDirection(value.transform);
			char *directionString = VuoPoint3d_summaryFromValue(direction);

			directionConeString = VuoText_format("<br>direction (%s)<br>cone %g°",
												 directionString, value.lightCone * 180./M_PI);

			free(directionString);
		}
		else
			directionConeString = strdup("");

		char *valueAsString = VuoText_format("%s%slight<br>color (%s)<br>brightness %g%s%s",
											 lightType, space, colorString, value.lightBrightness, positionRangeString, directionConeString);

		free(directionConeString);
		free(positionRangeString);
		free(colorString);

		return valueAsString;
	}

	unsigned long vertexCount = VuoSceneObject_getVertexCount(value);
	unsigned long elementCount = VuoSceneObject_getElementCount(value);

	char *transform = VuoTransform_summaryFromValue(value.transform);

	unsigned long childObjectCount = 0;
	if (value.childObjects)
		childObjectCount = VuoListGetCount_VuoSceneObject(value.childObjects);
	const char *childObjectPlural = childObjectCount == 1 ? "" : "s";

	char *descendants;
	if (childObjectCount)
	{
		unsigned long descendantCount = 0;
		unsigned long totalVertexCount = 0;
		unsigned long totalElementCount = 0;
		VuoSceneObject_getStatistics(value, &descendantCount, &totalVertexCount, &totalElementCount);
		const char *descendantPlural = descendantCount == 1 ? "" : "s";

		descendants = VuoText_format("<br>%ld descendant%s<br><br>total, including descendants:<br>%ld vertices, %ld elements",
									 descendantCount, descendantPlural, totalVertexCount, totalElementCount);
	}
	else
		descendants = strdup("");

	VuoList_VuoText shaderNames = VuoSceneRenderer_findShaderNames(value);
	VuoRetain(shaderNames);
	char *shaderNamesSummary;
	if (VuoListGetCount_VuoText(shaderNames))
	{
		VuoInteger shaderNameCount = VuoListGetCount_VuoText(shaderNames);
		const char *header = "<br><br>shaders:<ul>";
		VuoInteger shaderNameLength = strlen(header);
		for (VuoInteger i = 1; i <= shaderNameCount; ++i)
			shaderNameLength += strlen("<li>") + strlen(VuoListGetValueAtIndex_VuoText(shaderNames, i)) + strlen("</li>");
		shaderNameLength += strlen("</ul>");

		shaderNamesSummary = (char *)malloc(shaderNameLength + 1);
		char *t = shaderNamesSummary;
		t = strcpy(t, header) + strlen(header);
		for (VuoInteger i = 1; i <= shaderNameCount; ++i)
		{
			t = strcpy(t, "<li>") + strlen("<li>");
			t = strcpy(t, VuoListGetValueAtIndex_VuoText(shaderNames, i)) + strlen(VuoListGetValueAtIndex_VuoText(shaderNames, i));
			t = strcpy(t, "</li>") + strlen("</li>");
		}
		t = strcpy(t, "</ul>");
	}
	else
		shaderNamesSummary = strdup("");
	VuoRelease(shaderNames);

	char *valueAsString = VuoText_format("%ld vertices, %ld elements<br><br>%s<br><br>%ld child object%s%s%s",
										 vertexCount, elementCount, transform, childObjectCount, childObjectPlural, descendants, shaderNamesSummary);

	free(descendants);
	free(transform);
	free(shaderNamesSummary);

	return valueAsString;
}

/**
 * Outputs information about the sceneobject (and its descendants).
 */
static void VuoSceneObject_dump_internal(const VuoSceneObject so, unsigned int level)
{
	for (unsigned int i=0; i<level; ++i)
		fprintf(stderr, "\t");

	fprintf(stderr, "object: %lu vertices, %lu elements\n", VuoSceneObject_getVertexCount(so), VuoSceneObject_getElementCount(so));

	if (so.childObjects)
	{
		unsigned int childObjectCount = VuoListGetCount_VuoSceneObject(so.childObjects);
		for (unsigned int i=1; i<=childObjectCount; ++i)
			VuoSceneObject_dump_internal(VuoListGetValueAtIndex_VuoSceneObject(so.childObjects, i), level+1);
	}
}

/**
 * Outputs information about the sceneobject (and its descendants).
 */
void VuoSceneObject_dump(const VuoSceneObject so)
{
	VuoSceneObject_dump_internal(so,0);
}
