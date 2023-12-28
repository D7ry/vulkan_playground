#pragma once
#include "spdlog/spdlog.h"
namespace Logging {
void Init();
}

#define ENABLE_DEBUG_ASSERT 1

#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#define INFO(...) SPDLOG_INFO(__VA_ARGS__)
#define WARN(...) SPDLOG_WARN(__VA_ARGS__)
#define ERROR(...) SPDLOG_ERROR(__VA_ARGS__)
#define TRACE(...) SPDLOG_TRACE(__VA_ARGS__)
#define DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__)
#define FATAL(...)                                                                                                     \
        SPDLOG_CRITICAL(__VA_ARGS__);                                                                                  \
        throw std::runtime_error("Fatal error: " + fmt::format(__VA_ARGS__))
#define ASSERT(...)                                                                                                    \
        if (!(__VA_ARGS__)) {                                                                                          \
                FATAL("Assertion failed: {}", #__VA_ARGS__);                                                           \
        }
#define DEBUG_ASSERT(...)                                                                                              \
        if (ENABLE_DEBUG_ASSERT) {                                                                                     \
                ASSERT(__VA_ARGS__)                                                                                    \
        }

#define INIT_LOGS() Logging::Init()