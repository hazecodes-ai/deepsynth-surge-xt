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

#ifndef SURGE_SRC_COMMON_PATCHPARAMETEREXTRACTOR_H
#define SURGE_SRC_COMMON_PATCHPARAMETEREXTRACTOR_H

#include <vector>
#include <string>
#include <unordered_map>

class SurgeStorage;
class SurgePatch;

namespace Surge
{
namespace PatchDB
{

// Extracted parameter data from a patch
struct ExtractedPatchData
{
    std::string name;
    std::string category;
    std::string author;
    std::string comment;
    
    // Key parameters for similarity matching
    struct SceneData {
        // Oscillators
        int osc1_type = 0;
        int osc2_type = 0; 
        int osc3_type = 0;
        float osc1_pitch = 0.0f;
        float osc2_pitch = 0.0f;
        float osc3_pitch = 0.0f;
        
        // Filters
        int filter1_type = 0;
        int filter2_type = 0;
        float filter1_cutoff = 0.0f;
        float filter2_cutoff = 0.0f;
        float filter1_resonance = 0.0f;
        float filter2_resonance = 0.0f;
        
        // Envelopes
        float amp_attack = 0.0f;
        float amp_decay = 0.0f;
        float amp_sustain = 0.0f;
        float amp_release = 0.0f;
        
        float filter_attack = 0.0f;
        float filter_decay = 0.0f;
        float filter_sustain = 0.0f;
        float filter_release = 0.0f;
        
        // LFOs
        float lfo1_rate = 0.0f;
        float lfo2_rate = 0.0f;
        int lfo1_shape = 0;
        int lfo2_shape = 0;
    };
    
    SceneData sceneA;
    SceneData sceneB;
    
    // Global parameters
    float volume = 0.0f;
    
    // FX parameters (simplified)
    struct FXData {
        bool enabled = false;
        int type = 0;
        std::vector<float> params;
    };
    std::vector<FXData> fx;
    
    // Convert to normalized vector for similarity calculations
    std::vector<float> toNormalizedVector() const;
    
    // Get semantic description
    std::string getSemanticDescription() const;
    
    // Parameter importance weights for similarity calculations
    static std::unordered_map<std::string, float> getParameterWeights();
};

class PatchParameterExtractor
{
public:
    PatchParameterExtractor(SurgeStorage* storage);
    ~PatchParameterExtractor();
    
    // Extract parameters from FXP file
    bool extractFromFile(const std::string& fxpPath, ExtractedPatchData& outData);
    
    // Extract parameters from loaded patch
    bool extractFromPatch(const SurgePatch& patch, ExtractedPatchData& outData);
    
    // Batch extract from directory
    std::vector<ExtractedPatchData> extractFromDirectory(const std::string& dirPath);
    
    // Get error messages
    const std::string& getLastError() const { return lastError; }
    
private:
    SurgeStorage* storage;
    std::string lastError;
    
    // Helper functions
    bool loadFXPFile(const std::string& fxpPath, std::vector<char>& outData);
    void extractSceneParameters(const SurgePatch& patch, int scene, 
                               ExtractedPatchData::SceneData& outScene);
    void extractGlobalParameters(const SurgePatch& patch, ExtractedPatchData& outData);
    void extractFXParameters(const SurgePatch& patch, ExtractedPatchData& outData);
    
    // Parameter mapping helpers
    float normalizeParameter(int paramId, float value) const;
    std::string getOscillatorTypeName(int type) const;
    std::string getFilterTypeName(int type) const;
    std::string getLFOShapeName(int shape) const;
    std::string getFXTypeName(int type) const;
};

} // namespace PatchDB
} // namespace Surge

#endif // SURGE_SRC_COMMON_PATCHPARAMETEREXTRACTOR_H