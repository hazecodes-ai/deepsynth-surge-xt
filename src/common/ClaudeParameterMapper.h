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

#ifndef SURGE_SRC_COMMON_CLAUDEPARAMETERMAPPER_H
#define SURGE_SRC_COMMON_CLAUDEPARAMETERMAPPER_H

#include <string>
#include <unordered_map>
#include <vector>
#include "ClaudeAPIClient.h"

class SurgeSynthesizer;

namespace Surge
{
namespace Claude
{

class ParameterMapper
{
  public:
    ParameterMapper(SurgeSynthesizer *synthesizer);
    ~ParameterMapper();

    void buildParameterMaps();
    bool setParameterFromName(const std::string &paramName, float value);
    int findParameterIndex(const std::string &name);
    std::string exportCurrentPatchInfo();
    bool applyModifications(const std::vector<PatchModification> &modifications);

  private:
    SurgeSynthesizer *synth;
    std::unordered_map<std::string, int> nameToIndexMap;
    std::unordered_map<std::string, int> oscNameToIndexMap;
    std::unordered_map<std::string, int> aliasToIndexMap;
    
    void buildAliasMap();
    std::string toLower(const std::string &str);
    bool tryParameterVariations(const std::string &baseName, float value);
    
    // Common parameter aliases for natural language mapping
    static const std::unordered_map<std::string, std::string> parameterAliases;
};

} // namespace Claude
} // namespace Surge

#endif // SURGE_SRC_COMMON_CLAUDEPARAMETERMAPPER_H