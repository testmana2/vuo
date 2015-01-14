/**
 * @file
 * vuo.scene.combine node implementation.
 *
 * @copyright Copyright © 2012–2013 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

VuoModuleMetadata({
					 "title" : "Combine 3D Objects",
					 "keywords" : [ "scenegraph", "group", "join", "together" ],
					 "version" : "1.0.0",
					 "node": {
						 "isInterface" : false,
						 "exampleCompositions" : [ "DisplayScene.vuo" ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoTransform) transform,
		VuoInputData(VuoList_VuoSceneObject) childObjects,
		VuoOutputData(VuoSceneObject) object
)
{
	*object = VuoSceneObject_make(
				NULL,
				NULL,
				transform,
				childObjects
			);
}
