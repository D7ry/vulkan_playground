#pragma once
// absolute constants
const int NUM_FRAME_IN_FLIGHT = 2; // how many frames to pipeline
const int TEXTURE_ARRAY_SIZE
    = 8; // size of the texture array for bindless texture indexing

// default setting values.
// Note taht values are only used on engine initialization
// and is subject to change at runtime.
namespace DEFAULTS
{
#if __APPLE__
const size_t WINDOW_WIDTH = 1280;
const size_t WINDOW_HEIGHT = 720;
#else
const size_t WINDOW_WIDTH = 1440;
const size_t WINDOW_HEIGHT = 900;
#endif // __APPLE__
const float FOV = 90.f;
const float ZNEAR = 0.1f;
const float ZFAR = 100.f;

const float PROFILER_PERF_PLOT_RANGE_SECONDS
    = 10; // how large the plot window is

#if !__APPLE__
const float MAX_FPS = 144.f;
#else
const float MAX_FPS = 60.f;
#endif // __APPLE__

namespace ImGui
{
#if !__APPLE__
const float DEFAULT_FONT_SIZE = 17;
#else
const float DEFAULT_FONT_SIZE = 20;
#endif // __APPLE__
} // namespace ImGui

} // namespace DEFAULTS
