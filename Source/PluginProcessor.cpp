#include "PluginProcessor.h"
#include <cmath>

juce::AudioProcessorValueTreeState::ParameterLayout DrumFilterAudioProcessor::makeLayout()
{
    using P = juce::AudioParameterFloat;
    juce::AudioProcessorValueTreeState::ParameterLayout p;
    p.add(std::make_unique<P>("threshold", "Kick threshold (dBFS)", juce::NormalisableRange<float>(-60.f, 0.f, 0.1f), -24.f));
    p.add(std::make_unique<P>("endHz", "End frequency (Hz)", juce::NormalisableRange<float>(30.f, 16000.f, 1.f, 0.35f), 200.f));
    p.add(std::make_unique<P>("decay", "Decay (ms)", juce::NormalisableRange<float>(10.f, 2000.f, 1.f, 0.4f), 150.f));
    p.add(std::make_unique<juce::AudioParameterChoice>("slope", "Slope", juce::StringArray{"6 dB/oct", "12 dB/oct", "18 dB/oct", "24 dB/oct"}, 3));
    return p;
}
DrumFilterAudioProcessor::DrumFilterAudioProcessor()
    : juce::AudioProcessor(juce::AudioProcessor::BusesProperties().withInput("Input", juce::AudioChannelSet::stereo(), true).withOutput("Output", juce::AudioChannelSet::stereo(), true)),
      state(*this, nullptr, "PARAMS", makeLayout()) {}
void DrumFilterAudioProcessor::prepareToPlay(double sampleRate, int)
{
    fs = sampleRate;
    envelope = 1.0; detectorEnv = previousDetectorEnv = 0.0;
    holdoff = coefficientCounter = 0;
    *detectorHigh.state = *juce::dsp::IIR::Coefficients<float>::makeHighPass(fs, 30.0);
    *detectorLow.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(fs, 150.0);
    detectorHigh.reset(); detectorLow.reset();
    for (auto& channel : filters)
        for (auto& filter : channel) { *filter.state = *juce::dsp::IIR::Coefficients<float>::makeLowPass(fs, juce::jmin(16000.0, fs * 0.45)); filter.reset(); }
}
void DrumFilterAudioProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;
    const int channels = juce::jmin(2, buffer.getNumChannels());
    if (channels == 0) return;
    const auto threshold = juce::Decibels::decibelsToGain(state.getRawParameterValue("threshold")->load());
    const auto endHz = static_cast<double>(state.getRawParameterValue("endHz")->load());
    const auto decaySamples = juce::jmax(1.0, fs * state.getRawParameterValue("decay")->load() / 1000.0);
    const int stages = juce::jlimit(1, 4, static_cast<int>(state.getRawParameterValue("slope")->load()) + 1);
    const double maxHz = juce::jmin(20000.0, fs * 0.45);
    for (int i = 0; i < buffer.getNumSamples(); ++i)
    {
        float mono = 0.f;
        for (int ch = 0; ch < channels; ++ch) mono += buffer.getSample(ch, i);
        mono /= static_cast<float>(channels);
        const float kickBand = detectorLow.processSample(detectorHigh.processSample(mono));
        const double magnitude = std::abs(kickBand);
        const double smoothing = std::exp(-1.0 / (fs * (magnitude > detectorEnv ? 0.001 : 0.025)));
        detectorEnv = smoothing * detectorEnv + (1.0 - smoothing) * magnitude;
        if (holdoff > 0) --holdoff;
        if (holdoff == 0 && detectorEnv > threshold && previousDetectorEnv <= threshold)
        {
            envelope = 1.0;
            holdoff = static_cast<int>(fs * 0.08);
        }
        previousDetectorEnv = detectorEnv;
        envelope = juce::jmax(0.0, envelope - 1.0 / decaySamples);
        const double cutoff = endHz * std::pow(maxHz / endHz, envelope);
        // Coefficient updates every 16 samples limit CPU cost; smoothing occurs in envelope.
        if ((coefficientCounter++ & 15) == 0)
        {
            auto coeff = juce::dsp::IIR::Coefficients<float>::makeLowPass(fs, juce::jlimit(20.0, fs * 0.45, cutoff));
            for (auto& channel : filters) for (auto& filter : channel) *filter.state = *coeff;
        }
        for (int ch = 0; ch < channels; ++ch)
        {
            float sample = buffer.getSample(ch, i);
            for (int stage = 0; stage < stages; ++stage) sample = filters[static_cast<size_t>(ch)][static_cast<size_t>(stage)].processSample(sample);
            buffer.setSample(ch, i, sample);
        }
    }
}
juce::AudioProcessorEditor* DrumFilterAudioProcessor::createEditor()
{
    return new juce::GenericAudioProcessorEditor(*this);
}
void DrumFilterAudioProcessor::getStateInformation(juce::MemoryBlock& dest)
{
    if (auto xml = state.copyState().createXml()) copyXmlToBinary(*xml, dest);
}
void DrumFilterAudioProcessor::setStateInformation(const void* data, int size)
{
    if (auto xml = getXmlFromBinary(data, size)) if (xml->hasTagName(state.state.getType())) state.replaceState(juce::ValueTree::fromXml(*xml));
}
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter() { return new DrumFilterAudioProcessor(); }
