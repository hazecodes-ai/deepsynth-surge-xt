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

#include "ClaudePromptDialog.h"
#include "SurgeGUIEditor.h"
#include "SurgeStorage.h"
#include "UserDefaults.h"
#include "SurgeSynthesizer.h"
#include "RuntimeFont.h"
#include "SurgeImageStore.h"
#include "ClaudeParameterMapper.h"
#include "widgets/MainFrame.h"

namespace Surge
{
namespace Overlays
{

ClaudePromptDialog::ClaudePromptDialog(SurgeGUIEditor *editor, SurgeStorage *storage)
    : OverlayComponent(), editor(editor), storage(storage)
{
    claudeClient = std::make_unique<Surge::Claude::APIClient>(storage);
    setupComponents();
}

ClaudePromptDialog::~ClaudePromptDialog() = default;

void ClaudePromptDialog::setupComponents()
{
    // Title
    titleLabel = std::make_unique<juce::Label>("title", "DeepSynth");
    titleLabel->setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*titleLabel);

    // Prompt label
    promptLabel = std::make_unique<juce::Label>("promptLabel", "Describe the sound you want:");
    promptLabel->setFont(juce::Font(14.0f));
    addAndMakeVisible(*promptLabel);

    // Prompt editor
    promptEditor = std::make_unique<juce::TextEditor>("promptEditor");
    promptEditor->setMultiLine(true);
    promptEditor->setReturnKeyStartsNewLine(true);
    promptEditor->setPopupMenuEnabled(true);
    promptEditor->setScrollbarsShown(true);
    promptEditor->addListener(this);
    addAndMakeVisible(*promptEditor);

    // Buttons
    generateButton = std::make_unique<juce::TextButton>("Generate New Patch");
    generateButton->addListener(this);
    addAndMakeVisible(*generateButton);

    modifyButton = std::make_unique<juce::TextButton>("Modify Current Patch");
    modifyButton->addListener(this);
    addAndMakeVisible(*modifyButton);

    settingsButton = std::make_unique<juce::TextButton>("API Settings");
    settingsButton->addListener(this);
    addAndMakeVisible(*settingsButton);

    cancelButton = std::make_unique<juce::TextButton>("Close");
    cancelButton->addListener(this);
    addAndMakeVisible(*cancelButton);

    // Response editor
    responseEditor = std::make_unique<juce::TextEditor>("responseEditor");
    responseEditor->setMultiLine(true);
    responseEditor->setReadOnly(true);
    responseEditor->setPopupMenuEnabled(true);
    responseEditor->setScrollbarsShown(true);
    addAndMakeVisible(*responseEditor);

    // Status label
    statusLabel = std::make_unique<juce::Label>("status", "Ready");
    statusLabel->setFont(juce::Font(12.0f));
    addAndMakeVisible(*statusLabel);
}

void ClaudePromptDialog::paint(juce::Graphics &g)
{
    auto skinCtrl = editor->currentSkin;
    auto c = skinCtrl->getColor(Colors::Dialog::Background);
    g.fillAll(c);

    auto borderC = skinCtrl->getColor(Colors::Dialog::Border);
    g.setColour(borderC);
    g.drawRect(getLocalBounds(), 1);
}

void ClaudePromptDialog::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Title
    titleLabel->setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);
    
    // Prompt section
    promptLabel->setBounds(bounds.removeFromTop(20));
    bounds.removeFromTop(5);
    promptEditor->setBounds(bounds.removeFromTop(100));
    bounds.removeFromTop(10);
    
    // Buttons row
    auto buttonRow = bounds.removeFromTop(30);
    auto buttonWidth = (buttonRow.getWidth() - 30) / 4;
    generateButton->setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(10);
    modifyButton->setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(10);
    settingsButton->setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(10);
    cancelButton->setBounds(buttonRow);
    
    bounds.removeFromTop(10);
    
    // Response section
    responseEditor->setBounds(bounds.removeFromBottom(150));
    bounds.removeFromBottom(10);
    
    // Status
    statusLabel->setBounds(bounds.removeFromBottom(20));
}

void ClaudePromptDialog::visibilityChanged()
{
    if (isVisible())
    {
        promptEditor->grabKeyboardFocus();
        updateStatus("Ready");
    }
}

void ClaudePromptDialog::setVisible(bool shouldBeVisible)
{
    OverlayComponent::setVisible(shouldBeVisible);
    if (shouldBeVisible)
    {
        if (!claudeClient->isAPIKeyValid())
        {
            updateStatus("Warning: Claude API key not configured", true);
        }
    }
}

void ClaudePromptDialog::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if (&editor == promptEditor.get() && promptEditor->isMultiLine())
        return; // Allow new lines in multiline editor
}

void ClaudePromptDialog::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{
    setVisible(false);
}

void ClaudePromptDialog::buttonClicked(juce::Button *button)
{
    if (button == generateButton.get())
    {
        processPrompt(false);
    }
    else if (button == modifyButton.get())
    {
        processPrompt(true);
    }
    else if (button == settingsButton.get())
    {
        showSettings();
    }
    else if (button == cancelButton.get())
    {
        setVisible(false);
    }
}

void ClaudePromptDialog::processPrompt(bool isModification)
{
    if (isProcessing)
        return;

    auto prompt = promptEditor->getText().toStdString();
    if (prompt.empty())
    {
        updateStatus("Please enter a prompt", true);
        return;
    }

    if (!claudeClient->isAPIKeyValid())
    {
        updateStatus("Please configure Claude API key in settings", true);
        showSettings();
        return;
    }

    isProcessing = true;
    generateButton->setEnabled(false);
    modifyButton->setEnabled(false);
    updateStatus("Generating response...");
    responseEditor->setText("Processing request...");

    auto callback = [this](const Surge::Claude::ClaudeResponse &response)
    {
        juce::MessageManager::callAsync([this, response]()
        {
            handleClaudeResponse(response);
            isProcessing = false;
            generateButton->setEnabled(true);
            modifyButton->setEnabled(true);
        });
    };

    if (isModification)
    {
        // Get current patch information
        Surge::Claude::ParameterMapper mapper(editor->synth);
        std::string patchInfo = mapper.exportCurrentPatchInfo();
        
        claudeClient->modifyPatch(prompt, patchInfo, callback);
    }
    else
    {
        claudeClient->generatePatch(prompt, callback);
    }
}

void ClaudePromptDialog::showSettings()
{
    auto settingsDialog = std::make_unique<ClaudeSettingsDialog>(editor, storage);
    settingsDialog->setBounds(0, 0, 600, 500);  // Much larger to show all elements
    settingsDialog->setCentrePosition(getBounds().getCentre());
    addAndMakeVisible(*settingsDialog);
    settingsDialog.release(); // Transfer ownership to parent
}

void ClaudePromptDialog::handleClaudeResponse(const Surge::Claude::ClaudeResponse &response)
{
    if (response.success)
    {
        responseEditor->setText(response.responseText);
        applyPatchModifications(response.modifications);
        
        if (response.modifications.empty())
        {
            updateStatus("Response received, but no parameters to modify");
        }
        else
        {
            updateStatus("Patch modified with " + std::to_string(response.modifications.size()) + " parameter changes");
        }
    }
    else
    {
        responseEditor->setText("Error: " + response.errorMessage);
        updateStatus("Error: " + response.errorMessage, true);
    }
}

void ClaudePromptDialog::applyPatchModifications(const std::vector<Surge::Claude::PatchModification> &modifications)
{
    std::cout << "DEBUG: Applying " << modifications.size() << " patch modifications" << std::endl;
    
    if (modifications.empty())
    {
        std::cout << "DEBUG: No modifications to apply!" << std::endl;
        
        // Test with direct parameter setting to verify the system works
        std::cout << "DEBUG: Testing with direct parameter setting" << std::endl;
        
        // Try to set filter cutoff directly using known parameter IDs
        // Scene A Filter 1 Cutoff is typically at a specific index
        for (int i = 0; i < 200; i++)  // Check first 200 parameters
        {
            auto param = editor->synth->storage.getPatch().param_ptr[i];
            if (param)
            {
                std::string fullName = param->get_full_name();
                if (fullName.find("Filter 1 Cutoff") != std::string::npos && fullName.find("A") != std::string::npos)
                {
                    std::cout << "DEBUG: Found Filter 1 Cutoff at index " << i << ": " << fullName << std::endl;
                    
                    // Set the parameter directly
                    float oldVal = param->get_value_f01();
                    editor->synth->setParameter01(editor->synth->idForParameter(param), 0.8f, false);
                    float newVal = param->get_value_f01();
                    
                    std::cout << "DEBUG: Changed parameter from " << oldVal << " to " << newVal << std::endl;
                    break;
                }
            }
        }
    }
    else
    {
        Surge::Claude::ParameterMapper mapper(editor->synth);
        bool success = mapper.applyModifications(modifications);
    }
    
    // Always refresh UI after any parameter changes
    std::cout << "DEBUG: Refreshing synth after parameter changes" << std::endl;
    
    // Mark patch as dirty
    editor->synth->storage.getPatch().isDirty = true;
    
    // Force complete UI refresh
    editor->synth->refresh_editor = true;
    editor->refresh_mod();
    
    // Repaint the editor
    if (editor->frame)
    {
        editor->frame->repaint();
    }
    
    std::cout << "DEBUG: Completed UI refresh after parameter changes" << std::endl;
}

void ClaudePromptDialog::updateStatus(const std::string &status, bool isError)
{
    statusLabel->setText(status, juce::dontSendNotification);
    
    if (isError)
    {
        statusLabel->setColour(juce::Label::textColourId, juce::Colours::red);
    }
    else
    {
        auto skinCtrl = editor->currentSkin;
        auto textColor = skinCtrl->getColor(Colors::Dialog::Label::Text);
        statusLabel->setColour(juce::Label::textColourId, textColor);
    }
}

//=============================================================================
// ClaudeSettingsDialog
//=============================================================================

ClaudeSettingsDialog::ClaudeSettingsDialog(SurgeGUIEditor *editor, SurgeStorage *storage)
    : OverlayComponent(), editor(editor), storage(storage)
{
    setupComponents();
}

ClaudeSettingsDialog::~ClaudeSettingsDialog() = default;

void ClaudeSettingsDialog::setupComponents()
{
    // Title
    titleLabel = std::make_unique<juce::Label>("title", "DEBUG: ENTER BUTTON TEST - DeepSynth API Settings");
    titleLabel->setFont(juce::Font(18.0f, juce::Font::bold));
    titleLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*titleLabel);

    // API Key label
    apiKeyLabel = std::make_unique<juce::Label>("apiKeyLabel", "Claude API Key:");
    apiKeyLabel->setFont(juce::Font(14.0f));
    addAndMakeVisible(*apiKeyLabel);

    // API Key editor
    apiKeyEditor = std::make_unique<juce::TextEditor>("apiKeyEditor");
    apiKeyEditor->setPasswordCharacter('*');
    apiKeyEditor->setText(Surge::Storage::getUserDefaultValue(storage, 
                                                             Surge::Storage::DefaultKey::ClaudeAPIKey, ""));
    apiKeyEditor->setMultiLine(false);  // Explicitly set single-line mode
    apiKeyEditor->setReturnKeyStartsNewLine(false);  // Disable new line on return
    apiKeyEditor->addListener(this);
    addAndMakeVisible(*apiKeyEditor);
    
    // Enter button (next to text field)
    enterButton = std::make_unique<juce::TextButton>("DEBUG ENTER BUTTON");
    enterButton->addListener(this);
    enterButton->setColour(juce::TextButton::buttonColourId, juce::Colours::red);  // Make it obvious
    addAndMakeVisible(*enterButton);

    // Info label
    infoLabel = std::make_unique<juce::Label>("infoLabel", 
        "Get your API key from: https://console.anthropic.com/\nAPI key format: sk-ant-...\nPress Enter to save or Escape to cancel");
    infoLabel->setFont(juce::Font(12.0f));
    infoLabel->setJustificationType(juce::Justification::centredLeft);
    addAndMakeVisible(*infoLabel);

    // Buttons
    saveButton = std::make_unique<juce::TextButton>("Save");
    saveButton->addListener(this);
    addAndMakeVisible(*saveButton);

    cancelButton = std::make_unique<juce::TextButton>("Cancel");
    cancelButton->addListener(this);
    addAndMakeVisible(*cancelButton);
}

void ClaudeSettingsDialog::paint(juce::Graphics &g)
{
    auto skinCtrl = editor->currentSkin;
    auto c = skinCtrl->getColor(Colors::Dialog::Background);
    g.fillAll(c);

    auto borderC = skinCtrl->getColor(Colors::Dialog::Border);
    g.setColour(borderC);
    g.drawRect(getLocalBounds(), 1);
}

void ClaudeSettingsDialog::resized()
{
    auto bounds = getLocalBounds().reduced(20);
    
    // Title
    titleLabel->setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(10);
    
    // API Key label
    apiKeyLabel->setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);
    
    // API key editor and Enter button on same row
    auto apiKeyRow = bounds.removeFromTop(30);
    enterButton->setBounds(apiKeyRow.removeFromRight(80));  // Enter button on the right
    apiKeyRow.removeFromRight(10);  // Small gap
    apiKeyEditor->setBounds(apiKeyRow);  // Text editor takes remaining space
    
    bounds.removeFromTop(20);
    
    // Info
    infoLabel->setBounds(bounds.removeFromTop(80));  // Increased height for 3 lines
    bounds.removeFromTop(30);
    
    // Buttons (Save/Cancel)
    auto buttonRow = bounds.removeFromTop(35);
    auto buttonWidth = (buttonRow.getWidth() - 20) / 2;
    saveButton->setBounds(buttonRow.removeFromLeft(buttonWidth));
    buttonRow.removeFromLeft(20);
    cancelButton->setBounds(buttonRow);
}

void ClaudeSettingsDialog::visibilityChanged()
{
    if (isVisible())
    {
        apiKeyEditor->grabKeyboardFocus();
    }
}

void ClaudeSettingsDialog::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    std::cout << "DEBUG: Return key pressed in ClaudeSettingsDialog" << std::endl;
    if (&editor == apiKeyEditor.get())
    {
        std::cout << "DEBUG: Calling saveSettings()" << std::endl;
        saveSettings();
    }
}

void ClaudeSettingsDialog::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{
    setVisible(false);
}

void ClaudeSettingsDialog::buttonClicked(juce::Button *button)
{
    if (button == saveButton.get() || button == enterButton.get())
    {
        saveSettings();
    }
    else if (button == cancelButton.get())
    {
        setVisible(false);
    }
}

void ClaudeSettingsDialog::saveSettings()
{
    std::cout << "DEBUG: saveSettings() called" << std::endl;
    auto apiKey = apiKeyEditor->getText().toStdString();
    std::cout << "DEBUG: API key length: " << apiKey.length() << std::endl;
    Surge::Storage::updateUserDefaultValue(storage, Surge::Storage::DefaultKey::ClaudeAPIKey, apiKey);
    std::cout << "DEBUG: API key saved, closing dialog" << std::endl;
    setVisible(false);
}

} // namespace Overlays
} // namespace Surge