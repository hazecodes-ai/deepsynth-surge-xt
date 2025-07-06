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

#ifndef SURGE_SRC_COMMON_CLAUDEAPICLIENT_H
#define SURGE_SRC_COMMON_CLAUDEAPICLIENT_H

#include <string>
#include <functional>
#include <memory>
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"

class SurgeStorage;

// Forward declare to avoid circular includes
namespace Surge {
namespace PatchDB {
class VectorDatabase;
struct PatchVector; // Forward declare PatchVector
}
}

namespace Surge
{
namespace Claude
{

struct PatchModification
{
    std::string parameterName;
    float value;
    std::string description;
};

struct ClaudeResponse
{
    bool success;
    std::string errorMessage;
    std::string responseText;
    std::vector<PatchModification> modifications;
};

class APIClient
{
  public:
    APIClient(SurgeStorage *storage);
    ~APIClient();

    void setAPIKey(const std::string &key);
    std::string getAPIKey() const;
    bool isAPIKeyValid() const;

    // Set RAG database for enhanced patch generation
    void setVectorDatabase(std::shared_ptr<Surge::PatchDB::VectorDatabase> db);

    void generatePatch(const std::string &prompt, 
                      std::function<void(const ClaudeResponse&)> callback);

    void modifyPatch(const std::string &prompt, 
                    const std::string &currentPatchXML,
                    std::function<void(const ClaudeResponse&)> callback);

    // Public for testing
    std::vector<PatchModification> extractModifications(const std::string &responseText);

  private:
    SurgeStorage *storage;
    std::string apiKey;
    std::unique_ptr<juce::WebInputStream> currentRequest;
    std::shared_ptr<Surge::PatchDB::VectorDatabase> vectorDatabase;

    void makeAPIRequest(const std::string &prompt, 
                       const std::string &context,
                       std::function<void(const ClaudeResponse&)> callback);
    
    // RAG helper methods
    std::string generateEnhancedPrompt(const std::string &userPrompt);
    std::vector<std::string> extractSearchTerms(const std::string &prompt);
    std::string formatSimilarPatches(const std::vector<Surge::PatchDB::PatchVector>& patches);

    ClaudeResponse parseResponse(const std::string &jsonResponse);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(APIClient)
};

} // namespace Claude
} // namespace Surge

#endif // SURGE_SRC_COMMON_CLAUDEAPICLIENT_H