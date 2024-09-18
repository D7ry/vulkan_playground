#pragma once
#include <map>

#include "imgui.h"

#include "Profiler.h"

class DeltaTimer;

// Draws framerate and performance profiling metrics
// The perf plot should work hand-in-hand with Profiler class,
// which performs actual stats collection
class ImGuiPerfPlot
{
  public:
    void Draw(
        const std::unique_ptr<std::vector<Profiler::Entry>>& lastProfileData,
        const double deltaTimeSeconds,
        float timeSinceStartSeconds
    );

  private:
    struct ScrollingBuffer
    {
        int MaxSize;
        int Offset;
        ImVector<ImVec2> Data;

        // need as many frame to hold the points
        ScrollingBuffer(
            int max_size
            = DEFAULTS::PROFILER_PERF_PLOT_RANGE_SECONDS * DEFAULTS::MAX_FPS
        ) {
            MaxSize = max_size;
            Offset = 0;
            Data.reserve(MaxSize);
        }

        void AddPoint(float x, float y) {
            if (Data.size() < MaxSize)
                Data.push_back(ImVec2(x, y));
            else {
                Data[Offset] = ImVec2(x, y);
                Offset = (Offset + 1) % MaxSize;
            }
        }

        void Erase() {
            if (Data.size() > 0) {
                Data.shrink(0);
                Offset = 0;
            }
        }
    };

    std::map<const char*, ScrollingBuffer> _scrollingBuffers;

    // shows the profiler plot; note on 
    // lower-end systems the profiler plot itself consumes
    // CPU cycles (~2ms on a M3 mac)
    bool _wantShowPerfPlot = true;
};
