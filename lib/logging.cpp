// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifdef LIBLOG_USE_BOOST_FILESYSTEM
#include <boost/filesystem.hpp>
#else
#include <filesystem>
#endif

#ifdef __linux__
#include <unistd.h>
#endif

#include <fstream>

#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/constant.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/settings_parser.hpp>

#include <spdlog/pattern_formatter.h>
#include <spdlog/sinks/ansicolor_sink.h>
#include <spdlog/sinks/syslog_sink.h>

// FIXME: move this into a detail header so we do not have to expose this to the world
#include <everest/rotating_file_sink.hpp>

#include <everest/exceptions.hpp>
#include <everest/logging.hpp>

// this will only be used while bootstrapping our logging (e.g. the logging settings aren't yet applied)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EVEREST_INTERNAL_LOG_AND_THROW(exception)                                                                      \
    do {                                                                                                               \
        spdlog::critical("{}", (exception).what());                                                                    \
        throw(exception);                                                                                              \
    } while (0);

#ifdef LIBLOG_USE_BOOST_FILESYSTEM
namespace fs = boost::filesystem;
#else
namespace fs = std::filesystem;
#endif
namespace logging = boost::log::BOOST_LOG_VERSION_NAMESPACE;
namespace attrs = logging::attributes;

namespace Everest {
namespace Logging {

inline constexpr auto level_verb = 0;
inline constexpr auto level_debg = 1;
inline constexpr auto level_info = 2;
inline constexpr auto level_warn = 3;
inline constexpr auto level_erro = 4;
inline constexpr auto level_crit = 5;

const std::array severity_strings = {
    std::string("VERB"), //
    std::string("DEBG"), //
    std::string("INFO"), //
    std::string("WARN"), //
    std::string("ERRO"), //
    std::string("CRIT"), //
};

std::array severity_strings_colors = {
    std::string(""), //
    std::string(""), //
    std::string(""), //
    std::string(""), //
    std::string(""), //
    std::string(""), //
};

const std::string clear_color = "\033[0m";

std::string current_process_name;

spdlog::level::level_enum global_level = spdlog::level::level_enum::trace;

std::string get_process_name() {
    std::string process_name;
#ifndef __linux__
    return process_name;
#endif
    // first get pid and use it to get the binary name of the running process
    auto pid = getpid();
    auto proc = "/proc/" + std::to_string(pid) + "/cmdline";
    fs::path proc_path = fs::path(proc);
    std::ifstream cmdline_file(proc_path.c_str());
    if (cmdline_file.is_open()) {
        std::string cmdline;
        cmdline_file >> cmdline;
        auto cmdline_path = fs::path(cmdline);
        process_name = cmdline_path.filename();
    }
    return process_name;
}

void init(const std::string& logconf) {
    init(logconf, "");
}

class Filter {
public:
    explicit Filter(const logging::filter& filter) : filter(filter) {
    }

    bool filter_msg(const spdlog::details::log_msg& msg) {
        auto src = logging::attribute_set();
        // TODO: proper conversion function between msg.level an severity_level
        src["Severity"] = attrs::constant<severity_level>(static_cast<severity_level>(msg.level));
        src["Process"] = attrs::constant<std::string>(msg.logger_name.data());
        // FIXME: support more of the boost log Filter syntax?
        auto set = logging::attribute_value_set(src, logging::attribute_set(), logging::attribute_set());
        return filter(set);
    }

private:
    logging::filter filter;
};

class ConsoleFilterSink : public spdlog::sinks::ansicolor_stdout_sink_mt, public Filter {
public:
    explicit ConsoleFilterSink(const logging::filter& filter) : Filter(filter) {
    }

private:
    void log(const spdlog::details::log_msg& msg) override {
        if (not filter_msg(msg)) {
            return;
        }

        spdlog::sinks::ansicolor_stdout_sink_mt::log(msg);
    }
};

class TextFileFilterSink : public Everest::Logging::rotating_file_sink_mt, public Filter {
public:
    explicit TextFileFilterSink(const logging::filter& filter, spdlog::filename_t base_filename, std::size_t max_size,
                                std::size_t max_files, bool rotate_on_open) :
        rotating_file_sink_mt(base_filename, max_size, max_files, rotate_on_open), Filter(filter) {
    }

private:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (not filter_msg(msg)) {
            return;
        }

        Everest::Logging::rotating_file_sink_mt::sink_it_(msg);
    }
};

class SyslogFilterSink : public spdlog::sinks::syslog_sink_mt, public Filter {
public:
    explicit SyslogFilterSink(const logging::filter& filter, std::string indet, int syslog_option, int syslog_facility,
                              bool enable_formatting) :
        spdlog::sinks::syslog_sink_mt(indet, syslog_option, syslog_facility, enable_formatting), Filter(filter) {
    }

private:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        if (not filter_msg(msg)) {
            return;
        }

        spdlog::sinks::syslog_sink_mt::sink_it_(msg);
    }
};
bool is_level(const logging::filter& filter, severity_level level) {
    auto src = logging::attribute_set();
    src["Severity"] = attrs::constant<severity_level>(level);
    src["Process"] = attrs::constant<std::string>(current_process_name);
    auto set = logging::attribute_value_set(src, logging::attribute_set(), logging::attribute_set());
    return filter(set);
}

spdlog::level::level_enum get_level_from_filter(const logging::filter& filter) {
    if (is_level(filter, severity_level::verbose)) {
        return spdlog::level::level_enum::trace;
    } else if (is_level(filter, severity_level::debug)) {
        return spdlog::level::level_enum::debug;
    } else if (is_level(filter, severity_level::info)) {
        return spdlog::level::level_enum::info;
    } else if (is_level(filter, severity_level::warning)) {
        return spdlog::level::level_enum::warn;
    } else if (is_level(filter, severity_level::error)) {
        return spdlog::level::level_enum::err;
    } else if (is_level(filter, severity_level::critical)) {
        return spdlog::level::level_enum::critical;
    }
    return spdlog::level::level_enum::info;
}

class EverestColorLevelFormatter : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override {
        switch (msg.level) {
        case spdlog::level::level_enum::trace: {
            auto& color = severity_strings_colors.at(level_verb);
            auto& verb = severity_strings.at(level_verb);
            format_message(color, verb, dest);
            break;
        }
        case spdlog::level::level_enum::debug: {
            auto& color = severity_strings_colors.at(level_debg);
            auto& debg = severity_strings.at(level_debg);
            format_message(color, debg, dest);
            break;
        }
        case spdlog::level::level_enum::info: {
            auto& color = severity_strings_colors.at(level_info);
            auto& info = severity_strings.at(level_info);
            format_message(color, info, dest);
            break;
        }
        case spdlog::level::level_enum::warn: {
            auto& color = severity_strings_colors.at(level_warn);
            auto& warn = severity_strings.at(level_warn);
            format_message(color, warn, dest);
            break;
        }
        case spdlog::level::level_enum::err: {
            auto& color = severity_strings_colors.at(level_erro);
            auto& erro = severity_strings.at(level_erro);
            format_message(color, erro, dest);
            break;
        }
        case spdlog::level::level_enum::critical: {
            auto& color = severity_strings_colors.at(level_crit);
            auto& crit = severity_strings.at(level_crit);
            format_message(color, crit, dest);
            break;
        }
        case spdlog::level::level_enum::off:
            [[fallthrough]];
        case spdlog::level::level_enum::n_levels:
            break;
        }
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<EverestColorLevelFormatter>();
    }

private:
    void format_message(const std::string& color, const std::string& loglevel, spdlog::memory_buf_t& dest) {
        if (not color.empty()) {
            dest.append(color.data(), color.data() + color.size());
        }
        dest.append(loglevel.data(), loglevel.data() + loglevel.size());
        if (not color.empty()) {
            dest.append(clear_color.data(), clear_color.data() + clear_color.size());
        }
    }
};

class EverestLevelFormatter : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override {
        switch (msg.level) {
        case spdlog::level::level_enum::trace: {
            auto& verb = severity_strings.at(level_verb);
            format_message(verb, dest);
            break;
        }
        case spdlog::level::level_enum::debug: {
            auto& debg = severity_strings.at(level_debg);
            format_message(debg, dest);
            break;
        }
        case spdlog::level::level_enum::info: {
            auto& info = severity_strings.at(level_info);
            format_message(info, dest);
            break;
        }
        case spdlog::level::level_enum::warn: {
            auto& warn = severity_strings.at(level_warn);
            format_message(warn, dest);
            break;
        }
        case spdlog::level::level_enum::err: {
            auto& erro = severity_strings.at(level_erro);
            format_message(erro, dest);
            break;
        }
        case spdlog::level::level_enum::critical: {
            auto& crit = severity_strings.at(level_crit);
            format_message(crit, dest);
            break;
        }
        case spdlog::level::level_enum::off:
            [[fallthrough]];
        case spdlog::level::level_enum::n_levels:
            break;
        }
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<EverestLevelFormatter>();
    }

private:
    void format_message(const std::string& loglevel, spdlog::memory_buf_t& dest) {
        dest.append(loglevel.data(), loglevel.data() + loglevel.size());
    }
};

class EverestFuncnameFormatter : public spdlog::custom_flag_formatter {
public:
    void format(const spdlog::details::log_msg& msg, const std::tm&, spdlog::memory_buf_t& dest) override {
        if (msg.source.funcname == nullptr) {
            return;
        }
        std::string funcname = msg.source.funcname;
        dest.append(funcname.data(), funcname.data() + funcname.size());
    }

    std::unique_ptr<custom_flag_formatter> clone() const override {
        return spdlog::details::make_unique<EverestFuncnameFormatter>();
    }
};

// The operator parses the severity level from the stream
std::istream& operator>>(std::istream& strm, severity_level& level) {
    if (strm.good()) {
        std::string str;
        strm >> str;

        for (unsigned int i = 0; i < severity_strings.size(); ++i) {
            if (str == severity_strings.at(i)) {
                level = static_cast<severity_level>(i);
                return strm;
            }
        }

        strm.setstate(std::ios_base::failbit);
    }

    return strm;
}

std::vector<std::shared_ptr<spdlog::sinks::sink>> sinks;

void init(const std::string& logconf, std::string process_name) {
    if (process_name.empty()) {
        current_process_name = get_process_name();
    } else {
        current_process_name = process_name;
    }

    // open logging.ini config file located at our base_dir and use it to configure filters and format
    fs::path logging_path = fs::path(logconf);
    std::ifstream logging_config(logging_path.c_str());
    if (!logging_config.is_open()) {
        EVEREST_INTERNAL_LOG_AND_THROW(EverestConfigError(std::string("Could not open logging config file at ") +
                                                          std::string(fs::absolute(logging_path).c_str())));
    }

    auto settings = logging::parse_settings(logging_config);
    logging::register_simple_filter_factory<severity_level>("Severity");

    auto core = settings["Core"].get_section();

    auto filter = core["Filter"].get<std::string>().get_value_or("");

    auto parsed_filter = logging::parse_filter(filter);

    global_level = get_level_from_filter(parsed_filter);

    if (not settings.has_section("Sinks")) {
        std::cerr << "No \"Sinks\" section in the logging configuration, at least one sink has be be present"
                  << std::endl;
        return; // FIXME: throw an exception here?
    }

    for (auto sink : settings["Sinks"].get_section()) {
        auto format = sink["Format"].get<std::string>().get_value_or("%TimeStamp% [%Severity%] %Message%");

        // parse the format into a format string that spdlog can use
        std::vector<std::pair<std::string, std::string>> replacements = {{"%TimeStamp%", "%Y-%m-%d %H:%M:%S.%f"},
                                                                         {"%Process%", "%-15!n"},
                                                                         {"%ProcessID%", "%P"},
                                                                         {"%Severity%", "%l"},
                                                                         {"%ThreadID%", "%t"},
                                                                         {"%function%", "%!"},
                                                                         {"%file%", "%s"},
                                                                         {"%line%", "%#"},
                                                                         {"%Message%", "%v"}};
        for (auto& replace : replacements) {
            auto pos = format.find(replace.first);
            while (pos != std::string::npos) {
                format.replace(pos, replace.first.size(), replace.second);
                pos = format.find(replace.first, pos + replace.second.size());
            }
        }

        auto formatter = std::make_unique<spdlog::pattern_formatter>();
        formatter->add_flag<Everest::Logging::EverestFuncnameFormatter>('!').set_pattern(format);

        auto destination = sink["Destination"].get<std::string>().get_value_or("");
        if (destination == "Console") {
            formatter->add_flag<Everest::Logging::EverestColorLevelFormatter>('l').set_pattern(format);

            severity_strings_colors[severity_level::verbose] =
                sink["SeverityStringColorTrace"].get<std::string>().get_value_or("");
            severity_strings_colors[severity_level::debug] =
                sink["SeverityStringColorDebug"].get<std::string>().get_value_or("");
            severity_strings_colors[severity_level::info] =
                sink["SeverityStringColorInfo"].get<std::string>().get_value_or("");
            severity_strings_colors[severity_level::warning] =
                sink["SeverityStringColorWarning"].get<std::string>().get_value_or("");
            severity_strings_colors[severity_level::error] =
                sink["SeverityStringColorError"].get<std::string>().get_value_or("");
            severity_strings_colors[severity_level::critical] =
                sink["SeverityStringColorCritical"].get<std::string>().get_value_or("");
            auto console_sink = std::make_shared<ConsoleFilterSink>(parsed_filter);
            console_sink->set_formatter(std::move(formatter));
            sinks.push_back(console_sink);
        } else if (destination == "TextFile") {
            formatter->add_flag<Everest::Logging::EverestLevelFormatter>('l').set_pattern(format);

            auto file_name = sink["FileName"].get<std::string>().get_value_or("%5N.log");
            auto rotation_size = sink["RotationSize"].get<std::size_t>().get_value_or(0);
            auto max_files = sink["MaxFiles"].get<std::size_t>().get_value_or(0);

            auto file_sink =
                std::make_shared<TextFileFilterSink>(parsed_filter, file_name, rotation_size, max_files, false);
            file_sink->set_formatter(std::move(formatter));
            sinks.push_back(file_sink);
        } else if (destination == "Syslog") {
            // TODO implement LocalAddress and TargetAddress settings
            auto enable_formatting = sink["EnableFormatting"].get<bool>().get_value_or(false);

            formatter->add_flag<Everest::Logging::EverestLevelFormatter>('l').set_pattern(format);
            auto syslog_sink =
                std::make_shared<SyslogFilterSink>(parsed_filter, process_name, 0, LOG_USER, enable_formatting);
            syslog_sink->set_formatter(std::move(formatter));
            sinks.push_back(syslog_sink);
        }
    }

    update_process_name(process_name);

    EVLOG_debug << "Logger initialized (using " << logconf << ")...";
}

void update_process_name(std::string process_name) {
    if (!process_name.empty()) {
        current_process_name = process_name;
        auto filter_logger = std::make_shared<spdlog::logger>(current_process_name, std::begin(sinks), std::end(sinks));
        spdlog::set_default_logger(filter_logger);
        spdlog::set_level(global_level);
    }
}
} // namespace Logging
} // namespace Everest
