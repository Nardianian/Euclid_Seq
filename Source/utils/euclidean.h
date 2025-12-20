/*
  ==============================================================================

    euclidean.h
    Created: 9 Oct 2022 9:05:42pm
    Author:  Andreas Sandersen
    Modified: 13 Dec 2025 0.10:00 am by Nard.
  ==============================================================================
*/
#pragma once
#include <vector>
#include <cmath>

class EuclideanPattern
{
public:
    EuclideanPattern() = default;

    void setPattern(int steps, int pulses)
    {
        numSteps = steps;
        numPulses = pulses;
        generate();
        currentStep = 0;
    }

    bool nextStep()
    {
        if (pattern.empty()) return false;
        bool result = pattern[currentStep];
        currentStep = (currentStep + 1) % numSteps;
        return result;
    }

    void reset() { currentStep = 0; }

    const std::vector<bool>& getPattern() const { return pattern; }

private:
    int numSteps = 16;
    int numPulses = 4;
    int currentStep = 0;
    std::vector<bool> pattern;

    void generate()
    {
        pattern.clear();
        pattern.resize(numSteps, false);
        if (numPulses <= 0) return;

        int bucket = 0;
        for (int i = 0; i < numSteps; ++i)
        {
            bucket += numPulses;
            if (bucket >= numSteps)
            {
                bucket -= numSteps;
                pattern[i] = true;
            }
        }
    }
};
#pragma once

