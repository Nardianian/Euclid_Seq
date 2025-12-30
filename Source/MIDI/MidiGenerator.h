#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include "Euclidean.h"
#include "Arp.h"
#include <atomic>
#include <array>
#include <vector>

class MidiGenerator
{
public:
    MidiGenerator()
    {
        rhythmPatterns.resize(6);
        rhythmArps.resize(6);
        arpNotes.resize(6);

        for (int r = 0; r < 6; ++r)
            arpEnabled[r].store(false);
    }

    // ===== DATA =====
    std::vector<EuclideanPattern> rhythmPatterns;
    std::vector<Arp> rhythmArps;
    std::vector<std::vector<int>> arpNotes; // note ARP effettive per ogni riga

    // ===== THREAD-SAFE FLAGS =====
    std::array<std::atomic<bool>, 6> arpEnabled;

    // ===== NOTES RECEIVED BY ARP (GUI / HOST) =====
    std::array<std::atomic<uint64_t>, 6> arpInputNotesMaskLow{};   // note 0–63
    std::array<std::atomic<uint64_t>, 6> arpInputNotesMaskHigh{};  // note 64–127

    // ===== FUNCTIONS =====
    bool triggerStep(int rhythmIndex, int baseNote, int& outMidiNote);
    bool advanceArp(int rhythmIndex, double samplesPerStep, double samplesPerArpStep);

    // ================= THREAD-SAFE RETRIEVAL OF ARP INPUT NOTES =================
    inline std::vector<int> getArpInputNotes(int rhythmIndex) const
    {
        jassert(rhythmIndex >= 0 && rhythmIndex < 6);

        std::vector<int> notes;
        auto lowMask = arpInputNotesMaskLow[rhythmIndex].load();
        auto highMask = arpInputNotesMaskHigh[rhythmIndex].load();

        for (int i = 0; i < 64; ++i)
            if (lowMask & (1ULL << i))
                notes.push_back(i);
        for (int i = 0; i < 64; ++i)
            if (highMask & (1ULL << i))
                notes.push_back(i + 64);

        return notes;
    }

    std::vector<int> getScaleNotes(int scaleID) const;
    std::vector<int> getChordNotes(int chordID) const;

    void updateArpNotes(int scaleID, int chordID);

    // ===== PUBLIC METHODS FOR THE PROCESSOR =====
    void setArpNotes(int rhythmIndex, const std::vector<int>& notes);

    // Returns the actual ARP notes (max 7) of the line
    std::vector<int> getArpNotes(int rhythmIndex) const
    {
        jassert(rhythmIndex >= 0 && rhythmIndex < 6);
        return arpNotes[rhythmIndex];
    }
};

