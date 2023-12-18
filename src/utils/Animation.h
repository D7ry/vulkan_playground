#include <chrono>
namespace Animation
{
        class StopWatch {
                public:
                  StopWatch();
                  void reset();
                  float elapsed() const;

                private:
                        std::chrono::time_point<std::chrono::high_resolution_clock> _start;
        };
}