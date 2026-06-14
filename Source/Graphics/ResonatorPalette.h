// Source/ResonatorPalette.h
#pragma once

#include <juce_graphics/juce_graphics.h>



namespace ResonatorPalette
{
    // Backgrounds
    constexpr juce::uint32 backgroundDeepVal   = 0xff0d1b2a;
    constexpr juce::uint32 backgroundPanelVal  = 0xff1a2a3a;
    constexpr juce::uint32 backgroundWidgetVal = 0xff243444;

    // Accents
    constexpr juce::uint32 accentPrimaryVal    = 0xff9d4edd; // Purple — chord / default elements
    constexpr juce::uint32 accentSecondaryVal  = 0xffff8300; // Orange — arp / high intensity elements

    // Controls
    constexpr juce::uint32 knobBodyVal         = 0xff3a4a5a;
    constexpr juce::uint32 knobIndicatorVal    = 0xffffffff;
    constexpr juce::uint32 knobOutlineVal      = 0xff556677;

    // Text
    constexpr juce::uint32 textPrimaryVal      = 0xffffffff;
    constexpr juce::uint32 textSecondaryVal    = 0xff8899aa;

    // Borders
    constexpr juce::uint32 borderPanelVal      = 0xff2a3a4a;

    // Inline colour constructors for use in paint methods
    inline juce::Colour backgroundDeep()   { return juce::Colour (backgroundDeepVal);   }
    inline juce::Colour backgroundPanel()  { return juce::Colour (backgroundPanelVal);  }
    inline juce::Colour backgroundWidget() { return juce::Colour (backgroundWidgetVal); }
    inline juce::Colour accentPrimary()    { return juce::Colour (accentPrimaryVal);    }
    inline juce::Colour accentSecondary()  { return juce::Colour (accentSecondaryVal);  }
    inline juce::Colour knobBody()         { return juce::Colour (knobBodyVal);         }
    inline juce::Colour knobIndicator()    { return juce::Colour (knobIndicatorVal);    }
    inline juce::Colour knobOutline()      { return juce::Colour (knobOutlineVal);      }
    inline juce::Colour textPrimary()      { return juce::Colour (textPrimaryVal);      }
    inline juce::Colour textSecondary()    { return juce::Colour (textSecondaryVal);    }
    inline juce::Colour borderPanel()      { return juce::Colour (borderPanelVal);      }
}
