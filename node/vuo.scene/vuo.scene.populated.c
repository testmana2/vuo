/**
 * @file
 * vuo.scene.populated node implementation.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"

#include "VuoSceneObject.h"

VuoModuleMetadata({
					 "title" : "Is 3D Object Populated",
					 "keywords" : [ "mesh", "model", "vertices", "non-empty", "nonempty" ],
					 "version" : "1.0.0",
					 "node": {
						 "exampleCompositions" : [ ]
					 }
				 });

void nodeEvent
(
		VuoInputData(VuoSceneObject) object,
		VuoOutputData(VuoBoolean) populated
)
{
	*populated = VuoSceneObject_isPopulated(object);
}
