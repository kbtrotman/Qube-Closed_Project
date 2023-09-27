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

#define TRACE QLOG->_qlog->trace
#define DEBUG QLOG->_qlog->debug
#define INFO QLOG->_qlog->info
#define WARN QLOG->_qlog->warn
#define ERROR QLOG->_qlog->error
#define CRITICAL QLOG->_qlog->critical
#define FLUSH  QLOG->_qlog->flush()

class q_log {

	public:
        static std::shared_ptr<spdlog::logger> _qlog;
        static q_log* get_log_instance();


    private:
		q_log();
		~q_log ();
		static q_log *class_instance; // Create the instance once
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