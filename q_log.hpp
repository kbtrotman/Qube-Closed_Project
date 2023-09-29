/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

#pragma once

// Singleton Instance For Global Logging!
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/bin_to_hex.h"

#define TRACE _Qlog->trace
#define DEBUG _Qlog->debug
#define INFO _Qlog->info
#define WARN _Qlog->warn
#define ERROR _Qlog->error
#define CRITICAL _Qlog->critical
#define FLUSH  _Qlog->flush()

class q_log {

	public:
		static q_log& getInstance();
        static std::shared_ptr<spdlog::logger> get_log_instance();
		static std::shared_ptr<spdlog::logger> _qlog;

    private:
		q_log();
		~q_log ();
		static int exStatus;
};



/**
#define SPDLOG_DEBUGF(logger, fmt, ...) \
{ \
logger->debug("{}::{}()#{}: " fmt, __FILE__ , __FUNCTION__, __LINE__, __VA_ARGS__); \
}

#define SPDLOG_DEBUG(logger, str) \
{ \
logger->debug("{}::{}()#{}: ", __FILE__ , __LINE__, str); \
}

**/