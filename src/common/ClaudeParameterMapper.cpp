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

#include "ClaudeParameterMapper.h"
#include "SurgeSynthesizer.h"
#include "Parameter.h"
#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>

namespace Surge
{
namespace Claude
{

const std::unordered_map<std::string, std::string> ParameterMapper::parameterAliases = {
    // Oscillator aliases - based on actual Surge parameter names from console output
    // Scene A parameters (default when no scene is specified)
    {"osc1_type", "A Osc 1 Type"},
    {"osc2_type", "A Osc 2 Type"},
    {"osc3_type", "A Osc 3 Type"},
    {"osc1_pitch", "A Osc 1 Pitch"},
    {"osc2_pitch", "A Osc 2 Pitch"},
    {"osc3_pitch", "A Osc 3 Pitch"},
    {"osc1_volume", "A Osc 1 Volume"},
    {"osc2_volume", "A Osc 2 Volume"},
    {"osc3_volume", "A Osc 3 Volume"},
    {"oscillator_drift", "Osc Drift"},
    {"fm_depth", "FM Depth"},
    
    // Filter aliases - based on actual names
    {"filter1_type", "A Filter 1 Type"},
    {"filter2_type", "A Filter 2 Type"},
    {"filter1_cutoff", "A Filter 1 Cutoff"},
    {"filter2_cutoff", "A Filter 2 Cutoff"},
    {"filter1_resonance", "A Filter 1 Resonance"},
    {"filter2_resonance", "A Filter 2 Resonance"},
    {"filter_cutoff", "A Filter 1 Cutoff"},
    {"filter_resonance", "A Filter 1 Resonance"},
    {"filter_type", "A Filter 1 Type"},
    {"highpass", "A Highpass"},
    {"filter_feedback", "A Feedback"},
    
    // Envelope aliases - based on actual names
    {"amp_attack", "A Amp EG Attack"},
    {"amp_decay", "A Amp EG Decay"},
    {"amp_sustain", "A Amp EG Sustain"},
    {"amp_release", "A Amp EG Release"},
    {"filter_attack", "A Filter EG Attack"},
    {"filter_decay", "A Filter EG Decay"},
    {"filter_sustain", "A Filter EG Sustain"},
    {"filter_release", "A Filter EG Release"},
    
    // LFO aliases
    {"lfo1_rate", "A LFO 1 Rate"},
    {"lfo1_shape", "A LFO 1 Type"},
    {"lfo1_amount", "A LFO 1 Amplitude"},
    {"lfo2_rate", "A LFO 2 Rate"},
    {"lfo2_shape", "A LFO 2 Type"},
    {"lfo2_amount", "A LFO 2 Amplitude"},
    
    // Global aliases
    {"master_volume", "Global Volume"},
    {"volume", "A Volume"},
    {"amp_gain", "A VCA Gain"},
    {"pan", "A Pan"},
    {"width", "A Width"},
    
    // Effects aliases
    {"fx_reverb_mix", "Send FX 1 Return"},
    {"fx_delay_mix", "Send FX 2 Return"},
    {"fx_chorus_mix", "Send FX 1 Return"},
    {"reverb_mix", "Send FX 1 Return"},
    {"delay_mix", "Send FX 2 Return"},
    
    // Additional common aliases
    {"cutoff", "Filter 1 Cutoff"},
    {"resonance", "Filter 1 Resonance"},
    {"attack", "Amp EG Attack"},
    {"decay", "Amp EG Decay"},
    {"sustain", "Amp EG Sustain"},
    {"release", "Amp EG Release"},
    
    // Scene B specific aliases (when explicitly requested)
    {"scene_b_osc1_type", "B Osc 1 Type"},
    {"scene_b_osc2_type", "B Osc 2 Type"},
    {"scene_b_osc3_type", "B Osc 3 Type"},
    {"scene_b_filter1_cutoff", "B Filter 1 Cutoff"},
    {"scene_b_filter2_cutoff", "B Filter 2 Cutoff"},
    {"scene_b_filter1_resonance", "B Filter 1 Resonance"},
    {"scene_b_filter2_resonance", "B Filter 2 Resonance"},
    {"scene_b_amp_attack", "B Amp EG Attack"},
    {"scene_b_amp_decay", "B Amp EG Decay"},
    {"scene_b_amp_sustain", "B Amp EG Sustain"},
    {"scene_b_amp_release", "B Amp EG Release"}
};

ParameterMapper::ParameterMapper(SurgeSynthesizer *synthesizer) : synth(synthesizer)
{
    buildParameterMaps();
}

ParameterMapper::~ParameterMapper() = default;

void ParameterMapper::buildParameterMaps()
{
    nameToIndexMap.clear();
    oscNameToIndexMap.clear();
    aliasToIndexMap.clear();
    
    std::cout << "DEBUG: Building parameter maps. Total parameters: " << synth->storage.getPatch().param_ptr.size() << std::endl;
    
    for (int i = 0; i < synth->storage.getPatch().param_ptr.size(); ++i)
    {
        auto *param = synth->storage.getPatch().param_ptr[i];
        if (param)
        {
            // Full display name mapping
            char txt[256];
            synth->getParameterName(synth->idForParameter(param), txt);
            std::string fullName = txt;
            nameToIndexMap[fullName] = i;
            nameToIndexMap[toLower(fullName)] = i;
            
            // Print some common parameters for debugging
            if (fullName.find("Filter") != std::string::npos || 
                fullName.find("Cutoff") != std::string::npos ||
                fullName.find("Resonance") != std::string::npos ||
                fullName.find("Volume") != std::string::npos ||
                fullName.find("Osc") != std::string::npos)
            {
                std::cout << "DEBUG: Parameter[" << i << "]: " << fullName << std::endl;
            }
            
            // OSC name mapping
            std::string oscName = param->get_osc_name();
            if (!oscName.empty())
            {
                oscNameToIndexMap[oscName] = i;
                oscNameToIndexMap[toLower(oscName)] = i;
            }
            
            // Internal name mapping
            std::string internalName = param->get_name();
            if (!internalName.empty())
            {
                nameToIndexMap[internalName] = i;
                nameToIndexMap[toLower(internalName)] = i;
            }
        }
    }
    
    buildAliasMap();
}

void ParameterMapper::buildAliasMap()
{
    for (const auto &alias : parameterAliases)
    {
        // Try exact match first
        auto it = nameToIndexMap.find(alias.second);
        if (it != nameToIndexMap.end())
        {
            aliasToIndexMap[alias.first] = it->second;
            aliasToIndexMap[toLower(alias.first)] = it->second;
            std::cout << "DEBUG: Mapped alias '" << alias.first << "' to '" << alias.second << "' at index " << it->second << std::endl;
        }
        else
        {
            // If exact match fails, try to find the parameter in any scene
            bool found = false;
            for (const auto &pair : nameToIndexMap)
            {
                if (pair.first.find(alias.second) != std::string::npos)
                {
                    aliasToIndexMap[alias.first] = pair.second;
                    aliasToIndexMap[toLower(alias.first)] = pair.second;
                    std::cout << "DEBUG: Mapped alias '" << alias.first << "' to '" << pair.first << "' at index " << pair.second << std::endl;
                    found = true;
                    break;
                }
            }
            
            if (!found)
            {
                std::cout << "DEBUG: Could not find parameter for alias '" << alias.first << "' -> '" << alias.second << "'" << std::endl;
            }
        }
    }
}

bool ParameterMapper::setParameterFromName(const std::string &paramName, float value)
{
    std::cout << "DEBUG: Trying to set parameter: " << paramName << " = " << value << std::endl;
    
    int paramIndex = findParameterIndex(paramName);
    if (paramIndex >= 0)
    {
        std::cout << "DEBUG: Found parameter at index: " << paramIndex << std::endl;
        
        // Special handling for certain parameter types
        auto param = synth->storage.getPatch().param_ptr[paramIndex];
        if (param) {
            // Store old value for debugging
            float oldValue = param->get_value_f01();
            
            // For discrete parameters like oscillator types, use the value directly
            if (param->valtype == vt_int || paramName.find("_type") != std::string::npos)
            {
                std::cout << "DEBUG: Setting discrete parameter" << std::endl;
                int intValue = static_cast<int>(value);
                
                // Clamp to valid range
                if (param->val_max.i > param->val_min.i) {
                    intValue = std::max(param->val_min.i, std::min(param->val_max.i, intValue));
                }
                
                param->val.i = intValue;
                
                // Also update via the synth's parameter system
                synth->setParameter01(synth->idForParameter(param), param->get_value_f01(), false);
            }
            else
            {
                // For continuous parameters, ensure proper range
                float normalizedValue = value;
                
                // Check if value seems to already be normalized (0-1)
                if (value > 1.0f || value < 0.0f)
                {
                    // Assume it needs normalization
                    normalizedValue = param->value_to_normalized(value);
                }
                
                // Clamp to 0-1 range
                normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
                
                std::cout << "DEBUG: Setting continuous parameter with normalized value: " << normalizedValue << std::endl;
                
                // Set via the synth's parameter system to ensure proper notification
                synth->setParameter01(synth->idForParameter(param), normalizedValue, false);
            }
            
            float newValue = param->get_value_f01();
            std::cout << "DEBUG: Parameter changed from " << oldValue << " to " << newValue << std::endl;
            
            // Force parameter update
            synth->storage.getPatch().isDirty = true;
            return true;
        }
        return false;
    }
    
    // Try parameter variations if direct match fails
    return tryParameterVariations(paramName, value);
}

int ParameterMapper::findParameterIndex(const std::string &name)
{
    std::string lowerName = toLower(name);
    
    std::cout << "DEBUG: Looking for parameter: '" << name << "' (lower: '" << lowerName << "')" << std::endl;
    
    // 1. Try alias map first (most common Claude responses)
    auto aliasIt = aliasToIndexMap.find(lowerName);
    if (aliasIt != aliasToIndexMap.end())
    {
        std::cout << "DEBUG: Found in alias map at index " << aliasIt->second << std::endl;
        return aliasIt->second;
    }
    
    // 2. Try exact name match
    auto it = nameToIndexMap.find(name);
    if (it != nameToIndexMap.end())
    {
        std::cout << "DEBUG: Found exact match at index " << it->second << std::endl;
        return it->second;
    }
    
    // 3. Try lowercase name match
    auto lowerIt = nameToIndexMap.find(lowerName);
    if (lowerIt != nameToIndexMap.end())
    {
        std::cout << "DEBUG: Found lowercase match at index " << lowerIt->second << std::endl;
        return lowerIt->second;
    }
    
    // 4. Try OSC name match
    auto oscIt = oscNameToIndexMap.find(lowerName);
    if (oscIt != oscNameToIndexMap.end())
    {
        std::cout << "DEBUG: Found OSC name match at index " << oscIt->second << std::endl;
        return oscIt->second;
    }
    
    // 5. Try partial match - look for the first parameter that contains our search term
    std::cout << "DEBUG: Trying partial match..." << std::endl;
    
    // First try to find in Scene A (which is usually what we want)
    for (const auto &pair : nameToIndexMap)
    {
        std::string pairLower = toLower(pair.first);
        // Skip Scene B parameters when looking for generic names
        if (pairLower.find(" b ") != std::string::npos || pairLower.find("scene b") != std::string::npos)
            continue;
            
        if (pairLower.find(lowerName) != std::string::npos)
        {
            std::cout << "DEBUG: Found partial match '" << pair.first << "' at index " << pair.second << std::endl;
            return pair.second;
        }
    }
    
    // If not found in Scene A, try all parameters
    for (const auto &pair : nameToIndexMap)
    {
        if (toLower(pair.first).find(lowerName) != std::string::npos)
        {
            std::cout << "DEBUG: Found partial match '" << pair.first << "' at index " << pair.second << std::endl;
            return pair.second;
        }
    }
    
    std::cout << "DEBUG: Parameter not found!" << std::endl;
    return -1; // Not found
}

bool ParameterMapper::tryParameterVariations(const std::string &baseName, float value)
{
    std::string lower = toLower(baseName);
    
    // Common variations to try
    std::vector<std::string> variations = {
        "A " + baseName,
        "B " + baseName,
        baseName + " 1",
        baseName + " 2",
        "A " + baseName + " 1",
        "A " + baseName + " 2"
    };
    
    for (const auto &variation : variations)
    {
        int index = findParameterIndex(variation);
        if (index >= 0)
        {
            std::cout << "DEBUG: Found parameter variation: " << variation << " at index " << index << std::endl;
            
            auto param = synth->storage.getPatch().param_ptr[index];
            if (param) {
                // Store old value for debugging
                float oldValue = param->get_value_f01();
                
                // For discrete parameters like oscillator types, use the value directly
                if (param->valtype == vt_int || baseName.find("_type") != std::string::npos)
                {
                    std::cout << "DEBUG: Setting discrete parameter variation" << std::endl;
                    int intValue = static_cast<int>(value);
                    
                    // Clamp to valid range
                    if (param->val_max.i > param->val_min.i) {
                        intValue = std::max(param->val_min.i, std::min(param->val_max.i, intValue));
                    }
                    
                    param->val.i = intValue;
                    
                    // Also update via the synth's parameter system
                    synth->setParameter01(synth->idForParameter(param), param->get_value_f01(), false);
                }
                else
                {
                    // For continuous parameters, ensure proper range
                    float normalizedValue = value;
                    
                    // Check if value seems to already be normalized (0-1)
                    if (value > 1.0f || value < 0.0f)
                    {
                        // Assume it needs normalization
                        normalizedValue = param->value_to_normalized(value);
                    }
                    
                    // Clamp to 0-1 range
                    normalizedValue = std::max(0.0f, std::min(1.0f, normalizedValue));
                    
                    std::cout << "DEBUG: Setting continuous parameter variation with normalized value: " << normalizedValue << std::endl;
                    
                    // Set via the synth's parameter system to ensure proper notification
                    synth->setParameter01(synth->idForParameter(param), normalizedValue, false);
                }
                
                float newValue = param->get_value_f01();
                std::cout << "DEBUG: Parameter variation changed from " << oldValue << " to " << newValue << std::endl;
                
                // Force parameter update
                synth->storage.getPatch().isDirty = true;
                return true;
            }
            return false;
        }
    }
    
    std::cout << "DEBUG: No parameter variations found for: " << baseName << std::endl;
    return false;
}

std::string ParameterMapper::exportCurrentPatchInfo()
{
    std::ostringstream oss;
    oss << "Current Surge XT Patch Parameters:\n\n";
    
    // Group parameters by category
    std::vector<std::string> categories = {"Global", "Oscillators", "Mixer", "Filters", "Envelopes", "Modulators", "FX"};
    
    for (const auto &category : categories)
    {
        oss << "=== " << category << " ===\n";
        
        for (int i = 0; i < synth->storage.getPatch().param_ptr.size(); ++i)
        {
            auto *param = synth->storage.getPatch().param_ptr[i];
            if (param)
            {
                std::string ctrlGroupName = "";
                switch (param->ctrlgroup)
                {
                case cg_GLOBAL: ctrlGroupName = "Global"; break;
                case cg_OSC: ctrlGroupName = "Oscillators"; break;
                case cg_MIX: ctrlGroupName = "Mixer"; break;
                case cg_FILTER: ctrlGroupName = "Filters"; break;
                case cg_ENV: ctrlGroupName = "Envelopes"; break;
                case cg_LFO: ctrlGroupName = "Modulators"; break;
                case cg_FX: ctrlGroupName = "FX"; break;
                default: continue;
                }
                
                if (ctrlGroupName == category)
                {
                    char txt[256];
                    auto param = synth->storage.getPatch().param_ptr[i];
                    if (!param) continue;
                    std::string paramName = param->get_full_name();
                    float value = param->get_value_f01();
                    
                    oss << "- " << paramName << ": " << value << "\n";
                }
            }
        }
        oss << "\n";
    }
    
    return oss.str();
}

bool ParameterMapper::applyModifications(const std::vector<PatchModification> &modifications)
{
    bool allSuccess = true;
    int successCount = 0;
    
    std::cout << "DEBUG: Applying " << modifications.size() << " modifications to synth" << std::endl;
    
    for (const auto &mod : modifications)
    {
        bool success = setParameterFromName(mod.parameterName, mod.value);
        if (!success)
        {
            std::cout << "Warning: Could not apply parameter modification: " 
                      << mod.parameterName << " = " << mod.value 
                      << " (" << mod.description << ")" << std::endl;
            allSuccess = false;
        }
        else
        {
            std::cout << "Applied: " << mod.parameterName << " = " << mod.value 
                      << " (" << mod.description << ")" << std::endl;
            successCount++;
        }
    }
    
    std::cout << "DEBUG: Successfully applied " << successCount << " out of " << modifications.size() << " modifications" << std::endl;
    
    // Always mark patch as dirty if we applied any changes
    if (successCount > 0)
    {
        synth->storage.getPatch().isDirty = true;
        
        std::cout << "DEBUG: Marked patch as dirty" << std::endl;
    }
    
    return allSuccess;
}

std::string ParameterMapper::toLower(const std::string &str)
{
    std::string result = str;
    std::transform(result.begin(), result.end(), result.begin(), 
                   [](unsigned char c) { return std::tolower(c); });
    return result;
}

} // namespace Claude
} // namespace Surge