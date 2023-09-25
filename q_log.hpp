/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

#pragma once

// For Logging!
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/bin_to_hex.h"

#define TRACE q_log::get_new_log_instance()._qlog->trace
#define DEBUG q_log::get_new_log_instance()._qlog->debug
#define INFO q_log::get_new_log_instance()._qlog->info
#define WARN q_log::get_new_log_instance()._qlog->warn
#define ERROR q_log::get_new_log_instance()._qlog->error
#define CRITICAL q_log::get_new_log_instance()._qlog->critical
#define FLUSH q_log::get_new_log_instance()._qlog->flush()

class q_log {

	public:
        static std::shared_ptr<spdlog::logger> _qlog;
        
        static q_log& get_new_log_instance() {
            static q_log instance; // Create the instance once
            logger_init = true;
            exStatus = 0;
            return instance;
        }

    private:
		q_log() {
			/* Init Logger */
			if (!logger_init) {
				auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>("/var/log/qube.log", 23, 59);
        		_qlog = std::make_shared<spdlog::logger>("qube_log", file_sink);

				_qlog->set_level(spdlog::level::trace);
				_qlog->flush_on(spdlog::level::err);
				INFO("==========================================================");
				INFO("QUBE_LOG::qube_log:   ***** Qube logging started. *****   ");
				INFO("==========================================================");
				FLUSH;
			}			
		}

		~q_log () {
			_qlog->flush();
			exit(exStatus);
		}
	
		static bool logger_init;
		static int exStatus;
};
//std::shared_ptr<spdlog::logger> qube_log::_qlog; // initialize static member