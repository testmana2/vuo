/**
 * @file
 * VuoInputEditorTransform interface.
 *
 * @copyright Copyright © 2012–2016 Kosada Incorporated.
 * This code may be modified and distributed under the terms of the MIT License.
 * For more information, see http://vuo.org/license.
 */

#ifndef VUOINPUTEDITORTRANSFORM_HH
#define VUOINPUTEDITORTRANSFORM_HH

#include "VuoInputEditorWithLineEdit.hh"

/**
 * A VuoInputEditorTransform factory.
 */
class VuoInputEditorTransformFactory : public VuoInputEditorFactory
{
   Q_OBJECT
   Q_PLUGIN_METADATA(IID "org.vuo.inputEditor" FILE "VuoInputEditorTransform.json")
   Q_INTERFACES(VuoInputEditorFactory)

public:
   virtual VuoInputEditor * newInputEditor(void);
};

/**
 * An input editor that displays widgets for editing a VuoTransform value,
 * allowing the user to specify the XYZ translation, XYZ rotation (yaw/pitch/roll), and XYZ scale.
 *
 * This input editor recognizes the following keys in the JSON details object:
 *   - "suggestedMin" and "suggestedMax" define the range of the slider (if both are defined)
 *		or spin box (if one is defined) but don't affect the line edit. By default, the dialog
 *      contains a spin box with unbounded range.
 *	 - "suggestedStep" defines the step size of the slider or spin box. By default, the step size is 1.
 *
 * @eg{
 *   {
 *     "suggestedMin" : -1.0,
 *     "suggestedMax" : 1.0,
 *     "suggestedStep" : 0.25
 *   }
 * }
 */
class VuoInputEditorTransform : public VuoInputEditorWithLineEdit
{
	Q_OBJECT

protected:
	void setUpDialog(QDialog &dialog, json_object *originalValue, json_object *details);
	json_object * getAcceptedValue(void);
	QString convertToLineEditFormat(json_object *value);
	json_object * convertFromLineEditsFormat(const QString &xTranslationAsString,
											 const QString &yTranslationAsString,
											 const QString &zTranslationAsString,
											 const QString &xRotationAsString,
											 const QString &yRotationAsString,
											 const QString &zRotationAsString,
											 const QString &xScaleAsString,
											 const QString &yScaleAsString,
											 const QString &zScaleAsString);
	bool eventFilter(QObject *object, QEvent *event);

private:
	enum coord
	{
		xTranslation,
		yTranslation,
		zTranslation,
		xRotation,
		yRotation,
		zRotation,
		xScale,
		yScale,
		zScale
	};

	map<coord, double> suggestedMinForCoord;	///< The minimum <coord>-value selectable via spinbox or slider.
	map<coord, double> suggestedMaxForCoord;	///< The maximum <coord>-value selectable via spinbox or slider.
	map<coord, double> suggestedStepForCoord;	///< The spinbox step value for <coord>.
	map<coord, QLineEdit *> lineEditForCoord;
	map<coord, QSlider *> sliderForCoord;
	map<coord, QDoubleSpinBox *> spinBoxForCoord;
	map<coord, QLabel *> labelForCoord;

	int lineEditValueToScaledSliderValue(double lineEditValue, coord whichCoord);
	double sliderValueToScaledLineEditValue(int sliderValue, coord whichCoord);

	void updateLineEditValue(int newSliderValue, coord whichCoord);

private slots:
	void updateSliderValue(QString newLineEditText);
	void updateLineEditValue();
	void updateLineEditValue(int newSliderValue);

	void emitValueChanged();
};

#endif // VUOINPUTEDITORTRANSFORM_HH
