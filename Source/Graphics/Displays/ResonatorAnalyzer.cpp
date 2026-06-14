// Source/ResonatorAnalyzer.cpp
#include "ResonatorAnalyzer.h"


// ============================================================
// Constructor / Destructor
// ============================================================

ResonatorAnalyzer::ResonatorAnalyzer (AudioPluginAudioProcessor& processor)
    : proc (processor)
{
    // Zero the grid
    for (auto& row : grid)
        row.fill (0.0f);

    dampingParam  = proc.apvts.getRawParameterValue ("DAMPING");
    feedbackParam = proc.apvts.getRawParameterValue ("FEEDBACK");

    startTimerHz (30);
}

ResonatorAnalyzer::~ResonatorAnalyzer()
{
    stopTimer();
}

// ============================================================
// Timer — pull data, scroll grid, repaint
// ============================================================

void ResonatorAnalyzer::timerCallback()
{
    // ------------------------------------------------------------------
    // 1. Build baseline noise from the ring buffer.
    //    We read gridCols evenly-spaced samples out of the ring buffer
    //    and use them as the per-column noise baseline for the new row.
    //    The ring buffer is a decimated amplitude signal, so each value
    //    is already in [0, ~1].  We scale it down so noise is subtle.
    // ------------------------------------------------------------------
    const float noiseScale = 0.1f;

    const int ringSize  = AudioPluginAudioProcessor::noiseRingSize;
    const int writePos  = proc.noiseWritePos.load (std::memory_order_acquire);

    for (int col = 0; col < gridCols; ++col)
    {
        // Spread the read position across the last ringSize samples,
        // mapping each column to a slightly different read offset so
        // the noise varies across frequency columns.
        const int offset  = (col * ringSize) / gridCols;
        const int readPos = (writePos - 1 - offset + ringSize) % ringSize;
        const float raw   = proc.noiseRingBuffer[readPos].load (std::memory_order_relaxed);
        scratchRow[col]   = raw * noiseScale;
    }

    // ------------------------------------------------------------------
    // 2. Add resonator peaks on top of the noise baseline.
    //    For each active note, add a gaussian centred at its log-
    //    frequency column position, scaled by envelope amplitude.
    // ------------------------------------------------------------------
    const int   noteCount  = proc.activeNoteCount.load (std::memory_order_acquire);
    const float peakScale  = 0.4f;   // max peak height in normalised units
    const float sigma      = sigmaCols;

    const float damping      = dampingParam  ? dampingParam->load()  : 0.5f;
    const float feedback     = feedbackParam ? feedbackParam->load() : 0.8f;
    const int maxHarmonics = 12;

    for (int n = 0; n < noteCount; ++n)
    {
        const auto& info = proc.activeNoteSnapshot[n];
        if (info.frequency < 0.0f || info.amplitude < 0.001f)
            continue;

        const float gateLevel     = proc.gateValue.load (std::memory_order_relaxed);
        const float compressedAmp = std::pow (info.amplitude, 0.4f) * gateLevel;

        // Fundamental frequency in Hz
        const float fundHz = info.frequency;

        const float feedbackAbs = std::abs (feedback);
        const bool  oddOnly     = feedback < 0.0f;

        for (int h = 1; h <= maxHarmonics; ++h)
        {
            if (oddOnly && (h % 2 == 0)) continue;
            
            // Harmonic frequency
            const float harmonicHz = fundHz * static_cast<float> (h);

            const float hzHigh = 440.0f * std::pow (2.0f, (96.0f - 69.0f) / 12.0f);
            if (harmonicHz > hzHigh) break;
            
            // Amplitude model:
            // - feedback scales overall energy (low feedback = dim everything)
            // - damping attenuates each successive harmonic exponentially
            // - fundamental (h=1) is unaffected by damping, only by feedback
            const float dampingAtten  = std::pow (1.0f - damping, static_cast<float> (h - 1));
            const float harmonicAmp   = compressedAmp * peakScale * feedbackAbs * dampingAtten * (h == 1 ? 1.0f : 0.5f);

            if (harmonicAmp < 0.005f)
            {
                if (oddOnly) continue; // even harmonic was skipped, don't abort
                break;
            }

            // Harmonics above the fundamental get a narrower gaussian
            // so they look like distinct overtone peaks rather than blobs
            const float harmonicSigma = sigma * (h == 1 ? 1.0f : 0.5f);
            const float centreX = hzToLogX (harmonicHz);

            for (int col = 0; col < gridCols; ++col)
            {
                const float x    = static_cast<float> (col) / static_cast<float> (gridCols - 1);
                const float peak = gaussian (x, centreX, harmonicSigma) * harmonicAmp;
                scratchRow[col] += peak;
            }
        }
    }

    // limit and set ends
    for (auto& v : scratchRow)
        v = std::tanh(v);

    scratchRow[0] = 0.0f;
    scratchRow[gridCols-1] = 0.0f;

    // ------------------------------------------------------------------
    // 3. Scroll the grid: drop the oldest row, prepend the new one.
    // ------------------------------------------------------------------
    for (int row = gridRows - 1; row > 0; --row)
        grid[row] = grid[row - 1];

    grid[0] = scratchRow;

    repaint();
}

// ============================================================
// Geometry helpers
// ============================================================

float ResonatorAnalyzer::columnToLogX (int col, int numCols)
{
    // Map col index to a position in [0, 1] using a log frequency scale.
    // We cover MIDI notes 24–96 (C1–C7), which is 72 semitones.
    // A log-frequency scale means equal semitone intervals map to
    // equal pixel distances — so the display matches musical intuition.
    constexpr float midiLow  = 24.0f;
    constexpr float midiHigh = 96.0f;

    const float midi = midiLow + (static_cast<float> (col) / static_cast<float> (numCols - 1))
                                 * (midiHigh - midiLow);

    // Hz for this midi note (just for the log mapping)
    const float hz    = 440.0f * std::pow (2.0f, (midi - 69.0f) / 12.0f);
    const float hzLow = 440.0f * std::pow (2.0f, (midiLow  - 69.0f) / 12.0f);
    const float hzHi  = 440.0f * std::pow (2.0f, (midiHigh - 69.0f) / 12.0f);

    return (std::log2 (hz / hzLow)) / (std::log2 (hzHi / hzLow));
}

float ResonatorAnalyzer::midiNoteToLogX (int midiNote, int /*numCols*/)
{
    constexpr float midiLow  = 24.0f;
    constexpr float midiHigh = 96.0f;

    const float clamped = juce::jlimit (midiLow, midiHigh, static_cast<float> (midiNote));
    const float hz      = 440.0f * std::pow (2.0f, (clamped  - 69.0f) / 12.0f);
    const float hzLow   = 440.0f * std::pow (2.0f, (midiLow  - 69.0f) / 12.0f);
    const float hzHi    = 440.0f * std::pow (2.0f, (midiHigh - 69.0f) / 12.0f);

    return (std::log2 (hz / hzLow)) / (std::log2 (hzHi / hzLow));
}

float ResonatorAnalyzer::hzToLogX (float hz)
{
    constexpr float midiLow  = 24.0f;
    constexpr float midiHigh = 96.0f;
    const float hzLow  = 440.0f * std::pow (2.0f, (midiLow  - 69.0f) / 12.0f);
    const float hzHigh = 440.0f * std::pow (2.0f, (midiHigh - 69.0f) / 12.0f);
    const float clamped = juce::jlimit (hzLow, hzHigh, hz);
    return std::log2 (clamped / hzLow) / std::log2 (hzHigh / hzLow);
}

float ResonatorAnalyzer::gaussian (float x, float centre, float sigma)
{
    const float d = (x - centre) / sigma;
    return std::exp (-0.5f * d * d);
}

juce::Point<float> ResonatorAnalyzer::project (float gx, float gy, float gz) const
{
    // Simple perspective: rows converge to a central vanishing point.
    //
    // gz = 0 → front row  (widest, lowest on screen)
    // gz = 1 → back row   (narrowest, closest to horizon)
    //
    // We linearly interpolate the row's width and vertical position
    // between front and horizon based on gz.

    const float w = static_cast<float> (getWidth());
    const float h = static_cast<float> (getHeight());

    // Vanishing point
    const float vpX = w * 0.5f;
    const float vpY = h * horizonFrac;

    // Front row extents
    const float frontY     = h * frontFrac;
    const float halfFrontW = w * frontWidthFrac * 0.5f;
    const float frontLeft  = vpX - halfFrontW;
    const float frontRight = vpX + halfFrontW;

    // Perspective factor: 1 at front, 0 at vanishing point
    const float pf = 1.0f - gz / 1.5f;
    const float pf2 = 1.0f - gz*gz;

    // Row's left/right X at this depth
    const float rowLeft  = vpX + (frontLeft  - vpX) * pf;
    const float rowRight = vpX + (frontRight - vpX) * pf;

    // Row's base Y at this depth (ignoring height displacement)
    const float rowBaseY = vpY + (frontY - vpY) * pf;

    // X position within the row
    const float screenX = rowLeft + gx * (rowRight - rowLeft);

    // Y: height displacement is also foreshortened by perspective.
    // A full-height (gy=1) peak at the front should be visually tall;
    // at the back it should be small.  We scale by pf so peaks shrink
    // with depth.  maxPeakHeight controls how tall a full-amplitude
    // peak is at the front row.
   const float maxPeakHeight = (frontY - vpY); // nearly fills front-to-horizon range
   const float screenY       = rowBaseY - gy * maxPeakHeight * pf2;

    return { screenX, screenY };
}

// ============================================================
// Paint
// ============================================================

void ResonatorAnalyzer::paint (juce::Graphics& g)
{
    // Background
    g.fillAll (ResonatorPalette::backgroundDeep());

    if (getWidth() < 10 || getHeight() < 10)
        return;

    // ----------------------------------------------------------------
    // Draw the grid as horizontal polylines (one per row) and vertical
    // connecting lines (one per column pair of adjacent rows).
    //
    // We draw back-to-front so closer rows paint over farther ones.
    // ----------------------------------------------------------------

    // Pre-compute projected points for all rows × cols
    // points[row][col]
    using PointRow = std::array<juce::Point<float>, gridCols>;
    std::array<PointRow, gridRows> pts;

    for (int row = 0; row < gridRows; ++row)
    {
        const float gz = static_cast<float> (row) / static_cast<float> (gridRows - 1);

        for (int col = 0; col < gridCols; ++col)
        {
            const float gx = columnToLogX (col, gridCols);
            const float gy = grid[row][col];
            pts[row][col]  = project (gx, gy, gz);
        }
    }

    // ----------------------------------------------------------------
    // Horizontal lines (rows), drawn back-to-front
    // ----------------------------------------------------------------
    for (int row = gridRows - 1; row >= 0; --row)
    {
        const float gz        = static_cast<float> (row) / static_cast<float> (gridRows - 1);
        const float strokeW   = juce::jmap (gz, 0.0f, 1.0f, 2.0f, 0.4f);

        juce::Path rowPath;
        rowPath.startNewSubPath (pts[row][0]);

        for (int col = 1; col < gridCols; ++col)
            rowPath.lineTo (pts[row][col]);


        const float rowBaseY = project (0.5f, 0.0f, gz).getY();
        const float rowPeakY = project (0.5f, 1.0f, gz).getY();

        juce::ColourGradient lineGradient (
            ResonatorPalette::backgroundWidget(),  // colour at the bottom (low points)
            0.0f, rowBaseY,                          // position: front row baseline
            ResonatorPalette::accentSecondary(),   // colour at the top (peaks)
            0.0f, rowPeakY,                             // position: horizon/vanishing point
            false                                  // linear, not radial
        );
        lineGradient.addColour (0.5, ResonatorPalette::accentPrimary()); // purple midpoint

        juce::ColourGradient fillGradient (
            ResonatorPalette::backgroundWidget().withAlpha(0.15f),  // colour at the bottom (low points)
            0.0f, rowBaseY,                          // position: front row baseline
            ResonatorPalette::accentSecondary().withAlpha(0.0f),   // colour at the top (peaks)
            0.0f, rowPeakY,                             // position: horizon/vanishing point
            false                                  // linear, not radial
        );
        fillGradient.addColour (0.5, ResonatorPalette::accentPrimary().withAlpha(0.02f)); // purple midpoint
        
        g.setGradientFill(fillGradient);
        g.fillPath(rowPath);

        g.setGradientFill(lineGradient);
        g.strokePath (rowPath, juce::PathStrokeType (strokeW,
                                                      juce::PathStrokeType::curved,
                                                      juce::PathStrokeType::rounded));
    }

   
}
