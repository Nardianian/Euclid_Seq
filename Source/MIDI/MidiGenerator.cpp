#include "MidiGenerator.h"
// At the moment most of the code is inline
// ===== IMPLEMENTATION =====
std::vector<int> MidiGenerator::getScaleNotes(int scaleID) const
{
    switch (scaleID)
    {
    case 0: // C maggiore
        return { 60, 62, 64, 65, 67, 69, 71, 72 };
    case 1: // D dorico
        return { 62, 64, 65, 67, 69, 71, 72, 74 };
    default:
        return { 60 }; // fallback sicuro
    }
}

std::vector<int> MidiGenerator::getChordNotes(int chordID) const
{
    switch (chordID)
    {
    case 0: // C maggiore
        return { 60, 64, 67 };
    case 1: // A minore
        return { 57, 60, 64 };
    default:
        return { 60 }; // fallback sicuro
    }
}

// ===== HELPER: Update ARP notes based on scale or chord =====
void MidiGenerator::updateArpNotes(int scaleID, int chordID)
{
    // Read the notes of the scale and chord
    std::vector<int> scaleNotes = getScaleNotes(scaleID);
    std::vector<int> chordNotes = getChordNotes(chordID);

    for (int r = 0; r < 6; ++r) // Per ogni rhythm
    {
        for (int i = 0; i < 7; ++i) // Ogni slot ARP (7 slot)
        {
            if (i < scaleNotes.size())
            {
                // Writes the scale note to the slot in a thread-safe way
                arpNoteSlots[r][i].store(scaleNotes[i]);
            }
            else
            {
                // Empty slot if there are not enough notes in the scale
                arpNoteSlots[r][i].store(-1);
            }
        }
        // Active the ARP
        arpEnabled[r].store(true);
    }
}


