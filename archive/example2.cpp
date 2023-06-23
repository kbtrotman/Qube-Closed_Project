
#include "spdlog/spdlog.h"
#include "spdlog/sinks/daily_file_sink.h"

class qube_log {

	public:
		static bool logger_init;
		int exStatus = 0;
		static std::shared_ptr<spdlog::logger> _qlog;

		qube_log() {
			/* Init Logger */
			if (!logger_init) {
				auto file_sink = std::make_shared<spdlog::sinks::daily_file_sink_st>("/var/log/qube.log", 23, 59);
        		_qlog = std::make_shared<spdlog::logger>("qube_log", file_sink);

				_qlog->set_level(spdlog::level::debug);
				_qlog->debug("Qube logging started.");
				logger_init = true;
			}			
		}

		~qube_log () {
			exit(exStatus);
		}
	
};

class qube_psql : public qube_log {
}

class qube_hash : public qube_psql {
}

class qube_fuse : public fuse_operations, public qube_hash {

}

int main( int argc, char *argv[] )
{

 
	int ret = -1;
	qube_fuse qf;

	qf._qlog->debug("==========================");

}