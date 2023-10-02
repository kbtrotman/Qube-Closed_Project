/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 **/

// Globals:  Private Defs, Then Classes
#include "qube.hpp"
#include "q_log.hpp"
    q_log& QLOG = q_log::getInstance();
	static std::shared_ptr<spdlog::logger> _Qlog = QLOG.get_log_instance();
#include "q_fuse.hpp"


// Define all the static variables that need to always be available.

// Main entry
int main( int argc, char *argv[] )
{
	static std::shared_ptr<spdlog::logger> _Qlog = QLOG.get_log_instance();
	q_fuse qf(QLOG);

	INFO("====================================");
	INFO("===  PG Server version = {:d}  ===", qf.qpsql_getServerVers());
	INFO("====================================");	

    INFO("=================================");
    INFO("=====  Filesystem Starting ======");
    INFO("=================================");

	// Startup the filesystem
  	return qf.run(argc, argv);

}