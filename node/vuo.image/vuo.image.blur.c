/**
 * @file
 * vuo.image.blur node implementation.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#include "node.h"
#include "VuoImageRenderer.h"

VuoModuleMetadata({
					  "title" : "Blur Image",
					  "keywords" : [ "gaussian", "distort", "obscure", "smudge", "filter", "censor", "smooth", "soften", "unfocus", "defocus", "detail" ],
					  "version" : "1.1.0",
					  "node": {
						  "exampleCompositions" : [ "BlurMovie.vuo" ]
					  }
				 });

void nodeEvent
(
		VuoInputData(VuoImage) image,
		VuoInputData(VuoReal, {"default":4, "suggestedMin":0, "suggestedMax":20}) radius,
		VuoOutputData(VuoImage) blurredImage
)
{
	*blurredImage = VuoImage_blur(image, radius, FALSE);
}
