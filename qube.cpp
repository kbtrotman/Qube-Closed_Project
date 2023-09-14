/**
 * qubeFS
 *
 * A Backup Filesystem
 * By K. B. Trotman
 * License: GNU GPL as of 2023
 *
 */
 
// For Logging!
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"
#include "spdlog/fmt/fmt.h"
#include "spdlog/fmt/bin_to_hex.h"

// Dedupe & crypto algorythms
#include <openssl/sha.h>

// Globals
#include "qube.hpp"

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

				_qlog->set_level(spdlog::level::trace);
				_qlog->flush_on(spdlog::level::err);
				INFO("==========================================================");
				INFO("QUBE_LOG::qube_log:   ***** Qube logging started. *****   ");
				INFO("==========================================================");
				FLUSH;
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

#include "q_convert.hpp"
#include "q_psql.hpp"

class qube_hash : public qube_psql {

	public:

		qube_hash () {
			TRACE("qube_hash::qube_hash: Hash Init--->");
			FLUSH;
		}

		static std::string get_sha512_hash(const std::vector<uint8_t> v_str){
			TRACE("qube_hash::get_sha512_hash---[{}]--->", (char*)v_str.data());
			
			// We have static sections, so let's blank everything to be safe here.
			std::memset(hash, 0, SHA512_DIGEST_LENGTH);
			ss.str("");
			ss.clear();

  			SHA512_CTX sha512;
  			SHA512_Init(&sha512);
  			SHA512_Update(&sha512, v_str.data(), v_str.size());
  			SHA512_Final(hash, &sha512);

			for(int i = 0; i < (SHA512_DIGEST_LENGTH); i++){
				ss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>( hash[i] );
			}
			TRACE("qube_hash::get_sha512_hash---[Leaving]---with hash string---[{}]--->.", ss.str());

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

	qube_log::_qlog->info("====================================");
	qube_log::_qlog->info("===  PG Server version = {:d}  ===", qf.qpsql_getServerVers());
	qube_log::_qlog->info("====================================");	

    qube_log::_qlog->info("=================================");
    qube_log::_qlog->info("=====  Filesystem Starting ======");
    qube_log::_qlog->info("=================================");

	// Startup the filesystem
  	return qf.run(argc, argv);

}