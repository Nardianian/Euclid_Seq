#pragma once
#include <JuceHeader.h>
#include <algorithm>
#include "Euclidean.h"
#include "Arp.h"
#include <atomic>

class MidiGenerator
{
public:
    MidiGenerator()
    {
        rhythmPatterns.resize(6);
        rhythmArps.resize(6);
        arpNotes.resize(6);

        for (int r = 0; r < 6; ++r)
            for (int i = 0; i < 7; ++i)
                arpNoteSlots[r][i].store(-1);

        for (int r = 0; r < 6; ++r)
            arpEnabled[r].store(false);
    }

    std::vector<EuclideanPattern> rhythmPatterns;
    std::vector<Arp> rhythmArps;
    std::vector<std::vector<int>> arpNotes;
    // ===== THREAD-SAFE ARP NOTE SLOTS =====
    // 7 note MIDI reali per rhythm, -1 = slot vuoto
    std::array<std::array<std::atomic<int>, 7>, 6> arpNoteSlots;
    // ===== ARP ENABLE FLAGS (thread-safe read) =====
    std::array<std::atomic<bool>, 6> arpEnabled;

    // ===== NOTES ACTUALLY RECEIVED BY ARP (thread-safe snapshot) =====
// Bitmask 0–127: note MIDI realmente generate dal ritmo
    std::array<std::atomic<uint64_t>, 6> arpInputNotesMaskLow{};   // notes 0–63
    std::array<std::atomic<uint64_t>, 6> arpInputNotesMaskHigh{};  // notes 64–127

    bool triggerStep(int rhythmIndex, int baseNote, int& outMidiNote)
    {
        if (rhythmIndex < 0 || rhythmIndex >= 6)
            return false;

        auto& pattern = rhythmPatterns[rhythmIndex];

        // Euclidean trigger
        if (!pattern.nextStep())
            return false;

        auto& arp = rhythmArps[rhythmIndex];
        auto& slots = arpNoteSlots[rhythmIndex];

        // ===== ARP DISATTIVATO =====
        if (!arpEnabled[rhythmIndex]) // questo flag DEVE esistere (già hai arpActive)
        {
            // ===== REGISTER NOTE AS ARP INPUT =====
            if (baseNote < 64)
                arpInputNotesMaskLow[rhythmIndex].fetch_or(1ULL << baseNote);
            else
                arpInputNotesMaskHigh[rhythmIndex].fetch_or(1ULL << (baseNote - 64));
            outMidiNote = baseNote;
            return true;
        }

        // ===== VERIFICA SE BASE NOTE È SELEZIONATA =====
        bool isSelected = false;
        int selectedCount = 0;

        for (int i = 0; i < 7; ++i)
        {
            int n = slots[i].load();
            if (n >= 0)
            {
                selectedCount++;
                if (n == baseNote)
                    isSelected = true;
            }
        }

        // Nessuna nota selezionata → passa tutto invariato
        if (selectedCount == 0 || !isSelected)
        {
            outMidiNote = baseNote;
            return true;
        }

        // ===== ARP ATTIVO SOLO SU NOTE SELEZIONATE =====
        int step = arp.nextStep() % selectedCount;
        int idx = 0;

        for (int i = 0; i < 7; ++i)
        {
            int n = slots[i].load();
            if (n >= 0)
            {
                if (idx == step)
                {
                    outMidiNote = n;
                    return true;
                }
                idx++;
            }
        }

        // fallback (non dovrebbe mai accadere)
        outMidiNote = baseNote;
        return true;
    }

    bool advanceArp(int rhythmIndex, double samplesPerStep, double samplesPerArpStep)
    {
        if (rhythmIndex < 0 || rhythmIndex >= rhythmArps.size())
            return false;

        return rhythmArps[rhythmIndex].advance(samplesPerStep, samplesPerArpStep);
    }
    // ===== GUI: READ NOTES RECEIVED BY ARP =====
    std::vector<int> getArpInputNotes(int rhythmIndex) const
    {
        std::vector<int> notes;

        if (rhythmIndex < 0 || rhythmIndex >= 6)
            return notes;

        uint64_t low = arpInputNotesMaskLow[rhythmIndex].load();
        uint64_t high = arpInputNotesMaskHigh[rhythmIndex].load();

        for (int i = 0; i < 64; ++i)
            if (low & (1ULL << i))
                notes.push_back(i);

        for (int i = 0; i < 64; ++i)
            if (high & (1ULL << i))
                notes.push_back(i + 64);

        return notes;
    }

    // ===== GUI / HOST: note scale/chord helper =====
    std::vector<int> getScaleNotes(int scaleID) const;
    std::vector<int> getChordNotes(int chordID) const;

    // ===== AGGIUNTA: aggiorna note ARP =====
    void updateArpNotes(int scaleID, int chordID);

    // ===== SET ARP NOTES FROM PROCESSOR (THREAD-SAFE) =====
// notes: vettore di note MIDI già risolte (max 7)
    void setArpNotes(int rhythmIndex, const std::vector<int>& notes)
    {
        if (rhythmIndex < 0 || rhythmIndex >= 6)
            return;

        auto& slots = arpNoteSlots[rhythmIndex];

        for (int i = 0; i < 7; ++i)
        {
            if (i < (int)notes.size())
                slots[i].store(notes[i]);
            else
                slots[i].store(-1);
        }
    }
};




