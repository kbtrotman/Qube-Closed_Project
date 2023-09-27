/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

#include "q_log.hpp"

    std::shared_ptr<spdlog::logger> _qlog=nullptr;
    q_log *class_instance=nullptr; // Create the instance once
    bool q_log::logger_init=false;
    int q_log::exStatus=0;
    q_log *QLOG = q_log::get_log_instance();

    q_log::q_log() {
        /* Init Logger */
        if (!logger_init) {
            auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>("/var/log/qube.log", 23, 59);
            _qlog = std::make_shared<spdlog::logger>("qube_log", file_sink);
            exStatus = 0;
            q_log::logger_init = true;

            _qlog->set_level(spdlog::level::trace);
            _qlog->flush_on(spdlog::level::err);
            INFO("==========================================================");
            INFO("QUBE_LOG::qube_log:   ***** Qube logging started. *****   ");
            INFO("==========================================================");
            FLUSH;
        }			
    }

    q_log::~q_log () {
        _qlog->flush();
        exit(exStatus);
    }

    q_log q_log::*get_log_instance() {
        if (class_instance == NULL) {
            class_instance = new q_log();
        }
        return class_instance;
    }
