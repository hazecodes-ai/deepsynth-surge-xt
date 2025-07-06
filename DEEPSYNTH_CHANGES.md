# DeepSynth Changes to Surge XT

This repository contains only the DeepSynth-specific changes to Surge XT.

## Changed Files

### New Files Added:
- `src/common/ClaudeAPIClient.h/cpp` - Claude API integration
- `src/common/ClaudeParameterMapper.h/cpp` - Parameter mapping for AI responses  
- `src/common/PatchParameterExtractor.h/cpp` - FXP file parameter extraction
- `src/common/PatchVectorDB.h/cpp` - Vector database for RAG system
- `src/surge-xt/gui/panels/DeepSynthPanel.h/cpp` - Main UI panel
- `src/surge-testrunner/UnitTestsClaude.cpp` - Comprehensive unit tests
- `docs/deepsynth_roadmap.md` - Development roadmap
- `docs/rag_architecture.md` - RAG system architecture
- `docs/rag_implementation_status.md` - Implementation status

### Modified Files:
- `src/common/CMakeLists.txt` - Added new source files
- `src/surge-xt/gui/SurgeGUIEditor.cpp` - Panel integration
- `src/surge-xt/gui/SurgeGUIEditorMenuStructures.cpp` - Menu items

## How to Apply These Changes

1. Clone the original Surge XT repository:
   ```bash
   git clone https://github.com/surge-synthesizer/surge.git
   ```

2. Copy the DeepSynth changes:
   ```bash
   cp -r deepsynth-surge-xt/src/* surge/src/
   cp -r deepsynth-surge-xt/docs/* surge/docs/
   ```

3. Build Surge XT with DeepSynth:
   ```bash
   cd surge
   mkdir build && cd build
   cmake ..
   make -j8
   ```

## Original Repository

This is based on Surge XT: https://github.com/surge-synthesizer/surge

## License

GPL-3.0 (same as Surge XT)