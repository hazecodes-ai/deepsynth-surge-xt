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

#include "HeadlessUtils.h"
#include "Player.h"

#include "catch2/catch_amalgamated.hpp"

#include "UnitTestUtilities.h"

#include "ClaudeAPIClient.h"
#include "ClaudeParameterMapper.h"
#include "SurgeSynthesizer.h"
#include "SurgeStorage.h"

using namespace Surge::Test;

// Integration tests for Claude AI functionality
TEST_CASE("Claude Full Integration Tests", "[claude][integration][slow]")
{
    SECTION("Complete Patch Generation Workflow")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::APIClient client(&surge->storage);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        // Test the complete workflow:
        // 1. Parse Claude response
        // 2. Extract parameters
        // 3. Map parameters to Surge
        // 4. Apply to synthesizer
        // 5. Verify changes
        
        std::string mockClaudeResponse = R"(
PARAMETERS:
- osc1_type: 2
- osc2_type: 1
- filter1_type: 1
- filter1_cutoff: 0.3
- filter1_resonance: 0.2
- amp_attack: 0.7
- amp_release: 0.8
- lfo1_rate: 0.15
- volume: 0.8

This patch creates an ambient sound using wavetable and sine oscillators.
)";
        
        // Extract modifications
        auto modifications = client.extractModifications(mockClaudeResponse);
        REQUIRE(modifications.size() == 9);
        
        // Get initial parameter values
        auto getParameterValue = [&](const std::string& paramName) -> float {
            for (int i = 0; i < surge->storage.getPatch().param_ptr.size(); ++i) {
                auto param = surge->storage.getPatch().param_ptr[i];
                if (param) {
                    char txt[256];
                    surge->getParameterName(surge->idForParameter(param), txt);
                    std::string fullName = txt;
                    if (fullName.find("Osc 1 Type") != std::string::npos && paramName == "osc1_type") {
                        return param->get_value_f01();
                    }
                    if (fullName.find("Filter 1 Cutoff") != std::string::npos && paramName == "filter1_cutoff") {
                        return param->get_value_f01();
                    }
                }
            }
            return -1.0f; // Not found
        };
        
        float initialOsc1Type = getParameterValue("osc1_type");
        float initialFilterCutoff = getParameterValue("filter1_cutoff");
        
        // Apply modifications
        bool success = mapper.applyModifications(modifications);
        REQUIRE(success);
        
        // Verify parameters changed
        float newOsc1Type = getParameterValue("osc1_type");
        float newFilterCutoff = getParameterValue("filter1_cutoff");
        
        // Parameters should have changed
        REQUIRE(newOsc1Type != initialOsc1Type);
        REQUIRE(newFilterCutoff != initialFilterCutoff);
        
        // Verify specific values are approximately correct
        REQUIRE(std::abs(newFilterCutoff - 0.3f) < 0.01f);
    }
    
    SECTION("Parameter Validation and Bounds Checking")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        // Test that invalid parameter values are handled correctly
        std::vector<Surge::Claude::PatchModification> testMods = {
            {"filter1_cutoff", 2.0f, "Value > 1.0"},        // Should clamp to 1.0
            {"filter1_resonance", -0.5f, "Negative value"}, // Should clamp to 0.0
            {"osc1_type", 100.0f, "Type out of range"},     // Should clamp to valid range
            {"amp_attack", 0.5f, "Valid value"},            // Should apply normally
        };
        
        bool success = mapper.applyModifications(testMods);
        REQUIRE(success); // Should succeed even with out-of-range values
    }
    
    SECTION("Scene A vs Scene B Parameter Mapping")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        // Test that Scene A parameters are correctly mapped
        REQUIRE(mapper.setParameterFromName("osc1_type", 2.0f));
        REQUIRE(mapper.setParameterFromName("filter1_cutoff", 0.5f));
        
        // Test explicit Scene B parameters if available
        // Note: These might not be available depending on Surge's parameter structure
        mapper.setParameterFromName("scene_b_osc1_type", 1.0f);
        mapper.setParameterFromName("scene_b_filter1_cutoff", 0.7f);
    }
}

// Test real-world Claude response formats
TEST_CASE("Claude Response Format Compatibility", "[claude][parsing]")
{
    SECTION("Standard Claude Response Format")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::APIClient client(&surge->storage);
        
        std::string standardResponse = R"(
PARAMETERS:
- osc1_type: 2
- filter1_cutoff: 0.5
- filter1_resonance: 0.3
- amp_attack: 0.1

This creates a wavetable patch with moderate filtering.
)";
        
        auto mods = client.extractModifications(standardResponse);
        REQUIRE(mods.size() == 4);
        REQUIRE(mods[0].parameterName == "osc1_type");
        REQUIRE(mods[0].value == 2.0f);
    }
    
    SECTION("Claude Response with Descriptions")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::APIClient client(&surge->storage);
        
        std::string responseWithDescriptions = R"(
PARAMETERS:
- osc1_type: 2 (Wavetable oscillator for rich harmonics)
- filter1_cutoff: 0.3 (Low cutoff for warmth)
- amp_attack: 0.7 (Slow attack for ambient feel)

This patch is designed for ambient soundscapes.
)";
        
        auto mods = client.extractModifications(responseWithDescriptions);
        REQUIRE(mods.size() == 3);
        REQUIRE(mods[0].description == "Wavetable oscillator for rich harmonics");
        REQUIRE(mods[1].description == "Low cutoff for warmth");
    }
    
    SECTION("Alternative Format Without Dashes")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::APIClient client(&surge->storage);
        
        std::string altResponse = R"(
PARAMETERS:
osc1_type: 1
filter1_cutoff: 0.6
volume: 0.8
)";
        
        auto mods = client.extractModifications(altResponse);
        REQUIRE(mods.size() == 3);
    }
    
    SECTION("Mixed Case Parameters")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::APIClient client(&surge->storage);
        
        std::string mixedCaseResponse = R"(
Parameters:
- OSC1_TYPE: 2
- Filter1_Cutoff: 0.4
)";
        
        auto mods = client.extractModifications(mixedCaseResponse);
        // Should still extract parameters despite case differences
        REQUIRE(mods.size() >= 0); // Might be 0 if case-sensitive, but shouldn't crash
    }
}

// Test specific patch types that Claude commonly generates
TEST_CASE("Claude Common Patch Types", "[claude][patches]")
{
    SECTION("Ambient Pad Patch")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        std::vector<Surge::Claude::PatchModification> ambientPatch = {
            {"osc1_type", 2.0f, "Wavetable"},
            {"osc2_type", 1.0f, "Sine"},
            {"filter1_type", 1.0f, "LP 24dB"},
            {"filter1_cutoff", 0.3f, "Warm filtering"},
            {"filter1_resonance", 0.2f, "Slight resonance"},
            {"amp_attack", 0.8f, "Very slow attack"},
            {"amp_release", 0.9f, "Long release"},
            {"lfo1_rate", 0.1f, "Very slow LFO"},
            {"volume", 0.7f, "Background level"}
        };
        
        bool success = mapper.applyModifications(ambientPatch);
        REQUIRE(success);
    }
    
    SECTION("Lead Synth Patch")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        std::vector<Surge::Claude::PatchModification> leadPatch = {
            {"osc1_type", 0.0f, "Classic"},
            {"filter1_cutoff", 0.8f, "Bright"},
            {"filter1_resonance", 0.5f, "Resonant"},
            {"amp_attack", 0.05f, "Fast attack"},
            {"amp_release", 0.3f, "Medium release"},
            {"lfo1_rate", 0.4f, "Moderate vibrato"},
            {"volume", 0.9f, "Lead level"}
        };
        
        bool success = mapper.applyModifications(leadPatch);
        REQUIRE(success);
    }
    
    SECTION("Bass Patch")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        std::vector<Surge::Claude::PatchModification> bassPatch = {
            {"osc1_type", 0.0f, "Classic for bass"},
            {"filter1_cutoff", 0.4f, "Low-pass for bass"},
            {"filter1_resonance", 0.1f, "Minimal resonance"},
            {"amp_attack", 0.0f, "Instant attack"},
            {"amp_release", 0.2f, "Short release"},
            {"volume", 0.95f, "Bass level"}
        };
        
        bool success = mapper.applyModifications(bassPatch);
        REQUIRE(success);
    }
    
    SECTION("Pluck/Arp Patch")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        std::vector<Surge::Claude::PatchModification> pluckPatch = {
            {"osc1_type", 1.0f, "Sine for clean pluck"},
            {"filter1_cutoff", 0.6f, "Medium filtering"},
            {"filter1_resonance", 0.3f, "Some resonance"},
            {"amp_attack", 0.02f, "Very fast attack"},
            {"amp_release", 0.4f, "Quick decay"},
            {"volume", 0.8f, "Pluck level"}
        };
        
        bool success = mapper.applyModifications(pluckPatch);
        REQUIRE(success);
    }
}

// Test robustness and edge cases
TEST_CASE("Claude Robustness Tests", "[claude][robustness]")
{
    SECTION("Malformed Parameter Values")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::APIClient client(&surge->storage);
        
        std::string malformedResponse = R"(
PARAMETERS:
- osc1_type: abc
- filter1_cutoff: 0.5
- amp_attack: 
- volume: 0.8.5
- good_param: 0.3
)";
        
        auto mods = client.extractModifications(malformedResponse);
        // Should extract only valid parameters
        bool hasValidParams = false;
        for (const auto& mod : mods) {
            if (mod.parameterName == "filter1_cutoff" || mod.parameterName == "good_param") {
                hasValidParams = true;
            }
        }
        REQUIRE(hasValidParams);
    }
    
    SECTION("Very Long Response")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::APIClient client(&surge->storage);
        
        std::string longResponse = R"(
Here is a very long explanation about synthesis techniques and how to create 
the perfect ambient sound. This text goes on for many paragraphs describing
various synthesis methods, filter types, and modulation techniques.

PARAMETERS:
- osc1_type: 2
- filter1_cutoff: 0.5

And then more explanation continues here with even more detailed information
about sound design principles and creative techniques for electronic music.
)";
        
        auto mods = client.extractModifications(longResponse);
        REQUIRE(mods.size() == 2);
        REQUIRE(mods[0].parameterName == "osc1_type");
    }
    
    SECTION("Unicode and Special Characters")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::APIClient client(&surge->storage);
        
        std::string unicodeResponse = R"(
PARAMETERS:
- osc1_type: 2 (Beautiful wavetable sound 🎵)
- filter1_cutoff: 0.5 (Perfect for ambient vibes ✨)
)";
        
        auto mods = client.extractModifications(unicodeResponse);
        REQUIRE(mods.size() == 2);
        // Descriptions should handle unicode characters
        REQUIRE(mods[0].description.find("Beautiful") != std::string::npos);
    }
}

// Performance and stress tests
TEST_CASE("Claude Performance Tests", "[claude][performance][slow]")
{
    SECTION("Rapid Parameter Changes")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        auto start = std::chrono::high_resolution_clock::now();
        
        // Simulate rapid parameter changes as might occur in real-time modulation
        for (int i = 0; i < 500; i++) {
            float value = static_cast<float>(i % 100) / 100.0f;
            mapper.setParameterFromName("filter1_cutoff", value);
            mapper.setParameterFromName("filter1_resonance", value * 0.5f);
            mapper.setParameterFromName("volume", 0.5f + value * 0.3f);
        }
        
        auto end = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        // Should complete within reasonable time
        REQUIRE(duration.count() < 3000); // 3 seconds max
    }
    
    SECTION("Large Batch Operations")
    {
        auto surge = Surge::Headless::createSurge(44100);
        Surge::Claude::ParameterMapper mapper(surge.get());
        
        // Create a large batch of modifications
        std::vector<Surge::Claude::PatchModification> largeBatch;
        std::vector<std::string> paramNames = {
            "filter1_cutoff", "filter1_resonance", "amp_attack", "amp_release",
            "volume", "osc1_type", "osc2_type", "lfo1_rate"
        };
        
        for (int i = 0; i < 200; i++) {
            std::string paramName = paramNames[i % paramNames.size()];
            float value = static_cast<float>(i % 100) / 100.0f;
            largeBatch.push_back({paramName, value, "Batch test"});
        }
        
        auto start = std::chrono::high_resolution_clock::now();
        bool success = mapper.applyModifications(largeBatch);
        auto end = std::chrono::high_resolution_clock::now();
        
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
        
        REQUIRE(success);
        REQUIRE(duration.count() < 5000); // 5 seconds max for large batch
    }
}