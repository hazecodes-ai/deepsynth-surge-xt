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

#ifndef SURGE_SRC_SURGE_TESTRUNNER_CLAUDETESTUTILITIES_H
#define SURGE_SRC_SURGE_TESTRUNNER_CLAUDETESTUTILITIES_H

#include "ClaudeAPIClient.h"
#include "ClaudeParameterMapper.h"
#include "SurgeSynthesizer.h"
#include <string>
#include <vector>

namespace Surge
{
namespace Test
{
namespace Claude
{

/**
 * Utility class for Claude-related testing
 */
class TestUtils
{
  public:
    /**
     * Create a mock Claude response for testing
     */
    static std::string createMockClaudeResponse(
        const std::vector<std::pair<std::string, float>>& parameters,
        const std::string& description = "Test patch");

    /**
     * Verify that a parameter has been set to approximately the expected value
     */
    static bool verifyParameterValue(SurgeSynthesizer* synth,
                                   const std::string& parameterName,
                                   float expectedValue,
                                   float tolerance = 0.01f);

    /**
     * Get the current value of a parameter by name
     */
    static float getParameterValue(SurgeSynthesizer* synth,
                                 const std::string& parameterName);

    /**
     * Create a standard set of test modifications for various patch types
     */
    static std::vector<Surge::Claude::PatchModification> createAmbientPatchMods();
    static std::vector<Surge::Claude::PatchModification> createLeadPatchMods();
    static std::vector<Surge::Claude::PatchModification> createBassPatchMods();

    /**
     * Test data generators
     */
    static std::vector<std::string> getValidParameterNames();
    static std::vector<std::string> getInvalidParameterNames();
    static std::vector<float> getValidParameterValues();
    static std::vector<float> getInvalidParameterValues();

    /**
     * Performance testing utilities
     */
    static void benchmarkParameterApplication(SurgeSynthesizer* synth,
                                            const std::vector<Surge::Claude::PatchModification>& mods,
                                            int iterations = 100);

    /**
     * Validation utilities
     */
    static bool isValidOscillatorType(float value);
    static bool isValidFilterType(float value);
    static bool isValidNormalizedValue(float value);
};

} // namespace Claude
} // namespace Test
} // namespace Surge

#endif // SURGE_SRC_SURGE_TESTRUNNER_CLAUDETESTUTILITIES_H