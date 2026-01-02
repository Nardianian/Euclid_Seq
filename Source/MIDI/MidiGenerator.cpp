#include "MidiGenerator.h"

// ================= MIDI GENERATOR IMPLEMENTATION =================

// ===== SCALE / CHORD =====
std::vector<int> MidiGenerator::getScaleNotes(int scaleID) const
{
    switch (scaleID)
    {
    case 0: return { 60, 62, 64, 65, 67, 69, 71, 72 }; // C maggiore
    case 1: return { 62, 64, 65, 67, 69, 71, 72, 74 }; // D dorico
    default: return { 60 };
    }
}

std::vector<int> MidiGenerator::getChordNotes(int chordID) const
{
    switch (chordID)
    {
    case 0: return { 60, 64, 67 }; // C maggiore
    case 1: return { 57, 60, 64 }; // A minore
    default: return { 60 };
    }
}
// ===== SET ARP INPUT NOTES (DO NOT TOUCH USER SELECTION) =====
void MidiGenerator::setArpInputNotes(int rhythmIndex, const std::vector<int>& notes)
{
    if (rhythmIndex < 0 || rhythmIndex >= 6)
        return;

    uint64_t lowMask = 0;
    uint64_t highMask = 0;

    for (int note : notes)
    {
        if (note < 0 || note > 127)
            continue;

        if (note < 64)
            lowMask |= (1ULL << note);
        else
            highMask |= (1ULL << (note - 64));
    }

    arpInputNotesMaskLow[rhythmIndex].store(lowMask);
    arpInputNotesMaskHigh[rhythmIndex].store(highMask);
}
// ===== SET ARP NOTES (THREAD-SAFE LOGIC) =====
void MidiGenerator::setArpNotes(int rhythmIndex, const std::vector<int>& notes)
{
    if (rhythmIndex < 0 || rhythmIndex >= 6)
        return;

    arpNotes[rhythmIndex].clear();

    for (int n : notes)
    {
        if (n >= 0 && n <= 127)
            arpNotes[rhythmIndex].push_back(n);

        if ((int)arpNotes[rhythmIndex].size() >= 7)
            break;
    }
}

bool MidiGenerator::advanceArp(int rhythmIndex,
    double samplesPerStep,
    double samplesPerArpStep)
{
    if (rhythmIndex < 0 || rhythmIndex >= (int)rhythmArps.size())
        return false;

    return rhythmArps[rhythmIndex].advance(samplesPerStep, samplesPerArpStep);
}

