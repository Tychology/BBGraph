/*
  ==============================================================================

   This file is part of the JUCE examples.
   Copyright (c) 2020 - Raw Material Software Limited

   The code included in this file is provided under the terms of the ISC license
   http://www.isc.org/downloads/software-support-policy/isc-license. Permission
   To use, copy, modify, and/or distribute this software for any purpose with or
   without fee is hereby granted provided that the above copyright notice and
   this permission notice appear in all copies.

   THE SOFTWARE IS PROVIDED "AS IS" WITHOUT ANY WARRANTY, AND ALL WARRANTIES,
   WHETHER EXPRESSED OR IMPLIED, INCLUDING MERCHANTABILITY AND FITNESS FOR
   PURPOSE, ARE DISCLAIMED.

  ==============================================================================
*/

/*******************************************************************************
 The block below describes the properties of this PIP. A PIP is a short snippet
 of code that can be read by the Projucer and used to generate a JUCE project.

 BEGIN_JUCE_PIP_METADATA

 name:             LookAndFeelDemo
 version:          1.0.0
 vendor:           JUCE
 website:          http://juce.com
 description:      Showcases custom look and feel components.

 dependencies:     juce_core, juce_data_structures, juce_events, juce_graphics,
                   juce_gui_basics
 exporters:        xcode_mac, vs2019, linux_make, androidstudio, xcode_iphone

 moduleFlags:      JUCE_STRICT_REFCOUNTEDPOINTER=1

 type:             Component
 mainClass:        LookAndFeelDemo

 useLocalCopy:     1

 END_JUCE_PIP_METADATA

*******************************************************************************/

#pragma once


//==============================================================================
/** Custom Look And Feel subclasss.

    Simply override the methods you need to, anything else will be inherited from the base class.
    It's a good idea not to hard code your colours, use the findColour method along with appropriate
    ColourIds so you can set these on a per-component basis.
*/
struct CustomLookAndFeel    : public juce::LookAndFeel_V4
{
    void drawRoundThumb (juce::Graphics& g, float x, float y, float diameter, juce::Colour colour, float outlineThickness)
    {
        auto halfThickness = outlineThickness * 0.5f;

        juce::Path p;
        p.addEllipse (x + halfThickness,
                      y + halfThickness,
                      diameter - outlineThickness,
                      diameter - outlineThickness);

        juce::DropShadow (juce::Colours::black, 1, {}).drawForPath (g, p);

        g.setColour (colour);
        g.fillPath (p);

        g.setColour (colour.brighter());
        g.strokePath (p, juce::PathStrokeType (outlineThickness));
    }

    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                          .withMultipliedAlpha      (button.isEnabled() ? 0.9f : 0.5f);

        if (isButtonDown || isMouseOverButton)
            baseColour = baseColour.contrasting (isButtonDown ? 0.2f : 0.1f);

        auto flatOnLeft   = button.isConnectedOnLeft();
        auto flatOnRight  = button.isConnectedOnRight();
        auto flatOnTop    = button.isConnectedOnTop();
        auto flatOnBottom = button.isConnectedOnBottom();

        auto width  = (float) button.getWidth()  - 1.0f;
        auto height = (float) button.getHeight() - 1.0f;

        if (width > 0 && height > 0)
        {
            auto cornerSize = juce::jmin (15.0f, juce::jmin (width, height) * 0.45f);
            auto lineThickness = cornerSize    * 0.1f;
            auto halfThickness = lineThickness * 0.5f;

            juce::Path outline;
            outline.addRoundedRectangle (0.5f + halfThickness, 0.5f + halfThickness, width - lineThickness, height - lineThickness,
                                         cornerSize, cornerSize,
                                         ! (flatOnLeft  || flatOnTop),
                                         ! (flatOnRight || flatOnTop),
                                         ! (flatOnLeft  || flatOnBottom),
                                         ! (flatOnRight || flatOnBottom));

            auto outlineColour = button.findColour (button.getToggleState() ? juce::TextButton::textColourOnId
                                                                            : juce::TextButton::textColourOffId);

            g.setColour (baseColour);
            g.fillPath (outline);

            if (! button.getToggleState())
            {
                g.setColour (outlineColour);
                g.strokePath (outline, juce::PathStrokeType (lineThickness));
            }
        }
    }

    void drawTickBox (juce::Graphics& g, juce::Component& component,
                      float x, float y, float w, float h,
                      bool ticked,
                      bool isEnabled,
                      bool isMouseOverButton,
                      bool isButtonDown) override
    {
        auto boxSize = w * 0.7f;

        auto isDownOrDragging = component.isEnabled() && (component.isMouseOverOrDragging() || component.isMouseButtonDown());

        auto colour = component.findColour (juce::TextButton::buttonColourId)
                               .withMultipliedSaturation ((component.hasKeyboardFocus (false) || isDownOrDragging) ? 1.3f : 0.9f)
                               .withMultipliedAlpha (component.isEnabled() ? 1.0f : 0.7f);

        drawRoundThumb (g, x, y + (h - boxSize) * 0.5f, boxSize, colour,
                        isEnabled ? ((isButtonDown || isMouseOverButton) ? 1.1f : 0.5f) : 0.3f);

        if (ticked)
        {
            g.setColour (isEnabled ? findColour (juce::TextButton::buttonOnColourId) : juce::Colours::grey);

            auto scale = 9.0f;
            auto trans = juce::AffineTransform::scale (w / scale, h / scale).translated (x - 2.5f, y + 1.0f);

            g.fillPath (LookAndFeel_V4::getTickShape (6.0f), trans);
        }
    }

    void drawLinearSliderThumb (juce::Graphics& g, int x, int y, int width, int height,
                                float sliderPos, float minSliderPos, float maxSliderPos,
                                const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto sliderRadius = (float) (getSliderThumbRadius (slider) - 2);

        auto isDownOrDragging = slider.isEnabled() && (slider.isMouseOverOrDragging() || slider.isMouseButtonDown());

        auto knobColour = slider.findColour (juce::Slider::thumbColourId)
                                .withMultipliedSaturation ((slider.hasKeyboardFocus (false) || isDownOrDragging) ? 1.3f : 0.9f)
                                .withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.7f);

        if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearVertical)
        {
            float kx, ky;

            if (style == juce::Slider::LinearVertical)
            {
                kx = (float) x + (float) width * 0.5f;
                ky = sliderPos;
            }
            else
            {
                kx = sliderPos;
                ky = (float) y + (float) height * 0.5f;
            }

            auto outlineThickness = slider.isEnabled() ? 0.8f : 0.3f;

            drawRoundThumb (g,
                            kx - sliderRadius,
                            ky - sliderRadius,
                            sliderRadius * 2.0f,
                            knobColour, outlineThickness);
        }
        else
        {
            // Just call the base class for the demo
            LookAndFeel_V2::drawLinearSliderThumb (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawLinearSlider (juce::Graphics& g, int x, int y, int width, int height,
                           float sliderPos, float minSliderPos, float maxSliderPos,
                           const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        g.fillAll (slider.findColour (juce::Slider::backgroundColourId));

        if (style == juce::Slider::LinearBar || style == juce::Slider::LinearBarVertical)
        {
	        juce::Path p;

            if (style == juce::Slider::LinearBarVertical)
                p.addRectangle ((float) x, sliderPos, (float) width, 1.0f + (float) height - sliderPos);
            else
                p.addRectangle ((float) x, (float) y, sliderPos - (float) x, (float) height);

            auto baseColour = slider.findColour (juce::Slider::rotarySliderFillColourId)
                                    .withMultipliedSaturation (slider.isEnabled() ? 1.0f : 0.5f)
                                    .withMultipliedAlpha (0.8f);

            g.setColour (baseColour);
            g.fillPath (p);

            auto lineThickness = juce::jmin (15.0f, (float)juce::jmin (width, height) * 0.45f) * 0.1f;
            g.drawRect (slider.getLocalBounds().toFloat(), lineThickness);
        }
        else
        {
            drawLinearSliderBackground (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
            drawLinearSliderThumb      (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawLinearSliderBackground (juce::Graphics& g, int x, int y, int width, int height,
                                     float /*sliderPos*/,
                                     float /*minSliderPos*/,
                                     float /*maxSliderPos*/,
                                     const juce::Slider::SliderStyle /*style*/, juce::Slider& slider) override
    {
        auto sliderRadius = (float) getSliderThumbRadius (slider) - 5.0f;
        juce::Path on, off;

        if (slider.isHorizontal())
        {
            auto iy = (float) y + (float) height * 0.5f - sliderRadius * 0.5f;
            juce::Rectangle<float> r ((float) x - sliderRadius * 0.5f, iy, (float) width + sliderRadius, sliderRadius);
            auto onW = r.getWidth() * ((float) slider.valueToProportionOfLength (slider.getValue()));

            on.addRectangle (r.removeFromLeft (onW));
            off.addRectangle (r);
        }
        else
        {
            auto ix = (float) x + (float) width * 0.5f - sliderRadius * 0.5f;
            juce::Rectangle<float> r (ix, (float) y - sliderRadius * 0.5f, sliderRadius, (float) height + sliderRadius);
            auto onH = r.getHeight() * ((float) slider.valueToProportionOfLength (slider.getValue()));

            on.addRectangle (r.removeFromBottom (onH));
            off.addRectangle (r);
        }

        g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId));
        g.fillPath (on);

        g.setColour (slider.findColour (juce::Slider::trackColourId));
        g.fillPath (off);
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        auto radius = (float)juce::jmin (width / 2, height / 2) - 2.0f;
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        auto isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

        if (slider.isEnabled())
            g.setColour (slider.findColour (juce::Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 1.0f : 0.7f));
        else
            g.setColour (juce::Colour (0x80808080));

        {
	        juce::Path filledArc;
            filledArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, angle, 0.0);
            g.fillPath (filledArc);
        }

        {
            auto lineThickness = juce::jmin (15.0f, (float)juce::jmin (width, height) * 0.45f) * 0.1f;
            juce::Path outlineArc;
            outlineArc.addPieSegment (rx, ry, rw, rw, rotaryStartAngle, rotaryEndAngle, 0.0);
            g.strokePath (outlineArc, juce::PathStrokeType (lineThickness));
        }
    }
};

//==============================================================================
/** Another really simple look and feel that is very flat and square.

    This inherits from CustomLookAndFeel above for the linear bar and slider backgrounds.
*/
struct SquareLookAndFeel    : public CustomLookAndFeel
{
    void drawButtonBackground (juce::Graphics& g, juce::Button& button, const juce::Colour& backgroundColour,
                               bool isMouseOverButton, bool isButtonDown) override
    {
        auto baseColour = backgroundColour.withMultipliedSaturation (button.hasKeyboardFocus (true) ? 1.3f : 0.9f)
                                          .withMultipliedAlpha      (button.isEnabled() ? 0.9f : 0.5f);

        if (isButtonDown || isMouseOverButton)
            baseColour = baseColour.contrasting (isButtonDown ? 0.2f : 0.1f);

        auto width  = (float) button.getWidth()  - 1.0f;
        auto height = (float) button.getHeight() - 1.0f;

        if (width > 0 && height > 0)
        {
            g.setGradientFill (juce::ColourGradient::vertical (baseColour, 0.0f,
                                                               baseColour.darker (0.1f), height));

            g.fillRect (button.getLocalBounds());
        }
    }

    void drawTickBox (juce::Graphics& g, juce::Component& component,
                      float x, float y, float w, float h,
                      bool ticked,
                      bool isEnabled,
                      bool /*isMouseOverButton*/,
                      bool /*isButtonDown*/) override
    {
        auto boxSize = w * 0.7f;

        auto isDownOrDragging = component.isEnabled() && (component.isMouseOverOrDragging() || component.isMouseButtonDown());

        auto colour = component.findColour (juce::TextButton::buttonOnColourId)
                               .withMultipliedSaturation ((component.hasKeyboardFocus (false) || isDownOrDragging) ? 1.3f : 0.9f)
                               .withMultipliedAlpha (component.isEnabled() ? 1.0f : 0.7f);

        g.setColour (colour);

        juce::Rectangle<float> r (x, y + (h - boxSize) * 0.5f, boxSize, boxSize);
        g.fillRect (r);

        if (ticked)
        {
            auto tickPath = LookAndFeel_V4::getTickShape (6.0f);
            g.setColour (isEnabled ? findColour (juce::TextButton::buttonColourId) : juce::Colours::grey);

            auto transform = juce::RectanglePlacement (juce::RectanglePlacement::centred)
                               .getTransformToFit (tickPath.getBounds(),
                                                   r.reduced (r.getHeight() * 0.05f));

            g.fillPath (tickPath, transform);
        }
    }

    void drawLinearSliderThumb (juce::Graphics& g, int x, int y, int width, int height,
                                float sliderPos, float minSliderPos, float maxSliderPos,
                                const juce::Slider::SliderStyle style, juce::Slider& slider) override
    {
        auto sliderRadius = (float) getSliderThumbRadius (slider);

        bool isDownOrDragging = slider.isEnabled() && (slider.isMouseOverOrDragging() || slider.isMouseButtonDown());

        auto knobColour = slider.findColour (juce::Slider::rotarySliderFillColourId)
                                .withMultipliedSaturation ((slider.hasKeyboardFocus (false) || isDownOrDragging) ? 1.3f : 0.9f)
                                .withMultipliedAlpha (slider.isEnabled() ? 1.0f : 0.7f);

        g.setColour (knobColour);

        if (style == juce::Slider::LinearHorizontal || style == juce::Slider::LinearVertical)
        {
            float kx, ky;

            if (style == juce::Slider::LinearVertical)
            {
                kx = (float) x + (float) width * 0.5f;
                ky = sliderPos;
                g.fillRect (juce::Rectangle<float> (kx - sliderRadius, ky - 2.5f, sliderRadius * 2.0f, 5.0f));
            }
            else
            {
                kx = sliderPos;
                ky = (float) y + (float) height * 0.5f;
                g.fillRect (juce::Rectangle<float> (kx - 2.5f, ky - sliderRadius, 5.0f, sliderRadius * 2.0f));
            }
        }
        else
        {
            // Just call the base class for the demo
            LookAndFeel_V2::drawLinearSliderThumb (g, x, y, width, height, sliderPos, minSliderPos, maxSliderPos, style, slider);
        }
    }

    void drawRotarySlider (juce::Graphics& g, int x, int y, int width, int height, float sliderPos,
                           float rotaryStartAngle, float rotaryEndAngle, juce::Slider& slider) override
    {
        auto diameter = (float)juce::jmin (width, height) - 4.0f;
        auto radius = (diameter / 2.0f) * std::cos (juce::MathConstants<float>::pi / 4.0f);
        auto centreX = (float) x + (float) width  * 0.5f;
        auto centreY = (float) y + (float) height * 0.5f;
        auto rx = centreX - radius;
        auto ry = centreY - radius;
        auto rw = radius * 2.0f;
        auto angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
        bool isMouseOver = slider.isMouseOverOrDragging() && slider.isEnabled();

        auto baseColour = slider.isEnabled() ? slider.findColour (juce::Slider::rotarySliderFillColourId).withAlpha (isMouseOver ? 0.8f : 1.0f)
                                             : juce::Colour (0x80808080);

        juce::Rectangle<float> r (rx, ry, rw, rw);
        auto transform = juce::AffineTransform::rotation (angle, r.getCentreX(), r.getCentreY());

        auto x1 = r.getTopLeft()   .getX();
        auto y1 = r.getTopLeft()   .getY();
        auto x2 = r.getBottomLeft().getX();
        auto y2 = r.getBottomLeft().getY();

        transform.transformPoints (x1, y1, x2, y2);

        g.setGradientFill (juce::ColourGradient (baseColour, x1, y1,
                                                 baseColour.darker (0.1f), x2, y2,
                                                 false));

        juce::Path knob;
        knob.addRectangle (r);
        g.fillPath (knob, transform);

        juce::Path needle;
        auto r2 = r * 0.1f;
        needle.addRectangle (r2.withPosition ({ r.getCentreX() - (r2.getWidth() / 2.0f), r.getY() }));

        g.setColour (slider.findColour (juce::Slider::rotarySliderOutlineColourId));
        g.fillPath (needle, juce::AffineTransform::rotation (angle, r.getCentreX(), r.getCentreY()));
    }
};

//==============================================================================
struct LookAndFeelDemoComponent  : public juce::Component
{
    LookAndFeelDemoComponent()
    {
        addAndMakeVisible (rotarySlider);
        rotarySlider.setValue (2.5);

        addAndMakeVisible (verticalSlider);
        verticalSlider.setValue (6.2);

        addAndMakeVisible (barSlider);
        barSlider.setValue (4.5);

        addAndMakeVisible (incDecSlider);
        incDecSlider.setRange (0.0, 10.0, 1.0);
        incDecSlider.setIncDecButtonsMode (juce::Slider::incDecButtonsDraggable_Horizontal);

        addAndMakeVisible (button1);

        addAndMakeVisible (button2);
        button2.setClickingTogglesState (true);
        button2.setToggleState (true, juce::dontSendNotification);

        addAndMakeVisible (button3);

        addAndMakeVisible (button4);
        button4.setToggleState (true, juce::dontSendNotification);

        for (int i = 0; i < 3; ++i)
        {
            auto* b = radioButtons.add (new juce::TextButton ("Button " + juce::String (i + 1)));

            addAndMakeVisible (b);
            b->setRadioGroupId (42);
            b->setClickingTogglesState (true);

            switch (i)
            {
                case 0:     b->setConnectedEdges (juce::Button::ConnectedOnRight);                            break;
                case 1:     b->setConnectedEdges (juce::Button::ConnectedOnRight + juce::Button::ConnectedOnLeft);  break;
                case 2:     b->setConnectedEdges (juce::Button::ConnectedOnLeft);                             break;
                default:    break;
            }
        }

        radioButtons.getUnchecked (2)->setToggleState (true, juce::dontSendNotification);
    }

    void resized() override
    {
        auto area = getLocalBounds().reduced (10);
        auto row = area.removeFromTop (100);

        rotarySlider  .setBounds (row.removeFromLeft (100).reduced (5));
        verticalSlider.setBounds (row.removeFromLeft (100).reduced (5));
        barSlider     .setBounds (row.removeFromLeft (100).reduced (5, 25));
        incDecSlider  .setBounds (row.removeFromLeft (100).reduced (5, 28));

        row = area.removeFromTop (100);
        button1.setBounds (row.removeFromLeft (100).reduced (5));

        auto row2 = row.removeFromTop (row.getHeight() / 2).reduced (0, 10);
        button2.setBounds (row2.removeFromLeft (100).reduced (5, 0));
        button3.setBounds (row2.removeFromLeft (100).reduced (5, 0));
        button4.setBounds (row2.removeFromLeft (100).reduced (5, 0));

        row2 = (row.removeFromTop (row2.getHeight() + 20).reduced (5, 10));

        for (auto* b : radioButtons)
            b->setBounds (row2.removeFromLeft (100));
    }

    juce::Slider rotarySlider    {juce::Slider::RotaryHorizontalVerticalDrag, juce::Slider::NoTextBox},
                 verticalSlider  {juce::Slider::LinearVertical, juce::Slider::NoTextBox },
                 barSlider       {juce::Slider::LinearBar, juce::Slider::NoTextBox },
                 incDecSlider    {juce::Slider::IncDecButtons, juce::Slider::TextBoxBelow };

    juce::TextButton button1  { "Hello World!" },
                     button2  { "Hello World!" },
                     button3  { "Hello World!" };

    juce::ToggleButton button4 { "Toggle Me" };

    juce::OwnedArray<juce::TextButton> radioButtons;
};

//==============================================================================
