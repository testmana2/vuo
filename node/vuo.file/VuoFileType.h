/**
 * @file
 * VuoFileType C type definition.
 *
 * @copyright Copyright © 2012–2014 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOFILETYPE_H
#define VUOFILETYPE_H

/// @{
typedef const struct VuoList_VuoFileType_struct { void *l; } * VuoList_VuoFileType;
#define VuoList_VuoFileType_TYPE_DEFINED
/// @}

/**
 * @ingroup VuoTypes
 * @defgroup VuoFileType VuoFileType
 * The type of data in a file.
 *
 * @{
 */

/**
 * The type of data in a file.
 */
typedef enum
{
	VuoFileType_AnyFile,
	VuoFileType_Audio,
	VuoFileType_Image,
	VuoFileType_Mesh,
	VuoFileType_Movie,
	VuoFileType_Scene,
	VuoFileType_Folder
} VuoFileType;

VuoFileType VuoFileType_valueFromJson(struct json_object * js);
struct json_object * VuoFileType_jsonFromValue(const VuoFileType value);
VuoList_VuoFileType VuoFileType_allowedValues(void);
char * VuoFileType_summaryFromValue(const VuoFileType value);

bool VuoFileType_isFileOfType(const VuoText path, VuoFileType fileType);

/**
 * Automatically generated function.
 */
///@{
VuoFileType VuoFileType_valueFromString(const char *str);
char * VuoFileType_stringFromValue(const VuoFileType value);
void VuoFileType_retain(VuoFileType value);
void VuoFileType_release(VuoFileType value);
///@}

/**
 * @}
 */

#endif // VUOFILETYPE_H
