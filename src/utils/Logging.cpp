#include "Logging.h"
void Logging::Init() {
        spdlog::set_pattern("[%H:%M:%S] [%s:%#] (%!) %v ");
};