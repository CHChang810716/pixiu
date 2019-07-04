#pragma once
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <boost/filesystem.hpp>
#include <nlohmann/json.hpp>
#include <iostream>
#include <fstream>

#define LOGGER_MEM(id) \
template<class... Args> \
void id(Args&&... args) const {\
}

namespace httpservpp {

struct Logger {
    static auto make_log(
        const std::string& name, 
        const std::string& level
    ) {
        auto logger = std::make_shared<spdlog::logger>(name, console_sink(), file_sink());
        logger->set_level(spdlog::level::from_str(level));
        return logger;
    }
    static void config(const boost::filesystem::path& fp) {
        std::ifstream fin(fp.string());
        nlohmann::json config;
        fin >> config;
        auto global_level_str = config.at("level").get<std::string>();
        console_sink()->set_level(spdlog::level::from_str(global_level_str));
        file_sink()->set_level(spdlog::level::from_str(global_level_str));
        for(auto itr = config.begin(); itr != config.end(); itr ++ ) {
            auto mod = itr.key();
            auto& param = *itr;
            auto level = param.value("level", global_level_str);
            auto logger = make_log(mod, level);
            // TODO: pattern

            spdlog::register_logger(logger);
        }
        spdlog::register_logger(make_log("default", "info"));
    }
    static spdlog::logger& get(const std::string& mod) {
        auto ptr = spdlog::get(mod);
        if(ptr) {
            return *ptr;
        } else {
            return *spdlog::get("default");
        }
    }
private:
    static spdlog::sink_ptr console_sink(){
        static auto inst = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        return inst;
    }
    static spdlog::sink_ptr file_sink() {
        static auto inst = std::make_shared<spdlog::sinks::rotating_file_sink_mt> ("http.log", 1024*5, 10);
        return inst;
    }
};

}

#undef LOGGER_MEM