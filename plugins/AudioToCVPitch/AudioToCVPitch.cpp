/*
 * DISTRHO PitchTracking Series
 * Copyright (C) 2021-2022 Bram Giesen
 * Copyright (C) 2022 Filipe Coelho <falktx@falktx.com>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 3 of
 * the License, or any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * For a full copy of the GNU General Public License see the LICENSE file.
 */

#include "DistrhoPlugin.hpp"

extern "C" {
#include <aubio.h>
}

START_NAMESPACE_DISTRHO

// -----------------------------------------------------------------------

// aubio setup values
static constexpr const uint32_t kAubioHopSize = 8;
static constexpr const uint32_t kAubioBufferSize = (1024 + 256 + 128) / kAubioHopSize;

// default values
static constexpr const float kDefaultSensitivity = 60.f;
static constexpr const float kDefaultTolerance = 6.25f;
static constexpr const float kDefaultThreshold = 12.5f;
static constexpr const int kDefaultOctave = -3;
static constexpr const bool kDefaultHoldOutputPitch = false;

// static checks
static_assert(sizeof(smpl_t) == sizeof(float), "smpl_t is float");
static_assert(kAubioBufferSize % kAubioHopSize == 0, "kAubioBufferSize / kAubioHopSize has no remainder");

// -----------------------------------------------------------------------

class AudioToCVPitch : public Plugin
{
    enum Parameters {
        paramSensitivity = 0,
        paramConfidenceThreshold,
        paramTolerance,
        paramOctave,
        paramHoldOutputPitch,
        paramDetectedPitch,
        paramPitchConfidence,
        paramCount
    };

    enum Outputs {
        outputPitch,
        outputSignal
    };

    struct {
        float sensitivity = kDefaultSensitivity;
        float threshold = kDefaultThreshold;
        int octave = kDefaultOctave;
        bool holdOutputPitch = kDefaultHoldOutputPitch;
    } parameters;

    float lastKnownPitchLinear = 0.f;
    float lastKnownPitchInHz = 0.f;
    float lastKnownPitchConfidence = 0.f;

    fvec_t* const detectedPitch = new_fvec(1);
    fvec_t* const inputBuffer = new_fvec(kAubioBufferSize);
    uint32_t inputBufferPos = 0;

    aubio_pitch_t* pitchDetector = nullptr;

public:
    AudioToCVPitch()
        : Plugin(paramCount, 0, 0)
    {
        setLatency(kAubioBufferSize);
        recreateAubioPitchDetector(getSampleRate());
    }

    ~AudioToCVPitch() override
    {
        if (pitchDetector != nullptr)
            del_aubio_pitch(pitchDetector);

        del_fvec(detectedPitch);
        del_fvec(inputBuffer);
    }

protected:
    // -------------------------------------------------------------------
    // Information

    const char* getLabel() const noexcept override
    {
        return "AudioToCVPitch";
    }

    const char* getDescription() const override
    {
        return "This plugin converts a monophonic audio signal to CV pitch";
    }

    const char* getMaker() const noexcept override
    {
        return "Bram Giesen and falkTX";
    }

    const char* getHomePage() const override
    {
        return "http://bramgiesen.com";
    }

    const char* getLicense() const noexcept override
    {
        return "GPLv3.0+";
    }

    uint32_t getVersion() const noexcept override
    {
        return d_version(1, 0, 0);
    }

    int64_t getUniqueId() const noexcept override
    {
        return d_cconst('P', 'T', 'c', 'v');
    }

    // -------------------------------------------------------------------
    // Init

    void initAudioPort(bool input, uint32_t index, AudioPort& port) override
    {
        if (input)
            return Plugin::initAudioPort(input, index, port);

        switch (index)
        {
        case outputPitch:
            port.name   = "Pitch Out";
            port.symbol = "PitchOut";
            port.hints  = kAudioPortIsCV | kCVPortHasPositiveUnipolarRange | kCVPortHasScaledRange;
            break;
        case outputSignal:
            port.name   = "Signal Out";
            port.symbol = "SignalOut";
            port.hints  = kAudioPortIsCV | kCVPortHasPositiveUnipolarRange | kCVPortHasScaledRange;
            break;
        }
    }

    void initParameter(uint32_t index, Parameter& parameter) override
    {
        switch (index)
        {
        case paramSensitivity:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Sensitivity";
            parameter.symbol = "Sensitivity";
            parameter.unit = "%";
            parameter.ranges.def = 60.f;
            parameter.ranges.min = 0.1f;
            parameter.ranges.max = 100.f;
            break;
        case paramConfidenceThreshold:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Confidence Threshold";
            parameter.symbol = "ConfidenceThreshold";
            parameter.unit = "%";
            parameter.ranges.def = 12.5f;
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 100.f;
            break;
        case paramTolerance:
            parameter.hints = kParameterIsAutomatable;
            parameter.name = "Tolerance";
            parameter.symbol = "Tolerance";
            parameter.unit = "%";
            parameter.ranges.def = 6.25f; // default is 0.15 for yin and 0.85 for yinfft
            parameter.ranges.min = 0.0f;
            parameter.ranges.max = 100.f;
            break;
        case paramOctave:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
            parameter.name = "Octave";
            parameter.symbol = "Octave";
            parameter.ranges.def = -3;
            parameter.ranges.min = -4;
            parameter.ranges.max = 4;
            break;
        case paramHoldOutputPitch:
            parameter.hints = kParameterIsAutomatable | kParameterIsInteger | kParameterIsBoolean;
            parameter.name = "Hold output pitch";
            parameter.symbol = "HoldOutputPitch";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 1;
            break;
        case paramDetectedPitch:
            parameter.hints = kParameterIsAutomatable | kParameterIsOutput;
            parameter.name = "Detected Pitch";
            parameter.symbol = "DetectedPitch";
            parameter.unit = "Hz";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 22050;
            break;
        case paramPitchConfidence:
            parameter.hints = kParameterIsAutomatable | kParameterIsOutput;
            parameter.name = "Pitch Confidence";
            parameter.symbol = "PitchConfidence";
            parameter.unit = "%";
            parameter.ranges.def = 0;
            parameter.ranges.min = 0;
            parameter.ranges.max = 100;
            break;
        }
    }

    // -------------------------------------------------------------------
    // Internal data

    float getParameterValue(uint32_t index) const override
    {
        switch (index)
        {
            case paramSensitivity:
                return parameters.sensitivity;
            case paramConfidenceThreshold:
                return parameters.threshold * 100.f;
            case paramTolerance:
                return aubio_pitch_get_tolerance(pitchDetector) * 100.f;
            case paramOctave:
                return parameters.octave;
            case paramHoldOutputPitch:
                return parameters.holdOutputPitch ? 1.0f : 0.0f;
            case paramDetectedPitch:
                return lastKnownPitchInHz;
            case paramPitchConfidence:
                return lastKnownPitchConfidence * 100.f;
            default:
                return 0.0f;
        }
    }

    void setParameterValue(uint32_t index, float value) override
    {
        switch (index)
        {
            case paramSensitivity:
                parameters.sensitivity = value;
                break;
            case paramConfidenceThreshold:
                parameters.threshold = value * 0.01f;
                break;
            case paramTolerance:
                aubio_pitch_set_tolerance(pitchDetector, value * 0.01f);
                break;
            case paramOctave:
                parameters.octave = static_cast<int>(value + 0.5f); // round up
                break;
            case paramHoldOutputPitch:
                parameters.holdOutputPitch = value > 0.5f;
                break;
        }
    }

    // -------------------------------------------------------------------
    // Process

    void activate() override
    {
        inputBufferPos = 0;
    }

    void run(const float** inputs, float** outputs, uint32_t numFrames) override
    {
        float cvPitch = lastKnownPitchLinear;
        float cvSignal = cvPitch > 0.0f ? 1.0f : 0.0f;

        for (uint32_t i = 0; i < numFrames; ++i)
        {
            inputBuffer->data[inputBufferPos] = inputs[0][i] * parameters.sensitivity;

            if (++inputBufferPos == kAubioBufferSize)
            {
                inputBufferPos = 0;

                aubio_pitch_do(pitchDetector, inputBuffer, detectedPitch);
                const float detectedPitchInHz = fvec_get_sample(detectedPitch, 0);
                const float pitchConfidence = aubio_pitch_get_confidence(pitchDetector);

                if (detectedPitchInHz > 0.f && pitchConfidence >= parameters.threshold)
                {
                    const float linearPitch = (12.f * log2f(detectedPitchInHz / 440.f) + 69.f) + (12.f * parameters.octave);
                    lastKnownPitchLinear = cvPitch = std::max(0.f, std::min(10.f, linearPitch * (1.f/12.f)));
                    lastKnownPitchInHz = detectedPitchInHz;
                    cvSignal = 1.f;
                }
                else if (parameters.holdOutputPitch)
                {
                    cvPitch = lastKnownPitchLinear;
                    cvSignal = 0.f;
                }
                else
                {
                    lastKnownPitchInHz = lastKnownPitchLinear = cvPitch = 0.0f;
                    cvSignal = 0.f;
                }

                lastKnownPitchConfidence = pitchConfidence;
            }

            outputs[outputPitch][i] = cvPitch;
            outputs[outputSignal][i] = cvSignal;
        }
    }

    void sampleRateChanged(const double newSampleRate) override
    {
        recreateAubioPitchDetector(newSampleRate);
    }

private:
    void recreateAubioPitchDetector(const double sampleRate)
    {
        float tolerance;

        if (pitchDetector != nullptr)
        {
            tolerance = aubio_pitch_get_tolerance(pitchDetector);
            del_aubio_pitch(pitchDetector);
        }
        else
        {
            tolerance = kDefaultTolerance * 0.01f;
        }

        pitchDetector = new_aubio_pitch("yinfast", kAubioBufferSize, kAubioHopSize, sampleRate);
        DISTRHO_SAFE_ASSERT_RETURN(pitchDetector != nullptr,);

        aubio_pitch_set_silence(pitchDetector, -30.0f);
        aubio_pitch_set_tolerance(pitchDetector, tolerance);
        aubio_pitch_set_unit(pitchDetector, "Hz");
    }

    DISTRHO_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(AudioToCVPitch)
};

// -----------------------------------------------------------------------

Plugin* createPlugin()
{
    return new AudioToCVPitch();
}

// -----------------------------------------------------------------------

END_NAMESPACE_DISTRHO
