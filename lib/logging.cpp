// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
#include <boost/date_time/posix_time/time_formatters_limited.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/attributes/current_process_id.hpp>
#include <boost/log/attributes/current_process_name.hpp>
#include <boost/log/attributes/current_thread_id.hpp>
#include <boost/log/sources/record_ostream.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/utility/setup/filter_parser.hpp>
#include <boost/log/utility/setup/formatter_parser.hpp>
#include <boost/log/utility/setup/from_settings.hpp>
#include <boost/log/utility/setup/from_stream.hpp>
#include <boost/log/utility/setup/settings.hpp>
#include <boost/log/utility/setup/settings_parser.hpp>
#include <fstream>

#include <everest/exceptions.hpp>
#include <everest/logging.hpp>

// this will only be used while bootstrapping our logging (e.g. the logging settings aren't yet applied)
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define EVEREST_INTERNAL_LOG_AND_THROW(exception)                                                                      \
    do {                                                                                                               \
        BOOST_LOG_TRIVIAL(fatal) << (exception).what();                                                                \
        throw(exception);                                                                                              \
    } while (0);

namespace fs = boost::filesystem;
namespace logging = boost::log::BOOST_LOG_VERSION_NAMESPACE;
namespace attrs = logging::attributes;

namespace Everest {
namespace Logging {
std::array<std::string, 6> severity_strings = {
    "VERB", //
    "DEBG", //
    "INFO", //
    "WARN", //
    "ERRO", //
    "CRIT", //
};

std::array<std::string, 6> severity_strings_colors = {
    "", //
    "", //
    "", //
    "", //
    "", //
    "", //
};

std::string process_name_padding(const std::string& process_name) {
    const unsigned int process_name_padding_length = 15;
    std::string padded_process_name = process_name;
    if (process_name_padding_length > padded_process_name.size())
        padded_process_name.insert(padded_process_name.size(), process_name_padding_length, ' ');
    if (padded_process_name.size() > process_name_padding_length)
        padded_process_name = padded_process_name.substr(0, process_name_padding_length);
    return padded_process_name;
}

attrs::mutable_constant<std::string> current_process_name(process_name_padding(logging::aux::get_process_name()));

// The operator puts a human-friendly representation of the severity level to the stream
std::ostream& operator<<(std::ostream& strm, severity_level level) {
    if (static_cast<std::size_t>(level) < severity_strings.size()) {
        strm << severity_strings_colors.at(level) << severity_strings.at(level) << "\033[0m";
    } else {
        strm << static_cast<int>(level);
    }

    return strm;
}

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

class CallbackSink : public logging::sinks::sink {
private:
    std::function<void(const LogRecord& record)> log_callback;

public:
    CallbackSink(bool cross_thread, const std::function<void(const LogRecord& record)>& log_callback) :
        sink(cross_thread), log_callback(log_callback) {
    }

    bool will_consume(logging::attribute_value_set const& attributes) {
        return true;
    }

    void consume(logging::record_view const& rec) {
        LogRecord log_record;

        log_record.message = *rec[logging::expressions::smessage];

        if (logging::value_ref<severity_level> severity = rec["Severity"].extract<severity_level>()) {
            log_record.severity = *severity;
        }
        if (logging::value_ref<boost::posix_time::ptime> timestamp =
                rec["TimeStamp"].extract<boost::posix_time::ptime>()) {
            log_record.timestamp = boost::posix_time::to_iso_extended_string(*timestamp);
            // TODO: conversion to chrono type?
        }
        if (logging::value_ref<std::string> process = rec["Process"].extract<std::string>()) {
            log_record.process = *process;
        }
        if (logging::value_ref<std::string> function = rec["function"].extract<std::string>()) {
            log_record.function = *function;

        }

        log_callback(log_record);
    }

    void flush() {
    }
};

void init(const std::string& logconf) {
    init(logconf, "");
}

void init(const std::string& logconf,
          const std::function<void(const LogRecord& record)>& log_callback) {
    init(logconf, "", log_callback);
}

void init(const std::string& logconf, std::string process_name) {
    init(logconf, process_name, nullptr);
}

void init(const std::string& logconf, std::string process_name,
          const std::function<void(const LogRecord& record)>& log_callback) {
    BOOST_LOG_FUNCTION();

    // add useful attributes
    logging::add_common_attributes();

    std::string padded_process_name;

    if (!process_name.empty()) {
        padded_process_name = process_name_padding(process_name);
    }

    logging::core::get()->add_global_attribute("Process", current_process_name);
    if (!padded_process_name.empty()) {
        current_process_name.set(padded_process_name);
    }
    logging::core::get()->add_global_attribute("Scope", attrs::named_scope());

    // register callback sink if a log callback was provided
    if (log_callback != nullptr) {
        auto callback_sink = boost::make_shared<CallbackSink>(true, log_callback);
        logging::core::get()->add_sink(callback_sink);
    }

    // Before initializing the library from settings, we need to register any custom filter and formatter factories
    logging::register_simple_filter_factory<severity_level>("Severity");
    logging::register_simple_formatter_factory<severity_level, char>("Severity");

    // open logging.ini config file located at our base_dir and use it to configure boost::log logging (filters and
    // format)
    fs::path logging_path = fs::path(logconf);
    std::ifstream logging_config(logging_path.c_str());
    if (!logging_config.is_open()) {
        EVEREST_INTERNAL_LOG_AND_THROW(EverestConfigError(std::string("Could not open logging config file at ") +
                                                          std::string(fs::absolute(logging_path).c_str())));
    }

    auto settings = logging::parse_settings(logging_config);

    auto sink = settings["Sinks.Console"].get_section();

    severity_strings_colors[severity_level::verbose] =
        sink["SeverityStringColorTrace"].get<std::string>().get_value_or("");
    severity_strings_colors[severity_level::debug] =
        sink["SeverityStringColorDebug"].get<std::string>().get_value_or("");
    severity_strings_colors[severity_level::info] = sink["SeverityStringColorInfo"].get<std::string>().get_value_or("");
    severity_strings_colors[severity_level::warning] =
        sink["SeverityStringColorWarning"].get<std::string>().get_value_or("");
    severity_strings_colors[severity_level::error] =
        sink["SeverityStringColorError"].get<std::string>().get_value_or("");
    severity_strings_colors[severity_level::critical] =
        sink["SeverityStringColorCritical"].get<std::string>().get_value_or("");

    logging::init_from_settings(settings);

    EVLOG_debug << "Logger initialized (using " << logconf << ")...";
}

void update_process_name(std::string process_name) {
    if (!process_name.empty()) {
        std::string padded_process_name;

        padded_process_name = process_name_padding(process_name);
        current_process_name.set(padded_process_name);
    }
}
} // namespace Logging
} // namespace Everest
