#include "implot.h"

#include "ImGuiPerfPlot.h"

void ImGuiPerfPlot::Draw(
    const std::unique_ptr<std::vector<Profiler::Entry>>& lastProfileData,
    const double deltaTimeSeconds,
    float timeSinceStart
) {
    { // Profiler
        ImGui::SeparatorText("Profiler");
        ImGui::Checkbox("Show Perf Plot", std::addressof(_wantShowPerfPlot));
        ImGui::Text("Framerate: %f", 1 / deltaTimeSeconds);

        bool showingPlot = false;
        if (_wantShowPerfPlot) {
            showingPlot = ImPlot::BeginPlot("Profiler");
        }

        if (showingPlot) { // set up x and y boundary
            ImPlot::SetupAxis(ImAxis_X1, "Time(Sec)");
            ImPlot::SetupAxisLimits(
                ImAxis_X1,
                timeSinceStart - DEFAULTS::PROFILER_PERF_PLOT_RANGE_SECONDS,
                timeSinceStart,
                ImPlotCond_Always
            );

            ImPlot::SetupAxis(ImAxis_Y1, "Cost(Ms)");
            // adaptively set y range
            const double deltaTimeMiliseconds = deltaTimeSeconds / 1000;
            ImPlot::SetupAxisLimits(
                ImAxis_Y1,
                0,
                deltaTimeMiliseconds < 15 ? 15 : 30,
                ImPlotCond_Always
            );
        }

        // iterate over entries, update scrolling buffers and plot
        // also shows text for the raw numbers under the plot
        for (Profiler::Entry& entry : *lastProfileData) {
            // ms time
            double ms = std::chrono::
                            duration<double, std::chrono::milliseconds::period>(
                                entry.end - entry.begin
                            )
                                .count();
            if (showingPlot) {
                auto it = _scrollingBuffers.find(entry.name);
                if (it == _scrollingBuffers.end()) {
                    auto res = _scrollingBuffers.emplace(
                        entry.name, ScrollingBuffer()
                    );
                    ASSERT(res.second); // insertion success
                    it = res.first;
                }
                ScrollingBuffer& buf = it->second;

                buf.AddPoint(timeSinceStart, ms);
                ImPlot::PlotLine(
                    entry.name,
                    &buf.Data[0].x,
                    &buf.Data[0].y,
                    buf.Data.size(),
                    0,
                    buf.Offset,
                    2 * sizeof(float)
                );
            }
            { // text section
                int indentWidth = entry.level * 10;
                if (indentWidth != 0) {
                    ImGui::Indent(indentWidth);
                }
                // entry name
                ImGui::Text("%s", entry.name);
                ImGui::Text("%f MS", ms);
                if (indentWidth != 0) {
                    ImGui::Unindent(indentWidth);
                }
            }
        }
        if (showingPlot) {
            ImPlot::EndPlot();
        }
    }
}
