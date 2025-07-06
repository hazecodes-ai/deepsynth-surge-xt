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

#include "DeepSynthPanel.h"
#include "SurgeGUIEditor.h"
#include "SurgeStorage.h"
#include "UserDefaults.h"
#include "SurgeSynthesizer.h"
#include "RuntimeFont.h"
#include "SurgeImageStore.h"
#include "ClaudeParameterMapper.h"
#include "ClaudeAPIClient.h"
#include "PatchVectorDB.h"
#include "widgets/MainFrame.h"
#include "SkinColors.h"

using namespace Surge::Skin;

namespace Surge
{
namespace GUI
{
namespace Panels
{

DeepSynthPanel::DeepSynthPanel(SurgeGUIEditor *editor, SurgeStorage *storage)
    : editor(editor), storage(storage)
{
    std::cout << "DEBUG: DeepSynthPanel constructor called" << std::endl;
    claudeClient = std::make_unique<Surge::Claude::APIClient>(storage);
    
    // Initialize vector database for RAG functionality
    vectorDatabase = std::make_shared<Surge::PatchDB::VectorDatabase>(storage);
    std::cout << "DEBUG: Building vector database from factory patches..." << std::endl;
    
    // Build database with error handling for corrupted patches
    try {
        vectorDatabase->buildFromFactoryPatches();
        std::cout << "DEBUG: Vector database built with " << vectorDatabase->patches.size() << " patches" << std::endl;
    } catch (const std::exception& e) {
        std::cout << "WARNING: Vector database build failed: " << e.what() << std::endl;
        std::cout << "DEBUG: Continuing without vector database..." << std::endl;
    }
    
    // Connect vector database to Claude client for RAG
    claudeClient->setVectorDatabase(vectorDatabase);
    
    setupComponents();
}

DeepSynthPanel::~DeepSynthPanel() 
{
    std::cout << "DEBUG: DeepSynthPanel destructor called" << std::endl;
}

void DeepSynthPanel::setupComponents()
{
    std::cout << "DEBUG: setupComponents() called" << std::endl;
    
    // Title
    titleLabel = std::make_unique<juce::Label>("title", "DeepSynth");
    titleLabel->setFont(juce::Font(14.0f, juce::Font::bold));
    titleLabel->setJustificationType(juce::Justification::centred);
    addAndMakeVisible(*titleLabel);

    // Prompt editor
    promptEditor = std::make_unique<juce::TextEditor>("promptEditor");
    promptEditor->setMultiLine(true);
    promptEditor->setReturnKeyStartsNewLine(true);
    promptEditor->setPopupMenuEnabled(true);
    promptEditor->setScrollbarsShown(true);
    promptEditor->addListener(this);
    promptEditor->setTextToShowWhenEmpty("Describe sound...", juce::Colours::grey);
    addAndMakeVisible(*promptEditor);

    // Buttons - smaller and arranged horizontally
    generateButton = std::make_unique<juce::TextButton>("Generate");
    generateButton->addListener(this);
    generateButton->setTooltip("Generate a new patch from your description");
    addAndMakeVisible(*generateButton);

    modifyButton = std::make_unique<juce::TextButton>("Modify");
    modifyButton->addListener(this);
    modifyButton->setTooltip("Modify the current patch based on your description");
    addAndMakeVisible(*modifyButton);

    // Remove API button from main panel

    // Response display - smaller
    responseDisplay = std::make_unique<juce::TextEditor>("responseDisplay");
    responseDisplay->setMultiLine(true);
    responseDisplay->setReadOnly(true);
    responseDisplay->setPopupMenuEnabled(true);
    responseDisplay->setScrollbarsShown(true);
    responseDisplay->setFont(juce::Font(11.0f));  // Set initial font
    if (hasResponse && !lastResponseText.empty()) {
        responseDisplay->setText(lastResponseText);  // Restore last response
        responseDisplay->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    } else {
        responseDisplay->setText("Response will appear here...");  // Default text only if no response
        responseDisplay->setColour(juce::TextEditor::textColourId, juce::Colours::lightgrey);
    }
    // Use skin colors for background
    auto skinCtrl = editor->currentSkin;
    responseDisplay->setColour(juce::TextEditor::backgroundColourId, 
                              skinCtrl->getColor(Colors::Dialog::Entry::Background));
    responseDisplay->setColour(juce::TextEditor::outlineColourId,
                              skinCtrl->getColor(Colors::Dialog::Entry::Border));
    addAndMakeVisible(*responseDisplay);

    // Status label
    statusLabel = std::make_unique<juce::Label>("status", "Ready");
    statusLabel->setFont(juce::Font(10.0f));
    addAndMakeVisible(*statusLabel);
}

void DeepSynthPanel::paint(juce::Graphics &g)
{
    auto skinCtrl = editor->currentSkin;
    auto bgColor = skinCtrl->getColor(Colors::Dialog::Background);
    
    // Add slight transparency and darker background for sidebar
    g.fillAll(bgColor.darker(0.1f));
    
    // Draw left border to separate from main UI
    auto borderColor = skinCtrl->getColor(Colors::Dialog::Border);
    g.setColour(borderColor);
    g.drawLine(0, 0, 0, getHeight(), 2);
    
    // Subtle rounded corners
    g.setColour(borderColor.withAlpha(0.3f));
    g.drawRoundedRectangle(getLocalBounds().toFloat(), 2.0f, 1.0f);
}

void DeepSynthPanel::resized()
{
    auto bounds = getLocalBounds().reduced(10);
    
    // Title - bigger for sidebar
    titleLabel->setFont(juce::Font(16.0f, juce::Font::bold));
    titleLabel->setBounds(bounds.removeFromTop(25));
    bounds.removeFromTop(10);
    
    // Prompt editor - taller for sidebar
    promptEditor->setBounds(bounds.removeFromTop(100));
    bounds.removeFromTop(10);
    
    // Buttons row - stacked vertically for narrow sidebar
    generateButton->setBounds(bounds.removeFromTop(30));
    bounds.removeFromTop(5);
    modifyButton->setBounds(bounds.removeFromTop(30));
    
    bounds.removeFromTop(10);
    
    // Status
    statusLabel->setBounds(bounds.removeFromBottom(20));
    bounds.removeFromBottom(5);
    
    // Response display - takes remaining space (much taller now)
    responseDisplay->setBounds(bounds);
}

void DeepSynthPanel::mouseDown(const juce::MouseEvent &event)
{
    dragStartPosition = event.getPosition();
}

void DeepSynthPanel::mouseDrag(const juce::MouseEvent &event)
{
    auto newPosition = getPosition() + (event.getPosition() - dragStartPosition);
    
    // Constrain to parent bounds
    if (auto parent = getParentComponent())
    {
        auto parentBounds = parent->getLocalBounds();
        auto componentBounds = juce::Rectangle<int>(newPosition.x, newPosition.y, getWidth(), getHeight());
        
        // Keep component within parent bounds
        newPosition.x = juce::jmax(0, juce::jmin(newPosition.x, parentBounds.getWidth() - getWidth()));
        newPosition.y = juce::jmax(0, juce::jmin(newPosition.y, parentBounds.getHeight() - getHeight()));
    }
    
    setTopLeftPosition(newPosition);
}

void DeepSynthPanel::buttonClicked(juce::Button *button)
{
    if (button == generateButton.get())
    {
        processPrompt(false);
    }
    else if (button == modifyButton.get())
    {
        processPrompt(true);
    }
    // API button removed
}

void DeepSynthPanel::textEditorReturnKeyPressed(juce::TextEditor &editor)
{
    if (&editor == promptEditor.get() && !promptEditor->isMultiLine())
    {
        processPrompt(false);
    }
}

void DeepSynthPanel::textEditorEscapeKeyPressed(juce::TextEditor &editor)
{
    // Clear focus
    editor.unfocusAllComponents();
}

void DeepSynthPanel::setVisible(bool shouldBeVisible)
{
    Component::setVisible(shouldBeVisible);
    if (shouldBeVisible)
    {
        // Position panel in bottom-right corner when shown
        if (auto parent = getParentComponent())
        {
            int panelWidth = 300;
            int panelHeight = 400;
            int panelX = parent->getWidth() - panelWidth - 10;
            int panelY = parent->getHeight() - panelHeight - 10;
            setBounds(panelX, panelY, panelWidth, panelHeight);
        }
        
        promptEditor->grabKeyboardFocus();
        if (!claudeClient->isAPIKeyValid())
        {
            updateStatus("API key not configured", true);
        }
    }
}

void DeepSynthPanel::processPrompt(bool isModification)
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
        updateStatus("Please configure API key", true);
        return;
    }

    isProcessing = true;
    generateButton->setEnabled(false);
    modifyButton->setEnabled(false);
    updateStatus("Processing...");
    responseDisplay->setText("Generating response...");
    responseDisplay->setFont(juce::Font(11.0f));
    responseDisplay->setColour(juce::TextEditor::textColourId, juce::Colours::yellow);

    auto callback = [this](const Surge::Claude::ClaudeResponse &response)
    {
        std::cout << "DEBUG: Callback received response, success=" << response.success << std::endl;
        juce::MessageManager::callAsync([this, response]()
        {
            std::cout << "DEBUG: In MessageManager::callAsync" << std::endl;
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

void DeepSynthPanel::handleClaudeResponse(const Surge::Claude::ClaudeResponse &response)
{
    std::cout << "DEBUG: handleClaudeResponse called, success=" << response.success << std::endl;
    
    hasResponse = true;  // Mark that we have a response
    
    if (response.success)
    {
        // Extract natural language description from response
        std::string displayText;
        
        // Find natural language description after parameter list
        std::string fullResponse = response.responseText;
        size_t descStart = fullResponse.find("\n\n");
        if (descStart != std::string::npos)
        {
            // Get text after parameters section
            std::string description = fullResponse.substr(descStart + 2);
            
            // Clean up description - remove extra whitespace
            description.erase(0, description.find_first_not_of(" \n\r\t"));
            description.erase(description.find_last_not_of(" \n\r\t") + 1);
            
            displayText = description;
        }
        else if (response.modifications.empty())
        {
            displayText = "No parameters were modified.";
        }
        else
        {
            // Fallback: show what was changed
            displayText = "Applied " + std::to_string(response.modifications.size()) + " parameter changes to create your sound.";
        }
        
        std::cout << "DEBUG: Setting response text: " << displayText << std::endl;
        
        lastResponseText = displayText;  // Save the response
        responseDisplay->setText(displayText);
        responseDisplay->setFont(juce::Font(11.0f));
        responseDisplay->setColour(juce::TextEditor::textColourId, juce::Colours::white);
        
        // Apply modifications
        if (!response.modifications.empty())
        {
            Surge::Claude::ParameterMapper mapper(editor->synth);
            mapper.applyModifications(response.modifications);
            
            // Refresh UI
            editor->synth->refresh_editor = true;
            editor->refresh_mod();
            if (editor->frame)
            {
                editor->frame->repaint();
            }
            
            updateStatus("Applied " + std::to_string(response.modifications.size()) + " changes");
        }
        else
        {
            updateStatus("No parameters to modify");
        }
    }
    else
    {
        hasResponse = true;  // Mark that we have a response even for errors
        lastResponseText = "Error: " + response.errorMessage;  // Save error message
        responseDisplay->setText(lastResponseText);
        responseDisplay->setFont(juce::Font(11.0f));
        responseDisplay->setColour(juce::TextEditor::textColourId, juce::Colours::red);
        updateStatus("Error", true);
    }
}

void DeepSynthPanel::updateStatus(const std::string &status, bool isError)
{
    statusLabel->setText(status, juce::dontSendNotification);
    
    auto skinCtrl = editor->currentSkin;
    if (isError)
    {
        statusLabel->setColour(juce::Label::textColourId, juce::Colours::red);
    }
    else
    {
        auto textColor = skinCtrl->getColor(Colors::Dialog::Label::Text);
        statusLabel->setColour(juce::Label::textColourId, textColor);
    }
}

void DeepSynthPanel::showApiSettings()
{
    // Create a simple panel for API settings
    auto apiKeyDialog = std::make_unique<juce::Component>();
    apiKeyDialog->setSize(450, 200);  // Make wider for Enter button
    
    auto currentKey = claudeClient->getAPIKey();
    
    auto* apiKeyEditor = new juce::TextEditor();
    apiKeyEditor->setText(currentKey);
    apiKeyEditor->setPasswordCharacter('*');
    apiKeyEditor->setMultiLine(false);
    apiKeyEditor->setReturnKeyStartsNewLine(false);
    apiKeyEditor->setBounds(10, 50, 320, 25);  // Make narrower for button
    apiKeyDialog->addAndMakeVisible(apiKeyEditor);
    
    // Add Enter button next to text field
    auto* enterButton = new juce::TextButton("Enter");
    enterButton->setBounds(340, 50, 80, 25);  // Next to text field
    enterButton->setColour(juce::TextButton::buttonColourId, juce::Colours::darkblue);
    apiKeyDialog->addAndMakeVisible(enterButton);
    
    auto* label = new juce::Label({}, "Enter your Claude API key:");
    label->setBounds(10, 20, 380, 25);
    apiKeyDialog->addAndMakeVisible(label);
    
    auto* infoLabel = new juce::Label({}, "Get your API key from: https://console.anthropic.com/");
    infoLabel->setBounds(10, 80, 420, 25);
    infoLabel->setFont(juce::Font(11.0f));
    apiKeyDialog->addAndMakeVisible(infoLabel);
    
    // Add button click handler
    enterButton->onClick = [this, apiKeyEditor]() {
        auto newKey = apiKeyEditor->getText().toStdString();
        claudeClient->setAPIKey(newKey);
        updateStatus("API key updated");
        // Close dialog by finding parent window
        if (auto* topLevel = apiKeyEditor->getTopLevelComponent()) {
            if (auto* dialogWindow = dynamic_cast<juce::DialogWindow*>(topLevel)) {
                dialogWindow->exitModalState(1);
            }
        }
    };
    
    // Add keyboard handler for Enter key
    apiKeyEditor->onReturnKey = [enterButton]() {
        enterButton->triggerClick();
    };
    
    juce::DialogWindow::LaunchOptions options;
    options.content.setOwned(apiKeyDialog.release());
    options.dialogTitle = "DeepSynth API Settings";
    options.componentToCentreAround = this;
    options.dialogBackgroundColour = editor->currentSkin->getColor(Colors::Dialog::Background);
    options.escapeKeyTriggersCloseButton = true;
    options.useNativeTitleBar = false;
    options.resizable = false;
    
    auto* dialog = options.launchAsync();
    
    if (dialog != nullptr)
    {
        dialog->enterModalState(true, juce::ModalCallbackFunction::create(
            [this, apiKeyEditor](int result) {
                if (result == 1) // OK was clicked
                {
                    auto newKey = apiKeyEditor->getText().toStdString();
                    claudeClient->setAPIKey(newKey);
                    updateStatus("API key updated");
                }
            }
        ));
    }
}

DeepSynthPanel::State DeepSynthPanel::getState() const
{
    State state;
    state.hasResponse = hasResponse;
    state.lastResponseText = lastResponseText;
    state.lastPromptText = promptEditor ? promptEditor->getText().toStdString() : "";
    return state;
}

void DeepSynthPanel::setState(const State& state)
{
    hasResponse = state.hasResponse;
    lastResponseText = state.lastResponseText;
    
    if (promptEditor && !state.lastPromptText.empty())
    {
        promptEditor->setText(state.lastPromptText);
    }
    
    if (responseDisplay && hasResponse && !lastResponseText.empty())
    {
        responseDisplay->setText(lastResponseText);
        responseDisplay->setColour(juce::TextEditor::textColourId, juce::Colours::white);
    }
}

} // namespace Panels
} // namespace GUI
} // namespace Surge