# DeepSynth for Surge XT

DeepSynth is an innovative extension for Surge XT that enables synthesizer patch generation and modification using natural language through Claude AI.

## Key Features

### 🤖 AI-Powered Patch Generation
- Generate patches automatically by describing desired sounds in natural language
- Use intuitive expressions like "warm ambient pad" or "aggressive lead sound"

### 🔍 RAG (Retrieval-Augmented Generation) System
- Vector database built from 3000+ factory patches
- References similar existing patches for more accurate AI responses
- Supports both cosine similarity-based vector search and text-based search

### 🎨 Intuitive UI
- Draggable floating panel
- Auto-positioned in bottom-right corner
- Overlay design that doesn't interfere with existing Surge XT UI

## Installation

### Requirements
- macOS, Windows, or Linux
- C++17 compatible compiler
- CMake 3.15 or higher
- Claude API key (obtain from https://console.anthropic.com/)

### Build Instructions

```bash
# Clone repository
git clone https://github.com/hazecodes-ai/deepsynth-surge-xt.git
cd deepsynth-surge-xt

# Update submodules
git submodule update --init --recursive

# Create build directory
mkdir build
cd build

# Configure and build
cmake ..
make -j8

# Run on macOS
open src/surge-xt/surge-xt_artefacts/Release/Standalone/Surge\ XT.app
```

## Usage

1. **Configure API Key**
   - Select Menu → DeepSynth → API Config
   - Enter your Claude API key (sk-ant-... format)

2. **Open Panel**
   - Select Menu → DeepSynth → Show Panel
   - Panel appears in bottom-right corner

3. **Generate Patches**
   - Enter desired sound description in text field
   - Click "Generate" button
   - AI-generated patch is automatically applied

4. **Modify Patches**
   - Enter modifications based on current patch
   - Click "Modify" button
   - AI analyzes existing patch and applies requested changes

## Technical Implementation

### Architecture
```
DeepSynthPanel (UI)
    ↓
ClaudeAPIClient (API Communication)
    ↓
PatchVectorDB (RAG System)
    ↓
PatchParameterExtractor (FXP File Parsing)
    ↓
ClaudeParameterMapper (Parameter Mapping)
```

### Core Components

- **PatchVectorDB**: 50-dimensional vector representation and similarity search for factory patches
- **PatchParameterExtractor**: Extracts patch parameters from FXP files
- **ClaudeAPIClient**: Handles Claude API communication and RAG prompt generation
- **ClaudeParameterMapper**: Converts AI responses to Surge parameters

### Security Considerations
- API keys are stored locally only and never included in source code
- Size limit for corrupted patch files (4MB)
- Stable error recovery through exception handling

## Contributing

1. Fork this repository
2. Create a feature branch (`git checkout -b feature/AmazingFeature`)
3. Commit your changes (`git commit -m 'Add some AmazingFeature'`)
4. Push to the branch (`git push origin feature/AmazingFeature`)
5. Create a Pull Request

## License

This project is licensed under the same GPL-3.0 license as Surge XT.

## Acknowledgments

- [Surge Synth Team](https://surge-synthesizer.github.io/) - For the excellent open-source synthesizer
- [Anthropic](https://www.anthropic.com/) - For providing Claude AI API
- All contributors and testers

## Important Notes

⚠️ **API Key Security**
- Never share your Claude API key or commit it to public repositories
- API keys are stored only in your local settings

⚠️ **Experimental Feature**
- DeepSynth is an experimental feature and may exhibit unexpected behavior
- Save generated patches before using them in important projects

## Support

- Issue Reports: [GitHub Issues](https://github.com/hazecodes-ai/deepsynth-surge-xt/issues)
- Discussions: [GitHub Discussions](https://github.com/hazecodes-ai/deepsynth-surge-xt/discussions)

---

Made with ❤️ by HazeCodes AI