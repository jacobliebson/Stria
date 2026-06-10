// Source/ResonatorAnalyzer.h
#pragma once
#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "ResonatorPalette.h"

// ============================================================
// ResonatorAnalyzer
//
// A 3-D perspective wireframe showing resonator activity.
//
// Visual layers (back to front, painted in one pass):
//   1. Grid mesh   — a rows × cols grid of vertices whose Y
//      displacement is driven by the noise ring buffer.
//      Old rows scroll back in Z; a new row is prepended each
//      frame at the front.
//   2. Peak layer  — gaussian peaks are added on top of the
//      noise baseline at each active note's log-frequency
//      position, scaled by the envelope amplitude.
//
// Perspective projection: straight vanishing-point, no tilt.
// Color: interpolates accentPrimary (purple) → accentSecondary
// (orange) based on normalised height.
// ============================================================
class ResonatorAnalyzer : public juce::Component,
                          private juce::Timer
{
public:
    explicit ResonatorAnalyzer (AudioPluginAudioProcessor& processor);
    ~ResonatorAnalyzer() override;

    void paint  (juce::Graphics&) override;
    void resized() override {}

private:
    // ----------------------------------------------------------------
    // Timer callback — pulls data from processor and triggers repaint
    // ----------------------------------------------------------------
    void timerCallback() override;

    // ----------------------------------------------------------------
    // Geometry helpers
    // ----------------------------------------------------------------

    // Map a grid column index → log-normalised X position [0, 1]
    // covering MIDI notes 24–96 (C1–C7).
    static float columnToLogX (int col, int numCols);

    // Convert a column X [0,1] and MIDI note to the same scale
    static float midiNoteToLogX (int midiNote, int numCols);

    static float hzToLogX (float hz);

    // Evaluate a gaussian peak: exp(-0.5 * ((x - centre) / sigma)^2)
    static float gaussian (float x, float centre, float sigma);

    // Project a 3-D grid point (gx, gy, gz) into 2-D screen space.
    //   gx in [0,1], gy in [0,1] (height), gz in [0,1] (depth, 0=front)
    juce::Point<float> project (float gx, float gy, float gz) const;

    // Colour blend based on normalised height [0,1]
    static juce::Colour heightColour (float t);

    // ----------------------------------------------------------------
    // Grid data
    // ----------------------------------------------------------------
    static constexpr int gridCols    = 128;   // frequency resolution
    static constexpr int gridRows    = 28;   // time-history depth
    static constexpr float sigmaCols = 0.015f; // gaussian width in normalised X

    // height[row][col] — row 0 is the front (newest)
    using GridRow = std::array<float, gridCols>;
    std::array<GridRow, gridRows> grid {};

    // Scratch buffer reused each frame to build the newest row
    GridRow scratchRow {};

    // ----------------------------------------------------------------
    // Perspective parameters (recalculated in project())
    // ----------------------------------------------------------------
    // Horizon fraction: where the vanishing point sits vertically
    // (0 = top, 1 = bottom of component)
    // Was 0.38f — move horizon down slightly to give peaks room above it
    static constexpr float horizonFrac = 0.2f;

    // Was 0.55f — reduce so tall peaks don't escape the top boundary
    static constexpr float maxPeakHeightFrac = 0.1f;

    // How much of the component height the front row occupies
    static constexpr float frontFrac   = 0.92f;

    // How much of the component width the front row spans
    static constexpr float frontWidthFrac = 0.88f;

    // ----------------------------------------------------------------
    AudioPluginAudioProcessor& proc;

    std::atomic<float>* dampingParam  = nullptr;
    std::atomic<float>* feedbackParam = nullptr;    

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ResonatorAnalyzer)
};
