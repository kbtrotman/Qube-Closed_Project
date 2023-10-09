/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */

#include "q_log.hpp"

	std::shared_ptr<spdlog::logger> q_log::_qlog = nullptr;
    int q_log::exStatus = 0;
    
    q_log::q_log(void) {
        /* Init Logger */	
    }
    
    q_log::~q_log () {
        _qlog->flush();
        exit(q_log::exStatus);
    }

    q_log& q_log::getInstance() {
        static q_log class_instance;
        return class_instance;
    }

    std::shared_ptr<spdlog::logger> q_log::get_log_instance() {

        if (_qlog == nullptr){
            auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>("/var/log/qube.log", 23, 59);
            _qlog = std::make_shared<spdlog::logger>("q_log", file_sink);
            exStatus = 0;

            _qlog->set_level(spdlog::level::trace);
            _qlog->flush_on(spdlog::level::err);
            _qlog->info("=======================================================");
            _qlog->info("Q_LOG::q_log:   ***** Qube logging started. *****   ");
            _qlog->info("=======================================================");
            _qlog->flush();
        }
        
        return _qlog;
    }
