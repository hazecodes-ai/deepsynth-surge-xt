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

#ifndef SURGE_SRC_SURGE_XT_GUI_PANELS_DEEPSYNTHPANEL_H
#define SURGE_SRC_SURGE_XT_GUI_PANELS_DEEPSYNTHPANEL_H

#include <juce_gui_basics/juce_gui_basics.h>
#include <memory>
#include <functional>

class SurgeGUIEditor;
class SurgeStorage;

namespace Surge
{
namespace Claude
{
class APIClient;
class ParameterMapper;
struct ClaudeResponse;
}
namespace PatchDB
{
class VectorDatabase;
}
}

namespace Surge
{
namespace GUI
{
namespace Panels
{

class DeepSynthPanel : public juce::Component,
                       public juce::Button::Listener,
                       public juce::TextEditor::Listener
{
  public:
    DeepSynthPanel(SurgeGUIEditor *editor, SurgeStorage *storage);
    ~DeepSynthPanel() override;

    void paint(juce::Graphics &g) override;
    void resized() override;
    
    // Mouse handling for dragging
    void mouseDown(const juce::MouseEvent &event) override;
    void mouseDrag(const juce::MouseEvent &event) override;
    
    void buttonClicked(juce::Button *button) override;
    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
    
    void setVisible(bool shouldBeVisible) override;
    void showApiSettings();
    
    // State management
    struct State {
        bool hasResponse = false;
        std::string lastResponseText = "";
        std::string lastPromptText = "";
    };
    
    State getState() const;
    void setState(const State& state);
    
  private:
    void setupComponents();
    void processPrompt(bool isModification);
    void handleClaudeResponse(const Surge::Claude::ClaudeResponse &response);
    void updateStatus(const std::string &status, bool isError = false);
    
    SurgeGUIEditor *editor;
    SurgeStorage *storage;
    
    std::unique_ptr<Surge::Claude::APIClient> claudeClient;
    std::shared_ptr<Surge::PatchDB::VectorDatabase> vectorDatabase;
    
    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::TextEditor> promptEditor;
    std::unique_ptr<juce::TextButton> generateButton;
    std::unique_ptr<juce::TextButton> modifyButton;
    std::unique_ptr<juce::Label> statusLabel;
    std::unique_ptr<juce::TextEditor> responseDisplay;
    
    bool isProcessing = false;
    bool hasResponse = false;
    std::string lastResponseText = "";
    
    // For dragging
    juce::Point<int> dragStartPosition;
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(DeepSynthPanel)
};

} // namespace Panels
} // namespace GUI
} // namespace Surge

#endif // SURGE_SRC_SURGE_XT_GUI_PANELS_DEEPSYNTHPANEL_H