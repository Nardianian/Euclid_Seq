#include "ClockManager.h"

void ClockManager::prepare(double sr)
{
    sampleRate = sr;
    samplesPerMidiClock = (60.0 / bpm) * sampleRate / 24.0;
}

void ClockManager::setClockSource(ClockSource src) { source.store(src); }
void ClockManager::setClockType(ClockType t) { type.store(t); }
void ClockManager::setClockRole(ClockRole r) { role.store(r); }

void ClockManager::processMidi(const juce::MidiBuffer& midi)
{
    if (role.load() != ClockRole::Slave)
        return;

    for (const auto meta : midi)
    {
        const auto msg = meta.getMessage();
        if (msg.isMidiClock())
        {
            midiClockCounter++;
        }
    }
}

void ClockManager::processBlock(int numSamples)
{
    sampleCounter += numSamples;

    if (source.load() == ClockSource::Internal)
    {
        internalSamplesPerStep = (60.0 / bpm) * sampleRate / 4.0;
    }
}

double ClockManager::getSamplesPerStep() const
{
    if (source.load() == ClockSource::Internal)
        return internalSamplesPerStep;

    if (source.load() == ClockSource::External && type.load() == ClockType::MidiBeatClock)
        return samplesPerMidiClock * 6.0; // 24 PPQN -> 16th

    return internalSamplesPerStep;
}
