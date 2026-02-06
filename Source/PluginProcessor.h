#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <mdsp_dsp/spectrogram/SpectrogramAccumulator.h>
#include <mdsp_dsp/spectrogram/SpectrogramSettings.h>
#include <atomic>
#include <vector>

class PluginProcessor : public juce::AudioProcessor,
                        private juce::AudioProcessorValueTreeState::Listener
{
public:
    enum class AnalysisMode
    {
        realTime = 0,
        offline = 1
    };

    PluginProcessor();
    ~PluginProcessor() override;

    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    juce::AudioProcessorValueTreeState& getValueTreeState() { return apvts_; }
    mdsp::dsp::SpectrogramAccumulator& getSpectrogramAccumulator() { return spectrogramAccumulator_; }
    
    AnalysisMode getAnalysisMode() const;
    void processOfflineBuffer();
    void clearOfflineBuffer();
    bool hasOfflineData() const { return !offlineAudioBuffer_.empty(); }

    static constexpr const char* kParamIdColorMap = "spectrogram_colorMap";

private:
    static constexpr const char* kParamIdMode = "mode";

    static juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    void parameterChanged (const juce::String& parameterID, float newValue) override;

    void updateSpectrogramSettings();

    juce::AudioProcessorValueTreeState apvts_;
    mdsp::dsp::SpectrogramAccumulator spectrogramAccumulator_;
    mdsp::dsp::SpectrogramSettings currentSettings_;

    std::atomic<bool> settingsDirty_{false};
    std::atomic<bool> preparePending_{false};  // Flag to indicate prepare is in progress
    juce::CriticalSection settingsLock_;
    juce::CriticalSection accumulatorLock_;  // Lock to protect accumulator during prepare
    
    // Offline mode: buffer audio samples
    std::vector<float> offlineAudioBuffer_;
    double offlineSampleRate_ = 0.0;  // 0.0 means not initialized yet
    juce::CriticalSection offlineBufferLock_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PluginProcessor)
};
