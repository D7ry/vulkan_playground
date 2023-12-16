#include "Logging.h"
#include "spdlog/spdlog.h"
void Logging::Init() {
        spdlog::set_pattern("[%H:%M:%S] [%s:%#] (%!) %v ");
};