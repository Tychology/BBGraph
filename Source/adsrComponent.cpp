/*
  ==============================================================================

  This is an automatically generated GUI class created by the Projucer!

  Be careful when adding custom code to these files, as only the code within
  the "//[xyz]" and "//[/xyz]" sections will be retained when the file is loaded
  and re-saved.

  Created with Projucer version: 6.1.4

  ------------------------------------------------------------------------------

  The Projucer is part of the JUCE library.
  Copyright (c) 2020 - Raw Material Software Limited.

  ==============================================================================
*/

//[Headers] You can add your own extra header files here...
//[/Headers]

#include "adsrComponent.h"


//[MiscUserDefs] You can add your own user definitions and misc code here...
//[/MiscUserDefs]

//==============================================================================
adsrComponent::adsrComponent ()
{
    //[Constructor_pre] You can add your own custom stuff here..
    //[/Constructor_pre]

    attackSlider.reset (new juce::Slider ("attackSlider"));
    addAndMakeVisible (attackSlider.get());
    attackSlider->setRange (0, 10, 0);
    attackSlider->setSliderStyle (juce::Slider::Rotary);
    attackSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    attackSlider->addListener (this);

    attackSlider->setBounds (0, 0, 88, 104);

    decaySlider.reset (new juce::Slider ("decaySlider"));
    addAndMakeVisible (decaySlider.get());
    decaySlider->setRange (0, 10, 0);
    decaySlider->setSliderStyle (juce::Slider::Rotary);
    decaySlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    decaySlider->addListener (this);

    sustainSlider.reset (new juce::Slider ("sustainSlider"));
    addAndMakeVisible (sustainSlider.get());
    sustainSlider->setRange (0, 10, 0);
    sustainSlider->setSliderStyle (juce::Slider::Rotary);
    sustainSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    sustainSlider->addListener (this);

    releaseSlider.reset (new juce::Slider ("releaseSlider"));
    addAndMakeVisible (releaseSlider.get());
    releaseSlider->setRange (0, 10, 0);
    releaseSlider->setSliderStyle (juce::Slider::Rotary);
    releaseSlider->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    releaseSlider->addListener (this);

    attackSlider5.reset (new juce::Slider ("attackSlider"));
    addAndMakeVisible (attackSlider5.get());
    attackSlider5->setRange (0, 10, 0);
    attackSlider5->setSliderStyle (juce::Slider::Rotary);
    attackSlider5->setTextBoxStyle (juce::Slider::TextBoxBelow, false, 80, 20);
    attackSlider5->addListener (this);

    attackLabel.reset (new juce::Label ("attackLabel",
                                        TRANS("Attack")));
    addAndMakeVisible (attackLabel.get());
    attackLabel->setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
    attackLabel->setJustificationType (juce::Justification::centredTop);
    attackLabel->setEditable (false, false, false);
    attackLabel->setColour (juce::TextEditor::textColourId, juce::Colours::black);
    attackLabel->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));

    attackLabel->setBounds (8, 112, 72, 24);

    decayLabel.reset (new juce::Label ("decayLabel",
                                       TRANS("Decay")));
    addAndMakeVisible (decayLabel.get());
    decayLabel->setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
    decayLabel->setJustificationType (juce::Justification::centredTop);
    decayLabel->setEditable (false, false, false);
    decayLabel->setColour (juce::TextEditor::textColourId, juce::Colours::black);
    decayLabel->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));

    decayLabel->setBounds (96, 112, 72, 24);

    sustainLabel.reset (new juce::Label ("sustainLabel",
                                         TRANS("Sustain")));
    addAndMakeVisible (sustainLabel.get());
    sustainLabel->setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
    sustainLabel->setJustificationType (juce::Justification::centredTop);
    sustainLabel->setEditable (false, false, false);
    sustainLabel->setColour (juce::TextEditor::textColourId, juce::Colours::black);
    sustainLabel->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));

    sustainLabel->setBounds (184, 112, 72, 24);

    releaseLabel.reset (new juce::Label ("releaseLabel",
                                         TRANS("Release")));
    addAndMakeVisible (releaseLabel.get());
    releaseLabel->setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
    releaseLabel->setJustificationType (juce::Justification::centredTop);
    releaseLabel->setEditable (false, false, false);
    releaseLabel->setColour (juce::TextEditor::textColourId, juce::Colours::black);
    releaseLabel->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));

    releaseLabel->setBounds (272, 112, 72, 24);

    volumeLabel.reset (new juce::Label ("volumeLabel",
                                        TRANS("Volume")));
    addAndMakeVisible (volumeLabel.get());
    volumeLabel->setFont (juce::Font (15.00f, juce::Font::plain).withTypefaceStyle ("Regular"));
    volumeLabel->setJustificationType (juce::Justification::centredTop);
    volumeLabel->setEditable (false, false, false);
    volumeLabel->setColour (juce::TextEditor::textColourId, juce::Colours::black);
    volumeLabel->setColour (juce::TextEditor::backgroundColourId, juce::Colour (0x00000000));

    volumeLabel->setBounds (408, 112, 72, 24);


    //[UserPreSize]
    //[/UserPreSize]

    setSize (600, 400);


    //[Constructor] You can add your own custom stuff here..
    //[/Constructor]
}

adsrComponent::~adsrComponent()
{
    //[Destructor_pre]. You can add your own custom destruction code here..
    //[/Destructor_pre]

    attackSlider = nullptr;
    decaySlider = nullptr;
    sustainSlider = nullptr;
    releaseSlider = nullptr;
    attackSlider5 = nullptr;
    attackLabel = nullptr;
    decayLabel = nullptr;
    sustainLabel = nullptr;
    releaseLabel = nullptr;
    volumeLabel = nullptr;


    //[Destructor]. You can add your own custom destruction code here..
    //[/Destructor]
}

//==============================================================================
void adsrComponent::paint (juce::Graphics& g)
{
    //[UserPrePaint] Add your own custom painting code here..
    //[/UserPrePaint]

    g.fillAll (juce::Colour (0xff323e44));

    //[UserPaint] Add your own custom painting code here..
    //[/UserPaint]
}

void adsrComponent::resized()
{
    //[UserPreResize] Add your own custom resize code here..
    //[/UserPreResize]

    decaySlider->setBounds (88, 0, getWidth() - 1046, 104);
    sustainSlider->setBounds (176, 0, getWidth() - 1046, 104);
    releaseSlider->setBounds (264, 0, getWidth() - 1046, 104);
    attackSlider5->setBounds (400, 0, getWidth() - 1046, 104);
    //[UserResized] Add your own custom resize handling here..
    //[/UserResized]
}

void adsrComponent::sliderValueChanged (juce::Slider* sliderThatWasMoved)
{
    //[UsersliderValueChanged_Pre]
    //[/UsersliderValueChanged_Pre]

    if (sliderThatWasMoved == attackSlider.get())
    {
        //[UserSliderCode_attackSlider] -- add your slider handling code here..
        //[/UserSliderCode_attackSlider]
    }
    else if (sliderThatWasMoved == decaySlider.get())
    {
        //[UserSliderCode_decaySlider] -- add your slider handling code here..
        //[/UserSliderCode_decaySlider]
    }
    else if (sliderThatWasMoved == sustainSlider.get())
    {
        //[UserSliderCode_sustainSlider] -- add your slider handling code here..
        //[/UserSliderCode_sustainSlider]
    }
    else if (sliderThatWasMoved == releaseSlider.get())
    {
        //[UserSliderCode_releaseSlider] -- add your slider handling code here..
        //[/UserSliderCode_releaseSlider]
    }
    else if (sliderThatWasMoved == attackSlider5.get())
    {
        //[UserSliderCode_attackSlider5] -- add your slider handling code here..
        //[/UserSliderCode_attackSlider5]
    }

    //[UsersliderValueChanged_Post]
    //[/UsersliderValueChanged_Post]
}



//[MiscUserCode] You can add your own definitions of your custom methods or any other code here...
//[/MiscUserCode]


//==============================================================================
#if 0
/*  -- Projucer information section --

    This is where the Projucer stores the metadata that describe this GUI layout, so
    make changes in here at your peril!

BEGIN_JUCER_METADATA

<JUCER_COMPONENT documentType="Component" className="adsrComponent" componentName=""
                 parentClasses="public juce::Component" constructorParams="" variableInitialisers=""
                 snapPixels="8" snapActive="1" snapShown="1" overlayOpacity="0.330"
                 fixedSize="0" initialWidth="600" initialHeight="400">
  <BACKGROUND backgroundColour="ff323e44"/>
  <SLIDER name="attackSlider" id="cd60ebc3047c63ea" memberName="attackSlider"
          virtualName="" explicitFocusOrder="0" pos="0 0 88 104" min="0.0"
          max="10.0" int="0.0" style="Rotary" textBoxPos="TextBoxBelow"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
  <SLIDER name="decaySlider" id="8b76f938070c7db0" memberName="decaySlider"
          virtualName="" explicitFocusOrder="0" pos="88 0 114M 104" min="0.0"
          max="10.0" int="0.0" style="Rotary" textBoxPos="TextBoxBelow"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
  <SLIDER name="sustainSlider" id="c8244fb75e6a89d5" memberName="sustainSlider"
          virtualName="" explicitFocusOrder="0" pos="176 0 114M 104" min="0.0"
          max="10.0" int="0.0" style="Rotary" textBoxPos="TextBoxBelow"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
  <SLIDER name="releaseSlider" id="3dd01c7d4ddf64f2" memberName="releaseSlider"
          virtualName="" explicitFocusOrder="0" pos="264 0 114M 104" min="0.0"
          max="10.0" int="0.0" style="Rotary" textBoxPos="TextBoxBelow"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
  <SLIDER name="attackSlider" id="26304e359f61a1ca" memberName="attackSlider5"
          virtualName="" explicitFocusOrder="0" pos="400 0 114M 104" min="0.0"
          max="10.0" int="0.0" style="Rotary" textBoxPos="TextBoxBelow"
          textBoxEditable="1" textBoxWidth="80" textBoxHeight="20" skewFactor="1.0"
          needsCallback="1"/>
  <LABEL name="attackLabel" id="5f86c45e036d67a6" memberName="attackLabel"
         virtualName="" explicitFocusOrder="0" pos="8 112 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Attack" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.0"
         kerning="0.0" bold="0" italic="0" justification="12"/>
  <LABEL name="decayLabel" id="ac04a923223d5611" memberName="decayLabel"
         virtualName="" explicitFocusOrder="0" pos="96 112 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Decay" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.0"
         kerning="0.0" bold="0" italic="0" justification="12"/>
  <LABEL name="sustainLabel" id="6608d98a4bf48f66" memberName="sustainLabel"
         virtualName="" explicitFocusOrder="0" pos="184 112 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Sustain" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.0"
         kerning="0.0" bold="0" italic="0" justification="12"/>
  <LABEL name="releaseLabel" id="1a3a3fbc8ad31440" memberName="releaseLabel"
         virtualName="" explicitFocusOrder="0" pos="272 112 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Release" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.0"
         kerning="0.0" bold="0" italic="0" justification="12"/>
  <LABEL name="volumeLabel" id="efb5a7c27efc294f" memberName="volumeLabel"
         virtualName="" explicitFocusOrder="0" pos="408 112 72 24" edTextCol="ff000000"
         edBkgCol="0" labelText="Volume" editableSingleClick="0" editableDoubleClick="0"
         focusDiscardsChanges="0" fontname="Default font" fontsize="15.0"
         kerning="0.0" bold="0" italic="0" justification="12"/>
</JUCER_COMPONENT>

END_JUCER_METADATA
*/
#endif


//[EndFile] You can add extra defines here...
//[/EndFile]

