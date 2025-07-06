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

#include "PatchParameterExtractor.h"
#include "SurgeStorage.h"
#include "Parameter.h"
#include "PatchFileHeaderStructs.h"
#include "filesystem/import.h"
#include <fstream>
#include <iostream>
#include <sstream>

using namespace sst::basic_blocks::mechanics;

namespace Surge
{
namespace PatchDB
{

std::vector<float> ExtractedPatchData::toNormalizedVector() const
{
    std::vector<float> vec;
    vec.reserve(50); // Estimated size
    
    // Scene A parameters
    vec.push_back(sceneA.osc1_type / 15.0f);      // Normalized osc types
    vec.push_back(sceneA.osc2_type / 15.0f);
    vec.push_back(sceneA.osc3_type / 15.0f);
    vec.push_back((sceneA.osc1_pitch + 60.0f) / 120.0f); // -60 to +60 semitones
    vec.push_back((sceneA.osc2_pitch + 60.0f) / 120.0f);
    vec.push_back((sceneA.osc3_pitch + 60.0f) / 120.0f);
    
    vec.push_back(sceneA.filter1_type / 12.0f);   // Normalized filter types
    vec.push_back(sceneA.filter2_type / 12.0f);
    vec.push_back(sceneA.filter1_cutoff);         // Already 0-1
    vec.push_back(sceneA.filter2_cutoff);
    vec.push_back(sceneA.filter1_resonance);
    vec.push_back(sceneA.filter2_resonance);
    
    vec.push_back(sceneA.amp_attack);             // Already 0-1
    vec.push_back(sceneA.amp_decay);
    vec.push_back(sceneA.amp_sustain);
    vec.push_back(sceneA.amp_release);
    
    vec.push_back(sceneA.filter_attack);
    vec.push_back(sceneA.filter_decay);
    vec.push_back(sceneA.filter_sustain);
    vec.push_back(sceneA.filter_release);
    
    vec.push_back(sceneA.lfo1_rate);
    vec.push_back(sceneA.lfo2_rate);
    vec.push_back(sceneA.lfo1_shape / 8.0f);      // Normalized LFO shapes
    vec.push_back(sceneA.lfo2_shape / 8.0f);
    
    // Scene B parameters (simplified - just key differences)
    vec.push_back(sceneB.osc1_type / 15.0f);
    vec.push_back(sceneB.filter1_cutoff);
    vec.push_back(sceneB.amp_attack);
    vec.push_back(sceneB.amp_release);
    
    // Global parameters
    vec.push_back(volume);
    
    // FX presence (simplified)
    for (size_t i = 0; i < std::min(fx.size(), size_t(8)); ++i)
    {
        vec.push_back(fx[i].enabled ? 1.0f : 0.0f);
        if (!fx[i].params.empty())
            vec.push_back(fx[i].params[0]); // First parameter only
        else
            vec.push_back(0.0f);
    }
    
    // Pad to consistent size if needed
    while (vec.size() < 50)
        vec.push_back(0.0f);
    
    return vec;
}

std::string ExtractedPatchData::getSemanticDescription() const
{
    std::ostringstream desc;
    
    // Basic categorization
    desc << "A " << category << " sound";
    
    // Oscillator description
    if (sceneA.osc1_type == 2) desc << " using wavetable synthesis";
    else if (sceneA.osc1_type >= 4) desc << " with FM synthesis";
    else desc << " with classic oscillators";
    
    // Filter description
    if (sceneA.filter1_cutoff < 0.3f) desc << ", heavily filtered";
    else if (sceneA.filter1_cutoff > 0.8f) desc << ", bright and open";
    
    if (sceneA.filter1_resonance > 0.7f) desc << " with high resonance";
    
    // Envelope description
    if (sceneA.amp_attack < 0.05f && sceneA.amp_release < 0.3f) desc << ", percussive";
    else if (sceneA.amp_attack > 0.3f) desc << ", with slow attack";
    
    if (sceneA.amp_release > 0.7f) desc << " and long release";
    
    // LFO description
    if (sceneA.lfo1_rate > 0.5f) desc << ", with fast modulation";
    else if (sceneA.lfo1_rate > 0.1f) desc << ", with gentle modulation";
    
    return desc.str();
}

std::unordered_map<std::string, float> ExtractedPatchData::getParameterWeights()
{
    return {
        {"osc_type", 2.0f},           // Very important for character
        {"filter_cutoff", 1.8f},      // Key for timbre
        {"filter_type", 1.5f},        // Important for character
        {"amp_attack", 1.2f},         // Important for feel
        {"amp_release", 1.2f},
        {"filter_resonance", 1.0f},   // Moderate importance
        {"lfo_rate", 0.8f},          // Less critical
        {"pitch", 0.5f},             // Often varied in use
        {"volume", 0.3f}             // Usually adjusted
    };
}

PatchParameterExtractor::PatchParameterExtractor(SurgeStorage* storage) 
    : storage(storage)
{
}

PatchParameterExtractor::~PatchParameterExtractor() = default;

bool PatchParameterExtractor::extractFromFile(const std::string& fxpPath, ExtractedPatchData& outData)
{
    lastError.clear();
    
    try
    {
        // Read the entire FXP file
        std::ifstream file(fxpPath, std::ios::binary);
        if (!file.is_open())
        {
            lastError = "Cannot open file: " + fxpPath;
            return false;
        }
        
        // Get file size
        file.seekg(0, std::ios::end);
        size_t fileSize = file.tellg();
        file.seekg(0, std::ios::beg);
        
        // Skip files that are too large (likely corrupted)
        const size_t MAX_PATCH_SIZE = 4 * 1024 * 1024; // 4MB limit
        if (fileSize > MAX_PATCH_SIZE)
        {
            lastError = "File too large (possibly corrupted): " + fxpPath + " (" + std::to_string(fileSize) + " bytes)";
            return false;
        }
        
        // Read file data
        std::vector<char> fileData(fileSize);
        file.read(fileData.data(), fileSize);
        file.close();
        
        if (fileSize == 0)
        {
            lastError = "Empty file: " + fxpPath;
            return false;
        }
        
        // Create a temporary patch object to load the data
        SurgePatch tempPatch(storage);
        
        // Load the patch data using Surge's existing loader
        tempPatch.load_patch(fileData.data(), static_cast<int>(fileSize), true);
        
        // Extract parameters from the loaded patch
        bool success = extractFromPatch(tempPatch, outData);
        
        // If patch name is empty, use filename as fallback
        if (success && outData.name.empty())
        {
            auto pathObj = fs::path(fxpPath);
            outData.name = pathObj.stem().string();
        }
        
        // If category is empty, use parent directory as fallback
        if (success && outData.category.empty())
        {
            auto pathObj = fs::path(fxpPath);
            outData.category = pathObj.parent_path().filename().string();
        }
        
        return success;
    }
    catch (const std::exception& e)
    {
        lastError = "Error extracting from file: " + std::string(e.what());
        return false;
    }
}

bool PatchParameterExtractor::extractFromPatch(const SurgePatch& patch, ExtractedPatchData& outData)
{
    lastError.clear();
    
    try
    {
        // Extract metadata
        outData.name = patch.name;
        outData.category = patch.category;
        outData.author = patch.author;
        outData.comment = patch.comment;
        
        // Extract scene parameters
        extractSceneParameters(patch, 0, outData.sceneA);
        extractSceneParameters(patch, 1, outData.sceneB);
        
        // Extract global parameters
        extractGlobalParameters(patch, outData);
        
        // Extract FX parameters
        extractFXParameters(patch, outData);
        
        return true;
    }
    catch (const std::exception& e)
    {
        lastError = "Error extracting patch parameters: " + std::string(e.what());
        return false;
    }
}

bool PatchParameterExtractor::loadFXPFile(const std::string& fxpPath, std::vector<char>& outData)
{
    // TODO: Implement proper FXP loading
    // For now, just check if file exists
    std::ifstream file(fxpPath, std::ios::binary);
    if (!file.is_open())
    {
        lastError = "Cannot open file: " + fxpPath;
        return false;
    }
    
    // Return dummy data for testing
    outData.clear();
    return true;
}

void PatchParameterExtractor::extractSceneParameters(const SurgePatch& patch, int scene, 
                                                    ExtractedPatchData::SceneData& outScene)
{
    // Extract oscillator parameters
    outScene.osc1_type = patch.scene[scene].osc[0].type.val.i;
    outScene.osc2_type = patch.scene[scene].osc[1].type.val.i;
    outScene.osc3_type = patch.scene[scene].osc[2].type.val.i;
    
    outScene.osc1_pitch = patch.scene[scene].osc[0].pitch.val.f;
    outScene.osc2_pitch = patch.scene[scene].osc[1].pitch.val.f;
    outScene.osc3_pitch = patch.scene[scene].osc[2].pitch.val.f;
    
    // Extract filter parameters
    outScene.filter1_type = patch.scene[scene].filterunit[0].type.val.i;
    outScene.filter2_type = patch.scene[scene].filterunit[1].type.val.i;
    
    outScene.filter1_cutoff = patch.scene[scene].filterunit[0].cutoff.val.f;
    outScene.filter2_cutoff = patch.scene[scene].filterunit[1].cutoff.val.f;
    outScene.filter1_resonance = patch.scene[scene].filterunit[0].resonance.val.f;
    outScene.filter2_resonance = patch.scene[scene].filterunit[1].resonance.val.f;
    
    // Extract envelope parameters
    outScene.amp_attack = patch.scene[scene].adsr[0].a.val.f;
    outScene.amp_decay = patch.scene[scene].adsr[0].d.val.f;
    outScene.amp_sustain = patch.scene[scene].adsr[0].s.val.f;
    outScene.amp_release = patch.scene[scene].adsr[0].r.val.f;
    
    outScene.filter_attack = patch.scene[scene].adsr[1].a.val.f;
    outScene.filter_decay = patch.scene[scene].adsr[1].d.val.f;
    outScene.filter_sustain = patch.scene[scene].adsr[1].s.val.f;
    outScene.filter_release = patch.scene[scene].adsr[1].r.val.f;
    
    // Extract LFO parameters
    outScene.lfo1_rate = patch.scene[scene].lfo[0].rate.val.f;
    outScene.lfo2_rate = patch.scene[scene].lfo[1].rate.val.f;
    outScene.lfo1_shape = patch.scene[scene].lfo[0].shape.val.i;
    outScene.lfo2_shape = patch.scene[scene].lfo[1].shape.val.i;
}

void PatchParameterExtractor::extractGlobalParameters(const SurgePatch& patch, ExtractedPatchData& outData)
{
    outData.volume = patch.volume.val.f;
}

void PatchParameterExtractor::extractFXParameters(const SurgePatch& patch, ExtractedPatchData& outData)
{
    outData.fx.clear();
    outData.fx.resize(8); // Surge has 8 FX slots
    
    for (int i = 0; i < 8; ++i)
    {
        outData.fx[i].enabled = (patch.fx[i].type.val.i > 0);
        outData.fx[i].type = patch.fx[i].type.val.i;
        
        // Extract first few parameters (simplified)
        for (int p = 0; p < std::min(4, n_fx_params); ++p)
        {
            outData.fx[i].params.push_back(patch.fx[i].p[p].val.f);
        }
    }
}

std::vector<ExtractedPatchData> PatchParameterExtractor::extractFromDirectory(const std::string& dirPath)
{
    std::vector<ExtractedPatchData> results;
    lastError.clear();
    
    try
    {
        if (!fs::exists(dirPath) || !fs::is_directory(dirPath))
        {
            lastError = "Directory does not exist: " + dirPath;
            return results;
        }
        
        for (const auto& entry : fs::recursive_directory_iterator(dirPath))
        {
            if (entry.is_regular_file() && entry.path().extension() == ".fxp")
            {
                ExtractedPatchData data;
                if (extractFromFile(entry.path().string(), data))
                {
                    results.push_back(std::move(data));
                }
                else
                {
                    std::cout << "Warning: Failed to extract " << entry.path() 
                             << " - " << lastError << std::endl;
                }
            }
        }
    }
    catch (const std::exception& e)
    {
        lastError = "Error scanning directory: " + std::string(e.what());
    }
    
    return results;
}

} // namespace PatchDB
} // namespace Surge