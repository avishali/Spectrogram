#include "PluginProcessor.h"
#include "PluginEditor.h"
#include <cstring>

namespace
{
constexpr int kMinFftOrder = 8;
constexpr int kDefaultFftOrder = 11;

constexpr int kMinOverlap = 1;
constexpr int kMaxOverlap = 8;
constexpr int kDefaultOverlap = 4;

constexpr int kDefaultMode = 0; // Real-time
} // namespace

PluginProcessor::PluginProcessor()
    : AudioProcessor (BusesProperties().withInput ("Input", juce::AudioChannelSet::stereo(), true)
                                      .withOutput ("Output", juce::AudioChannelSet::stereo(), true)),
      apvts_ (*this, nullptr, "PARAMETERS", createParameterLayout())
{
    apvts_.addParameterListener (kParamIdMode, this);
    apvts_.addParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdFftOrder, this);
    apvts_.addParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdOverlap, this);
    apvts_.addParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdWindowType, this);
    apvts_.addParameterListener (mdsp::dsp::SpectrogramSettings::kParamIdFreqScale, this);

    currentSettings_.fftOrder = kDefaultFftOrder;
    currentSettings_.overlap = kDefaultOverlap;
    currentSettings_.windowType = mdsp::dsp::SpectrogramSettings::hann;
    currentSettings_.frequencyScale = mdsp::dsp::SpectrogramSettings::logarithmic;
    currentSettings_.validate();

    // Prepare accumulator with default rate so UI has valid dimensions when editor opens before playback
    constexpr double kDefaultSampleRate = 44100.0;
    offlineSampleRate_ = kDefaultSampleRate;
    spectrogramAccumulator_.prepare (currentSettings_, kDefaultSampleRate);
}

PluginProcessor::~PluginProcessor() = default;

juce::AudioProcessorValueTreeState::ParameterLayout PluginProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> parameters;

    // Analysis Mode (Choice: Real-time/Offline)
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        kParamIdMode,
        "Analysis Mode",
        juce::StringArray ({ "Real-time", "Offline" }),
        kDefaultMode));

    // FFT Order (Choice: 8-14)
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        mdsp::dsp::SpectrogramSettings::kParamIdFftOrder,
        "FFT Order",
        juce::StringArray ({ "8", "9", "10", "11", "12", "13", "14" }),
        kDefaultFftOrder - kMinFftOrder));

    // Overlap (Choice: 1-8)
    juce::StringArray overlapChoices;
    for (int i = kMinOverlap; i <= kMaxOverlap; ++i)
        overlapChoices.add (juce::String (i));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        mdsp::dsp::SpectrogramSettings::kParamIdOverlap,
        "Time Overlap",
        overlapChoices,
        kDefaultOverlap - kMinOverlap));

    // Window Type (Choice)
    juce::StringArray windowChoices;
    windowChoices.add ("Hann");
    windowChoices.add ("Blackman-Harris");
    windowChoices.add ("Flat Top");
    windowChoices.add ("Rectangular");
    windowChoices.add ("Kaiser");
    windowChoices.add (juce::String::fromUTF8 ("Cosine\xc2\xb3"));
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        mdsp::dsp::SpectrogramSettings::kParamIdWindowType,
        "Window Type",
        windowChoices,
        0));

    // Frequency Scale (Choice: Linear/Logarithmic/Mel/Bark)
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        mdsp::dsp::SpectrogramSettings::kParamIdFreqScale,
        "Frequency Scale",
        juce::StringArray ({ "Linear", "Logarithmic", "Mel", "Bark" }),
        1));

    // Color Map (Choice: display-only, persisted for state recall)
    parameters.push_back (std::make_unique<juce::AudioParameterChoice> (
        kParamIdColorMap,
        "Color Map",
        juce::StringArray ({ "Theme", "Fire", "Ice", "Grayscale", "Viridis", "Magma", "Inferno", "Plasma" }),
        0));

    return { parameters.begin(), parameters.end() };
}

void PluginProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    juce::ignoreUnused (samplesPerBlock);
    
    // Store sample rate first so parameterChanged can use it
    offlineSampleRate_ = sampleRate;
    
    updateSpectrogramSettings();
    
    juce::Logger::writeToLog ("[PluginProcessor] prepareToPlay: sampleRate=" + juce::String (sampleRate) + 
                              ", FFT order=" + juce::String (currentSettings_.fftOrder) + 
                              ", overlap=" + juce::String (currentSettings_.overlap) +
                              ", windowType=" + juce::String (static_cast<int> (currentSettings_.windowType)) +
                              ", freqScale=" + juce::String (currentSettings_.frequencyScale == mdsp::dsp::SpectrogramSettings::linear ? "linear" : "logarithmic"));
    
    spectrogramAccumulator_.prepare (currentSettings_, sampleRate);
    settingsDirty_.store (false, std::memory_order_release);
    
    const int fftSize = 1 << currentSettings_.fftOrder;
    juce::Logger::writeToLog ("[PluginProcessor] prepareToPlay: Accumulator prepared, FFT size=" + juce::String (fftSize));
}

void PluginProcessor::releaseResources()
{
    // When playback stops, process buffered audio if in offline mode
    if (getAnalysisMode() == AnalysisMode::offline && hasOfflineData())
    {
        processOfflineBuffer();
    }
}

bool PluginProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
    return layouts.getMainInputChannelSet() == layouts.getMainOutputChannelSet();
}

void PluginProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ignoreUnused (midiMessages);

    // Note: Settings updates are deferred to prepareToPlay() to avoid:
    // 1. Memory allocation (prepare() allocates buffers and creates FFT objects)
    // 2. Mutex locking (updateSpectrogramSettings() uses CriticalSection)
    // Parameter changes will take effect when playback restarts

    // Safe early-out for empty buffers
    if (buffer.getNumChannels() == 0 || buffer.getNumSamples() == 0)
        return;

    juce::ScopedNoDenormals noDenormals;
    const int totalNumInputChannels = getTotalNumInputChannels();
    const int totalNumOutputChannels = getTotalNumOutputChannels();
    const int numSamples = buffer.getNumSamples();

    // Feed first channel to spectrogram accumulator for analysis
    if (totalNumInputChannels > 0)
    {
        const float* channelData = buffer.getReadPointer (0);
        
        const auto mode = getAnalysisMode();
        if (mode == AnalysisMode::realTime)
        {
            // Check if prepare is pending - if so, skip feeding samples to avoid race condition
            if (! preparePending_.load (std::memory_order_acquire))
            {
                // Try to acquire lock (non-blocking check)
                // If prepare is happening, skip this block
                juce::ScopedTryLock tryLock (accumulatorLock_);
                if (tryLock.isLocked())
                {
                    // Real-time: push samples directly to accumulator
                    static int sampleCount = 0;
                    sampleCount += numSamples;
                    
                    for (int i = 0; i < numSamples; ++i)
                        spectrogramAccumulator_.pushSample (channelData[i]);
                    
                    if (sampleCount % 44100 == 0) // Log every second at 44.1kHz
                    {
                        const bool blockReady = spectrogramAccumulator_.isNextBlockReady();
                        juce::Logger::writeToLog ("[PluginProcessor] processBlock: Pushed " + juce::String (sampleCount) + 
                                                " samples, blockReady=" + juce::String (blockReady ? "true" : "false"));
                    }
                }
                // If lock couldn't be acquired, prepare is in progress - skip this block
            }
            // If preparePending_ is true, skip feeding samples until prepare completes
        }
        else // offline
        {
            // Offline: buffer samples for later processing
            juce::ScopedLock lock (offlineBufferLock_);
            offlineAudioBuffer_.insert (offlineAudioBuffer_.end(), channelData, channelData + numSamples);
        }
    }

    // Audio passthrough: copy input channels to output channels
    // When buses are connected, input/output share the same buffer (no copy needed)
    // When buses are separate, we need to explicitly copy
    const int channelsToCopy = juce::jmin (totalNumInputChannels, totalNumOutputChannels);
    for (int channel = 0; channel < channelsToCopy; ++channel)
    {
        const float* input = buffer.getReadPointer (channel);
        float* output = buffer.getWritePointer (channel);
        
        // Only copy if input and output are different buffers (separate buses)
        if (input != output)
            std::memcpy (output, input, static_cast<size_t> (numSamples) * sizeof (float));
    }

    // Clear any output channels that don't have corresponding input channels
    for (int channel = totalNumInputChannels; channel < totalNumOutputChannels; ++channel)
        buffer.clear (channel, 0, numSamples);
}

juce::AudioProcessorEditor* PluginProcessor::createEditor()
{
    return new PluginEditor (*this);
}

bool PluginProcessor::hasEditor() const
{
    return true;
}

const juce::String PluginProcessor::getName() const
{
    return JucePlugin_Name;
}

bool PluginProcessor::acceptsMidi() const
{
    return false;
}

bool PluginProcessor::producesMidi() const
{
    return false;
}

bool PluginProcessor::isMidiEffect() const
{
    return false;
}

double PluginProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int PluginProcessor::getNumPrograms()
{
    return 1;
}

int PluginProcessor::getCurrentProgram()
{
    return 0;
}

void PluginProcessor::setCurrentProgram (int index)
{
    juce::ignoreUnused (index);
}

const juce::String PluginProcessor::getProgramName (int index)
{
    juce::ignoreUnused (index);
    return {};
}

void PluginProcessor::changeProgramName (int index, const juce::String& newName)
{
    juce::ignoreUnused (index, newName);
}

void PluginProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = apvts_.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void PluginProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xmlState (getXmlFromBinary (data, sizeInBytes));

    if (xmlState.get() != nullptr)
        if (xmlState->hasTagName (apvts_.state.getType()))
            apvts_.replaceState (juce::ValueTree::fromXml (*xmlState));
}

void PluginProcessor::parameterChanged (const juce::String& parameterID, float newValue)
{
    juce::ignoreUnused (newValue);

    if (parameterID == kParamIdMode)
    {
        // Mode change: if switching to offline, process buffered data
        if (getAnalysisMode() == AnalysisMode::offline && hasOfflineData())
        {
            processOfflineBuffer();
        }
    }
    else if (parameterID == mdsp::dsp::SpectrogramSettings::kParamIdFftOrder ||
             parameterID == mdsp::dsp::SpectrogramSettings::kParamIdOverlap ||
             parameterID == mdsp::dsp::SpectrogramSettings::kParamIdWindowType ||
             parameterID == mdsp::dsp::SpectrogramSettings::kParamIdFreqScale)
    {
        // Update settings
        updateSpectrogramSettings();
        
        // Re-prepare accumulator with new settings if sample rate is available
        // Use async call to safely prepare on message thread while audio thread may be active
        if (offlineSampleRate_ > 0.0)
        {
            // Set flag to indicate prepare is pending (audio thread will skip feeding samples)
            preparePending_.store (true, std::memory_order_release);
            
            // Schedule prepare on message thread to avoid race condition with audio thread
            juce::MessageManager::callAsync ([this]() {
                if (preparePending_.load (std::memory_order_acquire))
                {
                    // Acquire lock to ensure audio thread isn't using accumulator
                    const juce::ScopedLock lock (accumulatorLock_);
                    
                    // Re-prepare accumulator with new settings
                    spectrogramAccumulator_.prepare (currentSettings_, offlineSampleRate_);
                    settingsDirty_.store (false, std::memory_order_release);
                    preparePending_.store (false, std::memory_order_release);
                    
                    const int newFftSize = 1 << currentSettings_.fftOrder;
                    
                    juce::Logger::writeToLog (juce::String ("[PluginProcessor] parameterChanged (async): Settings updated and accumulator re-prepared") +
                                            ", FFT order=" + juce::String (currentSettings_.fftOrder) +
                                            ", FFT size=" + juce::String (newFftSize) +
                                            ", windowType=" + juce::String (static_cast<int> (currentSettings_.windowType)) +
                                            ", freqScale=" + juce::String (currentSettings_.frequencyScale == mdsp::dsp::SpectrogramSettings::linear ? "linear" : "logarithmic"));
                }
            });
        }
        else
        {
            // Sample rate not set yet, mark as dirty for next prepareToPlay
            settingsDirty_.store (true, std::memory_order_release);
        }
    }
}

void PluginProcessor::updateSpectrogramSettings()
{
    const juce::ScopedLock lock (settingsLock_);

    auto* fftOrderParam = dynamic_cast<juce::AudioParameterChoice*> (
        apvts_.getParameter (mdsp::dsp::SpectrogramSettings::kParamIdFftOrder));
    if (fftOrderParam != nullptr)
        currentSettings_.fftOrder = kMinFftOrder + fftOrderParam->getIndex();

    auto* overlapParam = dynamic_cast<juce::AudioParameterChoice*> (
        apvts_.getParameter (mdsp::dsp::SpectrogramSettings::kParamIdOverlap));
    if (overlapParam != nullptr)
        currentSettings_.overlap = kMinOverlap + overlapParam->getIndex();

    auto* windowTypeParam = dynamic_cast<juce::AudioParameterChoice*> (
        apvts_.getParameter (mdsp::dsp::SpectrogramSettings::kParamIdWindowType));
    if (windowTypeParam != nullptr)
    {
        const int choiceIndex = juce::jlimit (0, 5, windowTypeParam->getIndex());
        currentSettings_.windowType = static_cast<mdsp::dsp::SpectrogramSettings::WindowType> (choiceIndex);
    }

    auto* freqScaleParam = dynamic_cast<juce::AudioParameterChoice*> (
        apvts_.getParameter (mdsp::dsp::SpectrogramSettings::kParamIdFreqScale));
    if (freqScaleParam != nullptr)
    {
        const int choiceIndex = juce::jlimit (0, 3, freqScaleParam->getIndex());
        currentSettings_.frequencyScale = static_cast<mdsp::dsp::SpectrogramSettings::FrequencyScale> (choiceIndex);
    }

    currentSettings_.validate();
}

PluginProcessor::AnalysisMode PluginProcessor::getAnalysisMode() const
{
    auto* modeParam = dynamic_cast<juce::AudioParameterChoice*> (
        apvts_.getParameter (kParamIdMode));
    if (modeParam != nullptr)
        return static_cast<AnalysisMode> (modeParam->getIndex());
    return AnalysisMode::realTime;
}

void PluginProcessor::processOfflineBuffer()
{
    if (offlineAudioBuffer_.empty())
        return;
    
    std::vector<float> bufferCopy;
    {
        juce::ScopedLock lock (offlineBufferLock_);
        if (offlineAudioBuffer_.empty())
            return;
        
        // Copy buffer to process outside the lock
        bufferCopy = offlineAudioBuffer_;
        offlineAudioBuffer_.clear();
        offlineAudioBuffer_.shrink_to_fit();
    }
    
    // Ensure accumulator is prepared with current settings
    updateSpectrogramSettings();
    spectrogramAccumulator_.prepare (currentSettings_, offlineSampleRate_);
    
    // Process all buffered samples
    // This will generate FFT blocks that the UI component can consume
    for (float sample : bufferCopy)
        spectrogramAccumulator_.pushSample (sample);
}

void PluginProcessor::clearOfflineBuffer()
{
    juce::ScopedLock lock (offlineBufferLock_);
    offlineAudioBuffer_.clear();
    offlineAudioBuffer_.shrink_to_fit();
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new PluginProcessor();
}
