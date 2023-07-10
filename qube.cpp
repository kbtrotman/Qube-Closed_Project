/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */
 
#include "qube.hpp"
// For Logging!
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
// Dedupe & crypto algorythms
#include <openssl/sha.h>

// Globals


// Functions & classes

//Classes -> Lowest Level to highest.
class qube_log {

	public:

		static std::shared_ptr<spdlog::logger> _qlog;

		qube_log() {
			/* Init Logger */
			if (!logger_init) {
				auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>("/var/log/qube.log", 23, 59);
        		_qlog = std::make_shared<spdlog::logger>("qube_log", file_sink);

				_qlog->set_level(spdlog::level::debug);
				_qlog->debug("Qube logging started.");
				_qlog->flush();
				logger_init = true;
			}			
		}

		~qube_log () {
			_qlog->flush();
			exit(exStatus);
		}
	
	private:
		static bool logger_init;
		static int exStatus;
};
std::shared_ptr<spdlog::logger> qube_log::_qlog; // initialize static member
bool qube_log::logger_init = false;
int qube_log::exStatus = 0;

#include "q_psql.hpp"

class qube_hash : public qube_psql {

	public:

		qube_hash () {
			_qlog->debug("Hash Init--->");
			_qlog->flush();
		}

		static std::string get_sha512_hash(const std::string str){
			_qlog->debug("qube_hash::get_sha512_hash---[{}]--->", str);
  			SHA512_CTX sha512;
  			SHA512_Init(&sha512);
  			SHA512_Update(&sha512, str.c_str(), str.size());
  			SHA512_Final(hash, &sha512);

			for(int i = 0; i < SHA512_DIGEST_LENGTH; i++){
				ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
			}
			_qlog->debug("qube_hash::get_sha512_hash---[Leaving]---with hash string---[{}]--->.", ss.str());
  			return ss.str();
		}

	private:
		static unsigned char hash[SHA512_DIGEST_LENGTH];
  		static std::stringstream ss;
};
unsigned char qube_hash::hash[SHA512_DIGEST_LENGTH] = {};
std::stringstream qube_hash::ss=std::stringstream{};
#include "q_fuse.hpp"


// Main entry
int main( int argc, char *argv[] )
{

	qube_fuse qf;

	qube_log::_qlog->info("===============================\n");
	qube_log::_qlog->info("=== PG Server version = {:d} ===", qf.qpsql_getServerVers());
	qube_log::_qlog->info("===============================\n");	

    qube_log::_qlog->info("=================================");
    qube_log::_qlog->info("=====  Filesystem Starting ======");
    qube_log::_qlog->info("=================================");

	// Startup the filesystem
  	return qf.run(argc, argv);

}