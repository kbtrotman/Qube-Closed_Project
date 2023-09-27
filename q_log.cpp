/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

#include "q_log.hpp"

    q_log* q_log::class_instance = nullptr;
    std::shared_ptr<spdlog::logger> q_log::_qlog;

    q_log::q_log(void) {
        /* Init Logger */
		
    }

    q_log* q_log::get_log_instance() {

        if (class_instance == nullptr) {
            class_instance = new q_log();
            auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>("/var/log/qube.log", 23, 59);
            _qlog = std::make_shared<spdlog::logger>("qube_log", file_sink);
            exStatus = 0;

            _qlog->set_level(spdlog::level::trace);
            _qlog->flush_on(spdlog::level::err);
            _qlog->info("==========================================================");
            _qlog->info("QUBE_LOG::qube_log:   ***** Qube logging started. *****   ");
            _qlog->info("==========================================================");
            _qlog->flush();
        }
        
        return class_instance;
    }

    q_log::~q_log () {
        _qlog->flush();
        exit(exStatus);
    }
