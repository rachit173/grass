#ifndef GRASS_LOGGER_H
#define GRASS_LOGGER_H

#include <string>

#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

using std::string;
using std::shared_ptr;

class GrassLogger {
  private:
    shared_ptr<spdlog::sinks::basic_file_sink_mt> trace_sink_;
    shared_ptr<spdlog::sinks::basic_file_sink_mt> debug_sink_;
    shared_ptr<spdlog::sinks::basic_file_sink_mt> info_sink_;
    shared_ptr<spdlog::sinks::basic_file_sink_mt> warn_sink_;
    shared_ptr<spdlog::sinks::basic_file_sink_mt> error_sink_;
    shared_ptr<spdlog::sinks::stdout_color_sink_mt> console_sink_;
  public:
    shared_ptr<spdlog::logger> main_logger_;

    GrassLogger(string filename) {
        spdlog::drop_all();

        console_sink_ = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink_->set_level(spdlog::level::info);
        console_sink_->set_pattern("[%l] [%x %T.%e] %v");

        trace_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fmt::format("logs/{}_{}.log", filename, "trace"), true);
        trace_sink_->set_level(spdlog::level::trace);
        trace_sink_->set_pattern("[%l] [%x %T.%e] [PID:%P TID:%t] %v");

        debug_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fmt::format("logs/{}_{}.log", filename, "debug"), true);
        debug_sink_->set_level(spdlog::level::debug);
        debug_sink_->set_pattern("[%l] [%x %T.%e] [PID:%P TID:%t] %v");

        info_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fmt::format("logs/{}_{}.log", filename, "info"), true);
        info_sink_->set_level(spdlog::level::info);
        info_sink_->set_pattern("[%l] [%x %T.%e] [PID:%P TID:%t] %v");

        warn_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fmt::format("logs/{}_{}.log", filename, "warn"), true);
        warn_sink_->set_level(spdlog::level::warn);
        warn_sink_->set_pattern("[%l] [%x %T.%e] [PID:%P TID:%t] %v");

        error_sink_ = std::make_shared<spdlog::sinks::basic_file_sink_mt>(fmt::format("logs/{}_{}.log", filename, "error"), true);
        error_sink_->set_level(spdlog::level::err);
        error_sink_->set_pattern("[%l] [%x %T.%e] [PID:%P TID:%t] %v");

        spdlog::sinks_init_list sink_list = {error_sink_, warn_sink_, info_sink_, debug_sink_, trace_sink_, console_sink_};

        main_logger_ = std::make_shared<spdlog::logger>("GrassLogger", sink_list.begin(), sink_list.end());
        main_logger_->set_level(spdlog::level::trace);

        spdlog::register_logger(main_logger_);
        spdlog::flush_every(std::chrono::seconds(1));
    }

    void setConsoleLogLevel(spdlog::level::level_enum level) {
        console_sink_->set_level(level);
    }
};

#endif //GRASS_LOGGER_H