#include "MidiGenerator.h"
// Al momento gran parte del codice é inline
// ===== IMPLEMENTAZIONE =====
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

// ===== HELPER: aggiorna le note ARP basate su scala o accordo =====
void MidiGenerator::updateArpNotes(int scaleID, int chordID)
{
    // Legge le note della scala e dell'accordo
    std::vector<int> scaleNotes = getScaleNotes(scaleID);
    std::vector<int> chordNotes = getChordNotes(chordID);

    for (int r = 0; r < 6; ++r) // Per ogni rhythm
    {
        for (int i = 0; i < 7; ++i) // Ogni slot ARP (7 slot)
        {
            if (i < scaleNotes.size())
            {
                // Scrive la nota della scala nello slot in modo thread-safe
                arpNoteSlots[r][i].store(scaleNotes[i]);
            }
            else
            {
                // Slot vuoto se non ci sono abbastanza note nella scala
                arpNoteSlots[r][i].store(-1);
            }
        }
        // Attiva l'ARP
        arpEnabled[r].store(true);
    }
}

