// For Logging!
#include "spdlog/spdlog.h"
#include "spdlog/cfg/env.h"  // support for loading levels from the environment variableinclude "spdlog/fmt/ostr.h" // support for user defined types
#include "spdlog/sinks/daily_file_sink.h"


class log {
	public:
		std::string filename = "/var/log/name-redacted.log";
		log () {
			auto daily_log = spdlog::daily_logger_st("daily_logger", filename, 0, 1 );
		}
        int fout (std::string mes_out) {
            spdlog::qlog->debug(mes_out);
            return 0;
        }
} daily_log;


int main( int argc, char *argv[] )
{
    daily_log.fout("Logging Started.");
}
