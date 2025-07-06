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

#ifndef SURGE_SRC_SURGE_XT_GUI_OVERLAYS_CLAUDEPROMPTDIALOG_H
#define SURGE_SRC_SURGE_XT_GUI_OVERLAYS_CLAUDEPROMPTDIALOG_H

#include "OverlayComponent.h"
#include "SkinSupport.h"
#include "ClaudeAPIClient.h"
#include "juce_gui_basics/juce_gui_basics.h"
#include <memory>

class SurgeGUIEditor;
class SurgeStorage;

namespace Surge
{
namespace Overlays
{

class ClaudePromptDialog : public OverlayComponent,
                          public juce::TextEditor::Listener,
                          public juce::Button::Listener
{
  public:
    ClaudePromptDialog(SurgeGUIEditor *editor, SurgeStorage *storage);
    ~ClaudePromptDialog();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void visibilityChanged() override;
    
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
    
    // Button::Listener
    void buttonClicked(juce::Button *button) override;

    void setVisible(bool shouldBeVisible) override;
    
    // Override to prevent auto-restore on startup
    bool getRetainOpenStateOnEditorRecreate() override { return false; }

  private:
    SurgeGUIEditor *editor;
    SurgeStorage *storage;
    
    std::unique_ptr<Surge::Claude::APIClient> claudeClient;
    
    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Label> promptLabel;
    std::unique_ptr<juce::TextEditor> promptEditor;
    std::unique_ptr<juce::TextButton> generateButton;
    std::unique_ptr<juce::TextButton> modifyButton;
    std::unique_ptr<juce::TextButton> settingsButton;
    std::unique_ptr<juce::TextButton> cancelButton;
    std::unique_ptr<juce::TextEditor> responseEditor;
    std::unique_ptr<juce::Label> statusLabel;
    
    bool isProcessing = false;
    
    void setupComponents();
    void processPrompt(bool isModification);
    void showSettings();
    void handleClaudeResponse(const Surge::Claude::ClaudeResponse &response);
    void applyPatchModifications(const std::vector<Surge::Claude::PatchModification> &modifications);
    void updateStatus(const std::string &status, bool isError = false);
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClaudePromptDialog)
};

class ClaudeSettingsDialog : public OverlayComponent,
                            public juce::TextEditor::Listener,
                            public juce::Button::Listener
{
  public:
    ClaudeSettingsDialog(SurgeGUIEditor *editor, SurgeStorage *storage);
    ~ClaudeSettingsDialog();

    void paint(juce::Graphics &g) override;
    void resized() override;

    void visibilityChanged() override;
    
    // TextEditor::Listener
    void textEditorReturnKeyPressed(juce::TextEditor &editor) override;
    void textEditorEscapeKeyPressed(juce::TextEditor &editor) override;
    
    // Button::Listener
    void buttonClicked(juce::Button *button) override;

  private:
    SurgeGUIEditor *editor;
    SurgeStorage *storage;
    
    // UI Components
    std::unique_ptr<juce::Label> titleLabel;
    std::unique_ptr<juce::Label> apiKeyLabel;
    std::unique_ptr<juce::TextEditor> apiKeyEditor;
    std::unique_ptr<juce::TextButton> enterButton;  // Enter button next to text field
    std::unique_ptr<juce::TextButton> saveButton;
    std::unique_ptr<juce::TextButton> cancelButton;
    std::unique_ptr<juce::Label> infoLabel;
    
    void setupComponents();
    void saveSettings();
    
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClaudeSettingsDialog)
};

} // namespace Overlays
} // namespace Surge

#endif // SURGE_SRC_SURGE_XT_GUI_OVERLAYS_CLAUDEPROMPTDIALOG_H