/*
 * Surge XT - a free and open source hybrid synthesizer,
 * built by Surge Synth Team
 *
 * Learn more at https://surge-synthesizer.github.io/
 *
 * Copyright 2018-2024, various authors, as described in the GitHub
 * transaction log.
 *
 * Surge XT is released under the GNU General Public Licence v3
 * or later (GPL-3.0-or-later). The license is found in the "LICENSE"
 * file in the root of this repository, or at
 * https://www.gnu.org/licenses/gpl-3.0.en.html
 */

#include "ClaudeTestUtilities.h"
#include <sstream>
#include <chrono>
#include <iostream>
#include <cmath>

namespace Surge
{
namespace Test
{
namespace Claude
{

std::string TestUtils::createMockClaudeResponse(
    const std::vector<std::pair<std::string, float>>& parameters,
    const std::string& description)
{
    std::ostringstream oss;
    oss << "PARAMETERS:\n";
    
    for (const auto& param : parameters)
    {
        oss << "- " << param.first << ": " << param.second << "\n";
    }
    
    oss << "\n" << description;
    return oss.str();
}

bool TestUtils::verifyParameterValue(SurgeSynthesizer* synth,
                                   const std::string& parameterName,
                                   float expectedValue,
                                   float tolerance)
{
    float actualValue = getParameterValue(synth, parameterName);
    if (actualValue < 0.0f) return false; // Parameter not found
    
    return std::abs(actualValue - expectedValue) <= tolerance;
}

float TestUtils::getParameterValue(SurgeSynthesizer* synth,
                                 const std::string& parameterName)
{
    for (int i = 0; i < synth->storage.getPatch().param_ptr.size(); ++i)
    {
        auto param = synth->storage.getPatch().param_ptr[i];
        if (param)
        {
            char txt[256];
            synth->getParameterName(synth->idForParameter(param), txt);
            std::string fullName = txt;
            
            // Simple matching - in a real implementation, you'd want more sophisticated matching
            if (fullName.find("Osc 1 Type") != std::string::npos && parameterName == "osc1_type") {
                return param->get_value_f01();
            }
            if (fullName.find("Filter 1 Cutoff") != std::string::npos && parameterName == "filter1_cutoff") {
                return param->get_value_f01();
            }
            if (fullName.find("Filter 1 Resonance") != std::string::npos && parameterName == "filter1_resonance") {
                return param->get_value_f01();
            }
            if (fullName.find("Amp EG Attack") != std::string::npos && parameterName == "amp_attack") {
                return param->get_value_f01();
            }
            if (fullName.find("Amp EG Release") != std::string::npos && parameterName == "amp_release") {
                return param->get_value_f01();
            }
            if (fullName.find("Volume") != std::string::npos && parameterName == "volume") {
                return param->get_value_f01();
            }
        }
    }
    return -1.0f; // Not found
}

std::vector<Surge::Claude::PatchModification> TestUtils::createAmbientPatchMods()
{
    return {
        {"osc1_type", 2.0f, "Wavetable for rich harmonics"},
        {"osc2_type", 1.0f, "Sine for smoothness"},
        {"filter1_type", 1.0f, "24dB lowpass"},
        {"filter1_cutoff", 0.3f, "Warm filtering"},
        {"filter1_resonance", 0.2f, "Slight resonance"},
        {"amp_attack", 0.8f, "Very slow attack"},
        {"amp_release", 0.9f, "Long release"},
        {"lfo1_rate", 0.1f, "Very slow LFO"},
        {"volume", 0.7f, "Background level"}
    };
}

std::vector<Surge::Claude::PatchModification> TestUtils::createLeadPatchMods()
{
    return {
        {"osc1_type", 0.0f, "Classic oscillator"},
        {"filter1_cutoff", 0.8f, "Bright filtering"},
        {"filter1_resonance", 0.5f, "Resonant peak"},
        {"amp_attack", 0.05f, "Fast attack"},
        {"amp_release", 0.3f, "Medium release"},
        {"lfo1_rate", 0.4f, "Moderate vibrato"},
        {"volume", 0.9f, "Lead level"}
    };
}

std::vector<Surge::Claude::PatchModification> TestUtils::createBassPatchMods()
{
    return {
        {"osc1_type", 0.0f, "Classic for bass"},
        {"filter1_cutoff", 0.4f, "Low-pass for bass"},
        {"filter1_resonance", 0.1f, "Minimal resonance"},
        {"amp_attack", 0.0f, "Instant attack"},
        {"amp_release", 0.2f, "Short release"},
        {"volume", 0.95f, "Bass level"}
    };
}

std::vector<std::string> TestUtils::getValidParameterNames()
{
    return {
        "osc1_type", "osc2_type", "osc3_type",
        "osc1_pitch", "osc2_pitch", "osc3_pitch",
        "filter1_type", "filter2_type",
        "filter1_cutoff", "filter2_cutoff",
        "filter1_resonance", "filter2_resonance",
        "amp_attack", "amp_decay", "amp_sustain", "amp_release",
        "filter_attack", "filter_decay", "filter_sustain", "filter_release",
        "lfo1_rate", "lfo2_rate",
        "lfo1_shape", "lfo2_shape",
        "volume", "pan", "width",
        "cutoff", "resonance", "attack", "release" // Common aliases
    };
}

std::vector<std::string> TestUtils::getInvalidParameterNames()
{
    return {
        "invalid_param",
        "nonexistent_osc",
        "fake_filter",
        "made_up_envelope",
        "",
        "osc99_type",
        "filter0_cutoff",
        "amp_invalid"
    };
}

std::vector<float> TestUtils::getValidParameterValues()
{
    return {
        0.0f, 0.1f, 0.25f, 0.5f, 0.75f, 0.9f, 1.0f,
        2.0f, 5.0f, 10.0f, // Integer values for types
        -12.0f, 0.0f, 12.0f // Pitch values
    };
}

std::vector<float> TestUtils::getInvalidParameterValues()
{
    return {
        -1.0f, -100.0f, 1000.0f, 99999.0f,
        std::numeric_limits<float>::infinity(),
        -std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::quiet_NaN()
    };
}

void TestUtils::benchmarkParameterApplication(SurgeSynthesizer* synth,
                                            const std::vector<Surge::Claude::PatchModification>& mods,
                                            int iterations)
{
    Surge::Claude::ParameterMapper mapper(synth);
    
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < iterations; ++i)
    {
        mapper.applyModifications(mods);
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
    
    std::cout << "Applied " << mods.size() << " parameters " << iterations 
              << " times in " << duration.count() << " microseconds" << std::endl;
    std::cout << "Average: " << (duration.count() / iterations) 
              << " microseconds per application" << std::endl;
}

bool TestUtils::isValidOscillatorType(float value)
{
    // Assuming oscillator types are integers 0-15
    return value >= 0.0f && value <= 15.0f && std::floor(value) == value;
}

bool TestUtils::isValidFilterType(float value)
{
    // Assuming filter types are integers 0-12
    return value >= 0.0f && value <= 12.0f && std::floor(value) == value;
}

bool TestUtils::isValidNormalizedValue(float value)
{
    return value >= 0.0f && value <= 1.0f && !std::isnan(value) && !std::isinf(value);
}

} // namespace Claude
} // namespace Test
} // namespace Surge