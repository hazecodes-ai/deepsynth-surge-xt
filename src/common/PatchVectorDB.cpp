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

#include "PatchVectorDB.h"
#include "PatchParameterExtractor.h"
#include "SurgeStorage.h"
#include "filesystem/import.h"
#include <cmath>
#include <algorithm>
#include <iostream>
#include <fstream>

namespace Surge
{
namespace PatchDB
{

float PatchVector::cosineSimilarity(const PatchVector& other) const
{
    if (parameterVector.size() != other.parameterVector.size())
        return 0.0f;
    
    float dotProduct = 0.0f;
    float normA = 0.0f;
    float normB = 0.0f;
    
    for (size_t i = 0; i < parameterVector.size(); ++i)
    {
        dotProduct += parameterVector[i] * other.parameterVector[i];
        normA += parameterVector[i] * parameterVector[i];
        normB += other.parameterVector[i] * other.parameterVector[i];
    }
    
    if (normA == 0.0f || normB == 0.0f)
        return 0.0f;
    
    return dotProduct / (std::sqrt(normA) * std::sqrt(normB));
}

float PatchVector::euclideanDistance(const PatchVector& other) const
{
    if (parameterVector.size() != other.parameterVector.size())
        return std::numeric_limits<float>::max();
    
    float sum = 0.0f;
    for (size_t i = 0; i < parameterVector.size(); ++i)
    {
        float diff = parameterVector[i] - other.parameterVector[i];
        sum += diff * diff;
    }
    
    return std::sqrt(sum);
}

VectorDatabase::VectorDatabase(SurgeStorage* storage) : storage(storage), extractor(storage)
{
}

VectorDatabase::~VectorDatabase() = default;

void VectorDatabase::buildFromFactoryPatches()
{
    patches.clear();
    
    // Get factory patch directories  
    auto factoryPath = fs::path(storage->datapath) / "patches_factory";
    auto thirdPartyPath = fs::path(storage->datapath) / "patches_3rdparty";
    
    std::cout << "Building patch vector database..." << std::endl;
    std::cout << "Factory path: " << factoryPath << std::endl;
    
    // Use the PatchParameterExtractor to batch extract from directories
    if (fs::exists(factoryPath))
    {
        auto extractedPatches = extractor.extractFromDirectory(factoryPath.string());
        std::cout << "Extracted " << extractedPatches.size() << " patches from factory" << std::endl;
        
        for (const auto& patchData : extractedPatches)
        {
            PatchVector pv;
            pv.name = patchData.name;
            pv.category = patchData.category;
            pv.description = patchData.getSemanticDescription();
            pv.parameterVector = patchData.toNormalizedVector();
            
            patches.push_back(pv);
        }
    }
    
    // Also scan third party patches if available
    if (fs::exists(thirdPartyPath))
    {
        auto extractedPatches = extractor.extractFromDirectory(thirdPartyPath.string());
        std::cout << "Extracted " << extractedPatches.size() << " patches from 3rd party" << std::endl;
        
        for (const auto& patchData : extractedPatches)
        {
            PatchVector pv;
            pv.name = patchData.name;
            pv.category = patchData.category;
            pv.description = patchData.getSemanticDescription();
            pv.parameterVector = patchData.toNormalizedVector();
            
            patches.push_back(pv);
        }
    }
    
    std::cout << "Loaded " << patches.size() << " patches into vector database" << std::endl;
}

void VectorDatabase::addPatch(const std::string& path)
{
    ExtractedPatchData patchData;
    if (extractor.extractFromFile(path, patchData))
    {
        PatchVector pv;
        pv.filePath = path;
        pv.name = patchData.name;
        pv.category = patchData.category;
        pv.description = patchData.getSemanticDescription();
        pv.parameterVector = patchData.toNormalizedVector();
        
        patches.push_back(pv);
    }
    else
    {
        std::cout << "Failed to extract patch: " << path << " - " << extractor.getLastError() << std::endl;
    }
}

PatchVector VectorDatabase::extractPatchVector(const std::string& patchPath)
{
    PatchVector pv;
    pv.filePath = patchPath;
    
    ExtractedPatchData patchData;
    if (extractor.extractFromFile(patchPath, patchData))
    {
        pv.name = patchData.name;
        pv.category = patchData.category;
        pv.description = patchData.getSemanticDescription();
        pv.parameterVector = patchData.toNormalizedVector();
    }
    else
    {
        // Fallback to filename parsing
        pv.name = fs::path(patchPath).stem().string();
        pv.category = fs::path(patchPath).parent_path().filename().string();
        pv.description = "Failed to extract parameters";
        // Empty vector indicates failure
    }
    
    return pv;
}

std::vector<PatchVector> VectorDatabase::findSimilarPatches(const PatchVector& query, int topK)
{
    std::vector<std::pair<float, size_t>> similarities;
    
    for (size_t i = 0; i < patches.size(); ++i)
    {
        float sim = query.cosineSimilarity(patches[i]);
        similarities.push_back({sim, i});
    }
    
    // Sort by similarity (descending)
    std::sort(similarities.begin(), similarities.end(), 
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Return top K
    std::vector<PatchVector> results;
    for (int i = 0; i < std::min(topK, (int)similarities.size()); ++i)
    {
        results.push_back(patches[similarities[i].second]);
    }
    
    return results;
}

std::vector<PatchVector> VectorDatabase::findSimilarByText(const std::string& description, int topK)
{
    // Simple text matching for now
    std::vector<std::pair<float, size_t>> scores;
    
    // Convert description to lowercase
    std::string queryLower = description;
    std::transform(queryLower.begin(), queryLower.end(), queryLower.begin(), ::tolower);
    
    for (size_t i = 0; i < patches.size(); ++i)
    {
        std::string nameLower = patches[i].name;
        std::transform(nameLower.begin(), nameLower.end(), nameLower.begin(), ::tolower);
        
        // Simple substring matching score
        float score = 0.0f;
        if (nameLower.find(queryLower) != std::string::npos)
            score = 1.0f;
        else if (queryLower.find(nameLower) != std::string::npos)
            score = 0.8f;
        
        // Check individual words
        std::istringstream iss(queryLower);
        std::string word;
        while (iss >> word)
        {
            if (nameLower.find(word) != std::string::npos)
                score += 0.3f;
        }
        
        if (score > 0)
            scores.push_back({score, i});
    }
    
    // Sort by score
    std::sort(scores.begin(), scores.end(),
              [](const auto& a, const auto& b) { return a.first > b.first; });
    
    // Return top K
    std::vector<PatchVector> results;
    for (int i = 0; i < std::min(topK, (int)scores.size()); ++i)
    {
        results.push_back(patches[scores[i].second]);
    }
    
    return results;
}

} // namespace PatchDB
} // namespace Surge