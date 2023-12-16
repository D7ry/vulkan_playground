#pragma once
#include "spdlog/spdlog.h"
namespace Logging {
void Init();
}


#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define FATAL(...) SPDLOG_CRITICAL(__VA_ARGS__)


#define INIT_LOGS() Logging::Init()