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

#ifndef SURGE_SRC_COMMON_PATCHVECTORDB_H
#define SURGE_SRC_COMMON_PATCHVECTORDB_H

#include <vector>
#include <string>
#include <memory>
#include <unordered_map>

class SurgeStorage;

#include "PatchParameterExtractor.h"

namespace Surge
{
namespace PatchDB
{

// Simple vector representation of a patch
struct PatchVector
{
    std::string name;
    std::string category;
    std::string author;
    std::string description;             // Semantic description
    std::vector<float> parameterVector;  // Normalized parameters [0,1]
    std::vector<float> textEmbedding;    // Text embedding from name/description
    std::string filePath;
    
    // Metadata
    std::unordered_map<std::string, std::string> tags;
    
    float cosineSimilarity(const PatchVector& other) const;
    float euclideanDistance(const PatchVector& other) const;
};

class VectorDatabase
{
public:
    VectorDatabase(SurgeStorage* storage);
    ~VectorDatabase();
    
    // Build database from factory patches
    void buildFromFactoryPatches();
    
    // Add a single patch
    void addPatch(const std::string& path);
    
    // Search functions
    std::vector<PatchVector> findSimilarPatches(const PatchVector& query, int topK = 5);
    std::vector<PatchVector> findSimilarByText(const std::string& description, int topK = 5);
    std::vector<PatchVector> findSimilarByParameters(const std::vector<float>& params, int topK = 5);
    
    // Hybrid search combining text and parameters
    std::vector<PatchVector> hybridSearch(const std::string& text, 
                                         const std::vector<float>& params,
                                         float textWeight = 0.5f,
                                         int topK = 5);
    
    // Save/Load database
    void saveToFile(const std::string& path);
    void loadFromFile(const std::string& path);
    
    // Get all patches in a category
    std::vector<PatchVector> getPatchesByCategory(const std::string& category);
    
    // Public access for testing
    std::vector<PatchVector> patches;
    
private:
    SurgeStorage* storage;
    PatchParameterExtractor extractor;
    
    // Helper functions
    PatchVector extractPatchVector(const std::string& patchPath);
    std::vector<float> normalizeParameters(const float* params, int count);
    std::vector<float> generateTextEmbedding(const std::string& text);
};

} // namespace PatchDB
} // namespace Surge

#endif // SURGE_SRC_COMMON_PATCHVECTORDB_H