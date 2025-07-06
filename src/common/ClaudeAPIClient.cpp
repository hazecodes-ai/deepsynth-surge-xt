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

#include "ClaudeAPIClient.h"
#include "PatchVectorDB.h"
#include "SurgeStorage.h"
#include "UserDefaults.h"
#include "juce_core/juce_core.h"
#include "juce_events/juce_events.h"
#include <regex>
#include <algorithm>
#include <sstream>

using namespace Surge::Claude;

APIClient::APIClient(SurgeStorage *storage) : storage(storage)
{
    // Load API key from user defaults
    apiKey = Surge::Storage::getUserDefaultValue(storage, Surge::Storage::ClaudeAPIKey, "");
}

APIClient::~APIClient() = default;

void APIClient::setVectorDatabase(std::shared_ptr<Surge::PatchDB::VectorDatabase> db)
{
    vectorDatabase = db;
}

void APIClient::setAPIKey(const std::string &key)
{
    apiKey = key;
    Surge::Storage::updateUserDefaultValue(storage, Surge::Storage::ClaudeAPIKey, key);
}

std::string APIClient::getAPIKey() const
{
    return apiKey;
}

bool APIClient::isAPIKeyValid() const
{
    // For debugging, temporarily accept any non-empty key
    return !apiKey.empty();
    // Original validation:
    // return !apiKey.empty() && apiKey.length() > 10 && apiKey.find("sk-ant-") == 0;
}

void APIClient::generatePatch(const std::string &prompt, 
                             std::function<void(const ClaudeResponse&)> callback)
{
    // Use RAG-enhanced prompt if vector database is available
    std::string enhancedPrompt = vectorDatabase ? generateEnhancedPrompt(prompt) : prompt;
    
    std::string context = R"(
You are a Surge XT synthesizer patch designer. Create a patch based on the user's description.

IMPORTANT: You MUST respond with EXACTLY this format (include the dash before each parameter):

PARAMETERS:
- filter1_cutoff: 0.5
- filter1_resonance: 0.3
- osc1_type: 2
- amp_attack: 0.1

Available parameters (use these exact names):
- osc1_type, osc2_type, osc3_type (integer 0-15 for oscillator types where 0=Classic, 1=Sine, 2=Wavetable, 3=Window, 4=FM2, 5=FM3, etc)
- osc1_pitch, osc2_pitch, osc3_pitch (-60.0 to 60.0 semitones)
- filter1_type, filter2_type (integer 0-12 where 0=LP 12dB, 1=LP 24dB, 2=LP Ladder, 3=HP 12dB, 4=HP 24dB, 5=BP, 6=Notch, 7=Comb, etc)
- filter1_cutoff, filter2_cutoff (0.0 to 1.0)
- filter1_resonance, filter2_resonance (0.0 to 1.0)
- amp_attack, amp_decay, amp_sustain, amp_release (0.0 to 1.0)
- filter_attack, filter_decay, filter_sustain, filter_release (0.0 to 1.0)
- lfo1_rate, lfo2_rate (0.0 to 1.0)
- lfo1_shape, lfo2_shape (integer 0-8)
- amp_gain (-48.0 to 48.0)
- volume (0.0 to 1.0)

Provide 5-10 parameter changes. Use normalized values (0.0-1.0) for continuous parameters.

User request: )";
    
    makeAPIRequest(enhancedPrompt, context, callback);
}

void APIClient::modifyPatch(const std::string &prompt, 
                           const std::string &currentPatchXML,
                           std::function<void(const ClaudeResponse&)> callback)
{
    std::string context = R"(
You are modifying an existing Surge XT synthesizer patch.
Suggest specific parameter changes based on the user's request.

IMPORTANT: You MUST respond with EXACTLY this format (include the dash before each parameter):

PARAMETERS:
- filter1_cutoff: 0.5
- filter1_resonance: 0.3
- osc1_type: 2
- amp_attack: 0.1

Available parameters (use these exact names):
- osc1_type, osc2_type, osc3_type (integer 0-15)
- osc1_pitch, osc2_pitch, osc3_pitch (-60.0 to 60.0)
- filter1_type, filter2_type (integer 0-12)
- filter1_cutoff, filter2_cutoff (0.0 to 1.0)
- filter1_resonance, filter2_resonance (0.0 to 1.0)
- amp_attack, amp_decay, amp_sustain, amp_release (0.0 to 1.0)
- filter_attack, filter_decay, filter_sustain, filter_release (0.0 to 1.0)
- lfo1_rate, lfo2_rate (0.0 to 1.0)
- volume (0.0 to 1.0)

User modification request: )";

    makeAPIRequest(prompt, context, callback);
}

void APIClient::makeAPIRequest(const std::string &prompt, 
                              const std::string &context,
                              std::function<void(const ClaudeResponse&)> callback)
{
    std::cout << "DEBUG: makeAPIRequest called with prompt: " << prompt << std::endl;
    std::cout << "DEBUG: API key valid: " << isAPIKeyValid() << std::endl;
    std::cout << "DEBUG: API key: " << apiKey.substr(0, 10) << "..." << std::endl;
    
    if (!isAPIKeyValid())
    {
        std::cout << "DEBUG: API key validation failed" << std::endl;
        ClaudeResponse response;
        response.success = false;
        response.errorMessage = "Invalid API key. Please set a valid Claude API key in settings.";
        callback(response);
        return;
    }

    // Create the request JSON properly
    juce::String fullPrompt = juce::String(context) + juce::String(prompt);
    
    // Properly escape the content for JSON
    juce::String escapedPrompt = fullPrompt.replace("\\", "\\\\")
                                          .replace("\"", "\\\"")
                                          .replace("\n", "\\n")
                                          .replace("\r", "\\r")
                                          .replace("\t", "\\t");
    
    juce::String jsonRequest = "{\"model\":\"claude-3-5-sonnet-20241022\","
                              "\"max_tokens\":2048,"
                              "\"messages\":[{\"role\":\"user\",\"content\":\"" + 
                              escapedPrompt + "\"}]}";
    
    // Create URL
    juce::URL url("https://api.anthropic.com/v1/messages");
    
    std::cout << "DEBUG: Making HTTP request to Claude API" << std::endl;
    std::cout << "DEBUG: JSON Request: " << jsonRequest.toStdString() << std::endl;
    
    // Make the HTTP request
    juce::Thread::launch([jsonRequest, callback, this]() {
        std::cout << "DEBUG: HTTP thread started" << std::endl;
        
        ClaudeResponse response;
        
        try {
            // Create URL with headers
            juce::URL url("https://api.anthropic.com/v1/messages");
            url = url.withPOSTData(jsonRequest);
            
            // JUCE uses headers string for custom headers
            juce::String headersString = 
                "Content-Type: application/json\r\n"
                "x-api-key: " + juce::String(apiKey) + "\r\n"
                "anthropic-version: 2023-06-01\r\n";
            
            std::cout << "DEBUG: Making request to: " << url.toString(true).toStdString() << std::endl;
            
            // Create input stream with timeout
            int statusCode = 0;
            juce::StringPairArray responseHeaders;
            auto stream = url.createInputStream(false, nullptr, nullptr, headersString, 30000, &responseHeaders, &statusCode);
            
            if (stream != nullptr)
            {
                juce::String responseString = stream->readEntireStreamAsString();
                std::cout << "DEBUG: Response status code: " << statusCode << std::endl;
                std::cout << "DEBUG: Response length: " << responseString.length() << std::endl;
                std::cout << "DEBUG: First 200 chars: " << responseString.substring(0, 200).toStdString() << std::endl;
                
                // Parse JSON response
                if (statusCode == 200 && responseString.contains("\"content\""))
                {
                    std::cout << "DEBUG: Full response: " << responseString.toStdString() << std::endl;
                    
                    // Claude's response format: {"content":[{"text":"...","type":"text"}],...}
                    // Look for the text content more carefully
                    int contentArrayStart = responseString.indexOf("\"content\":[");
                    if (contentArrayStart >= 0)
                    {
                        // Find the text field within the content array
                        int searchFrom = contentArrayStart + 11; // Skip past "content":["
                        int textFieldStart = responseString.indexOf(searchFrom, juce::String("\"text\":"));
                        
                        if (textFieldStart >= 0)
                        {
                            // Find the actual text value start (after "text":")
                            int textValueStart = textFieldStart + 7; // Skip "text":"
                            
                            // Handle both quoted and unquoted text
                            if (responseString[textValueStart] == '"')
                            {
                                textValueStart++; // Skip opening quote
                                
                                // Find the closing quote, handling escaped quotes
                                int textEnd = textValueStart;
                                bool escaped = false;
                                while (textEnd < responseString.length())
                                {
                                    if (!escaped && responseString[textEnd] == '"')
                                    {
                                        break;
                                    }
                                    escaped = (!escaped && responseString[textEnd] == '\\');
                                    textEnd++;
                                }
                                
                                if (textEnd < responseString.length())
                                {
                                    juce::String textContent = responseString.substring(textValueStart, textEnd);
                                    
                                    // Unescape JSON string
                                    textContent = textContent.replace("\\n", "\n")
                                                           .replace("\\r", "\r")
                                                           .replace("\\t", "\t")
                                                           .replace("\\\"", "\"")
                                                           .replace("\\\\/", "/")
                                                           .replace("\\\\", "\\");
                                    
                                    response.success = true;
                                    response.responseText = textContent.toStdString();
                                    response.modifications = extractModifications(response.responseText);
                                    std::cout << "DEBUG: Successfully parsed response" << std::endl;
                                    std::cout << "DEBUG: Response text: " << response.responseText << std::endl;
                                }
                                else
                                {
                                    response.success = false;
                                    response.errorMessage = "Failed to find end of text content";
                                }
                            }
                            else
                            {
                                response.success = false;
                                response.errorMessage = "Unexpected text format in response";
                            }
                        }
                        else
                        {
                            response.success = false;
                            response.errorMessage = "No text field found in content array";
                        }
                    }
                    else
                    {
                        response.success = false;
                        response.errorMessage = "No content array found in response";
                    }
                }
                else if (statusCode >= 400 || responseString.contains("\"error\":"))
                {
                    response.success = false;
                    
                    // Log the full error response for debugging
                    std::cout << "DEBUG: Full error response: " << responseString.toStdString() << std::endl;
                    
                    // Try to extract error message
                    int errorMsgStart = responseString.indexOf("\"message\":\"") + 11;
                    int errorMsgEnd = responseString.indexOfChar('"', errorMsgStart);
                    
                    if (errorMsgStart > 10 && errorMsgEnd > errorMsgStart)
                    {
                        response.errorMessage = responseString.substring(errorMsgStart, errorMsgEnd).toStdString();
                    }
                    else
                    {
                        // Try alternative error format
                        int errorStart = responseString.indexOf("\"error\":");
                        if (errorStart >= 0)
                        {
                            response.errorMessage = "API error (status " + std::to_string(statusCode) + "): " + 
                                                  responseString.substring(errorStart).toStdString();
                        }
                        else
                        {
                            response.errorMessage = "API error occurred (status: " + std::to_string(statusCode) + ")";
                        }
                    }
                    std::cout << "DEBUG: API Error: " << response.errorMessage << std::endl;
                }
                else
                {
                    response.success = false;
                    response.errorMessage = "Unexpected API response (status: " + std::to_string(statusCode) + ")";
                    std::cout << "DEBUG: Unexpected response format" << std::endl;
                }
            }
            else
            {
                response.success = false;
                response.errorMessage = "Failed to connect to Claude API - check your internet connection";
                std::cout << "DEBUG: Failed to create input stream" << std::endl;
            }
        }
        catch (const std::exception& e)
        {
            response.success = false;
            response.errorMessage = std::string("Exception: ") + e.what();
            std::cout << "DEBUG: Exception caught: " << e.what() << std::endl;
        }
        
        // Call the callback on the message thread
        juce::MessageManager::callAsync([callback, response]() {
            callback(response);
        });
    });
}

ClaudeResponse APIClient::parseResponse(const std::string &jsonResponse)
{
    ClaudeResponse response;
    response.success = true;
    response.responseText = jsonResponse;
    response.modifications = extractModifications(jsonResponse);
    return response;
}

std::vector<PatchModification> APIClient::extractModifications(const std::string &responseText)
{
    std::vector<PatchModification> modifications;
    
    std::cout << "DEBUG: Extracting modifications from response text" << std::endl;
    std::cout << "DEBUG: Full response for parameter extraction:\n" << responseText << std::endl;
    
    // Look for PARAMETERS: section
    size_t parametersPos = responseText.find("PARAMETERS:");
    if (parametersPos == std::string::npos)
    {
        std::cout << "DEBUG: No PARAMETERS: section found" << std::endl;
        // Try case-insensitive search
        std::string lowerResponse = responseText;
        std::transform(lowerResponse.begin(), lowerResponse.end(), lowerResponse.begin(), ::tolower);
        size_t lowerPos = lowerResponse.find("parameters:");
        if (lowerPos != std::string::npos)
        {
            parametersPos = lowerPos;
            std::cout << "DEBUG: Found lowercase 'parameters:' at position " << parametersPos << std::endl;
        }
        else
        {
            return modifications;
        }
    }

    std::string parametersSection = responseText.substr(parametersPos);
    std::cout << "DEBUG: Found parameters section: " << parametersSection << std::endl;
    
    // Parse parameter lines using regex - match lines like "- param_name: value (description)" or "- param_name: value"
    // Make the description part optional
    // Allow more flexible parameter names (with spaces, numbers) and value formats
    std::regex paramRegex(R"(-\s*([a-zA-Z0-9_\s]+):\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)(?:\s*\(([^)]*)\))?)");
    
    // Also try alternative formats without dash
    std::regex altParamRegex(R"(([a-zA-Z0-9_\s]+):\s*([-+]?[0-9]*\.?[0-9]+(?:[eE][-+]?[0-9]+)?)(?:\s*\(([^)]*)\))?)");
    
    std::sregex_iterator iter(parametersSection.begin(), parametersSection.end(), paramRegex);
    std::sregex_iterator end;

    int count = 0;
    for (; iter != end; ++iter)
    {
        const std::smatch &match = *iter;
        if (match.size() >= 3)  // At least parameter name and value
        {
            PatchModification mod;
            std::string paramName = match[1].str();
            // Trim whitespace from parameter name
            paramName.erase(0, paramName.find_first_not_of(" \t\r\n"));
            paramName.erase(paramName.find_last_not_of(" \t\r\n") + 1);
            mod.parameterName = paramName;
            mod.value = std::stof(match[2].str());
            mod.description = (match.size() > 3 && match[3].matched) ? match[3].str() : "";
            modifications.push_back(mod);
            
            std::cout << "DEBUG: Extracted parameter " << ++count << ": " 
                      << mod.parameterName << " = " << mod.value 
                      << " (" << mod.description << ")" << std::endl;
        }
    }
    
    // If no parameters found with dash format, try without dash
    if (modifications.empty())
    {
        std::cout << "DEBUG: No parameters found with dash format, trying alternative format" << std::endl;
        std::sregex_iterator altIter(parametersSection.begin(), parametersSection.end(), altParamRegex);
        
        for (; altIter != end; ++altIter)
        {
            const std::smatch &match = *altIter;
            if (match.size() >= 3)  // At least parameter name and value
            {
                PatchModification mod;
                std::string paramName = match[1].str();
                // Trim whitespace from parameter name
                paramName.erase(0, paramName.find_first_not_of(" \t\r\n"));
                paramName.erase(paramName.find_last_not_of(" \t\r\n") + 1);
                
                // Skip the line if it's the "PARAMETERS:" header itself
                if (paramName == "PARAMETERS" || paramName.empty())
                    continue;
                    
                mod.parameterName = paramName;
                mod.value = std::stof(match[2].str());
                mod.description = (match.size() > 3 && match[3].matched) ? match[3].str() : "";
                modifications.push_back(mod);
                
                std::cout << "DEBUG: Extracted parameter (alt format) " << ++count << ": " 
                          << mod.parameterName << " = " << mod.value 
                          << " (" << mod.description << ")" << std::endl;
            }
        }
    }
    
    std::cout << "DEBUG: Total parameters extracted: " << modifications.size() << std::endl;
    return modifications;
}

std::string APIClient::generateEnhancedPrompt(const std::string &userPrompt)
{
    if (!vectorDatabase) {
        return userPrompt;
    }
    
    // Extract search terms from user prompt
    auto searchTerms = extractSearchTerms(userPrompt);
    
    // Find similar patches for each search term
    std::vector<Surge::PatchDB::PatchVector> allSimilarPatches;
    for (const auto& term : searchTerms) {
        auto patches = vectorDatabase->findSimilarByText(term, 3);
        allSimilarPatches.insert(allSimilarPatches.end(), patches.begin(), patches.end());
    }
    
    // Remove duplicates (simple name-based)
    std::sort(allSimilarPatches.begin(), allSimilarPatches.end(), 
              [](const auto& a, const auto& b) { return a.name < b.name; });
    allSimilarPatches.erase(
        std::unique(allSimilarPatches.begin(), allSimilarPatches.end(),
                   [](const auto& a, const auto& b) { return a.name == b.name; }),
        allSimilarPatches.end());
    
    // Limit to top 5 patches to avoid prompt bloat
    if (allSimilarPatches.size() > 5) {
        allSimilarPatches.resize(5);
    }
    
    if (allSimilarPatches.empty()) {
        return userPrompt;
    }
    
    // Format enhanced prompt
    std::string enhancedPrompt = userPrompt + "\n\n";
    enhancedPrompt += "For reference, here are some similar patches from the factory library:\n";
    enhancedPrompt += formatSimilarPatches(allSimilarPatches);
    enhancedPrompt += "\nUse these as inspiration but create something new based on the user's request.";
    
    return enhancedPrompt;
}

std::vector<std::string> APIClient::extractSearchTerms(const std::string &prompt)
{
    std::vector<std::string> terms;
    
    // Common synthesis terms to look for
    std::vector<std::string> keywords = {
        "bass", "lead", "pad", "pluck", "arp", "chord", "string",
        "brass", "bell", "organ", "piano", "ep", "electric",
        "ambient", "atmospheric", "warm", "bright", "dark", "soft",
        "hard", "aggressive", "gentle", "smooth", "rough", "clean",
        "distorted", "filtered", "resonant", "fm", "wavetable",
        "analog", "digital", "vintage", "modern", "classic"
    };
    
    // Convert prompt to lowercase for matching
    std::string lowerPrompt = prompt;
    std::transform(lowerPrompt.begin(), lowerPrompt.end(), lowerPrompt.begin(), ::tolower);
    
    // Find keywords in the prompt
    for (const auto& keyword : keywords) {
        if (lowerPrompt.find(keyword) != std::string::npos) {
            terms.push_back(keyword);
        }
    }
    
    // If no specific terms found, use the whole prompt for general search
    if (terms.empty()) {
        terms.push_back(prompt);
    }
    
    return terms;
}

std::string APIClient::formatSimilarPatches(const std::vector<Surge::PatchDB::PatchVector>& patches)
{
    std::ostringstream oss;
    
    for (size_t i = 0; i < patches.size(); ++i) {
        const auto& patch = patches[i];
        oss << "- " << patch.name << " (" << patch.category << ")";
        if (!patch.description.empty()) {
            oss << ": " << patch.description;
        }
        oss << "\n";
    }
    
    return oss.str();
}