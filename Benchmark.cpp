#include "Globals.h"
#include <cstdio>
#include <iostream>
#include <string>

namespace
{
    enum class Phase { Idle, TechniqueOn, TechniqueOff };

    const double kPhaseSeconds = 5.0;      
    Phase  g_phase = Phase::Idle;
    double g_phaseStart = 0.0;           
    int    g_frameCount = 0;             
    float  g_fpsOn = 0.0f;          
    float  g_fpsOff = 0.0f;          
    bool   g_previousState = true;         

    void beginPhase(Phase phase, bool techniqueEnabled)
    {
        g_phase = phase;
        g_surfaceFx = techniqueEnabled;
        g_phaseStart = glfwGetTime();
        g_frameCount = 0;
    }

    void writeReport()
    {
        float diffPercent = (g_fpsOff > 0.0f) ? (g_fpsOff - g_fpsOn) / g_fpsOff * 100.0f : 0.0f;

        char line[256];
        snprintf(line, sizeof(line),
            "Benchmark: technique ON = %.1f FPS | technique OFF = %.1f FPS | cost = %.1f%%",
            g_fpsOn, g_fpsOff, diffPercent);

        std::cout << line << "\n";
        pushToast(line);

        std::ofstream out("benchmark_results.txt", std::ios::app);
        if (out)
        {
            out << "GPU: " << glGetString(GL_RENDERER) << "\n";
            out << "Resolution: " << g_fbW << "x" << g_fbH << "\n";
            out << line << "\n\n";
        }
        else
        {
            std::cout << "Could not write benchmark_results.txt\n";
        }
    }
}

bool benchmarkActive()
{
    return g_phase != Phase::Idle;
}

void benchmarkStart()
{
    if (g_phase != Phase::Idle)
        return;

    g_previousState = g_surfaceFx;
    std::cout << "Benchmark started: " << kPhaseSeconds << " s with technique ON, then "
        << kPhaseSeconds << " s with technique OFF. Do not touch the camera.\n";
    pushToast("Benchmark started (10 seconds)...");
    beginPhase(Phase::TechniqueOn, true);
}

void benchmarkUpdate()
{
    if (g_phase == Phase::Idle)
        return;

    g_frameCount++;
    double elapsed = glfwGetTime() - g_phaseStart;
    if (elapsed < kPhaseSeconds)
        return;

    float averageFps = (float)((double)g_frameCount / elapsed);

    if (g_phase == Phase::TechniqueOn)
    {
        g_fpsOn = averageFps;
        beginPhase(Phase::TechniqueOff, false);
    }
    else
    {
        g_fpsOff = averageFps;
        g_surfaceFx = g_previousState;
        g_phase = Phase::Idle;
        writeReport();
    }
}

std::string benchmarkStatusText()
{
    if (g_phase == Phase::Idle)
        return "";

    double elapsed = glfwGetTime() - g_phaseStart;
    int remaining = (int)(kPhaseSeconds - elapsed) + 1;
    if (remaining < 0) remaining = 0;

    std::string text = "BENCHMARK: technique ";
    text += (g_phase == Phase::TechniqueOn) ? "ON" : "OFF";
    text += " (" + std::to_string(remaining) + " s)";
    return text;
}