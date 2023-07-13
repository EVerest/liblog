// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2022 Pionix GmbH and Contributors to EVerest
#ifndef LOGGING_HPP
#define LOGGING_HPP

#include <boost/current_function.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/attributes/attribute_set.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/exception.hpp>
#include <boost/log/utility/manipulators/add_value.hpp>
#include <boost/throw_exception.hpp>
#include <exception>
#include <optional>
#include <string>

namespace logging = boost::log::BOOST_LOG_VERSION_NAMESPACE;
namespace attrs = logging::attributes;

namespace Everest {
namespace Logging {

enum severity_level {
    verbose,
    debug,
    info,
    warning,
    error,
    critical,
};

struct LogRecord {
    std::string message;
    std::optional<severity_level> severity;
    std::optional<std::string> timestamp;
    std::optional<std::string> process;
    std::optional<std::string> function;
    std::optional<int> line;
    std::optional<std::string> file;
};

void init(const std::string& logconf);
void init(const std::string& logconf, const std::function<void(const LogRecord& record)>& log_callback);
void init(const std::string& logconf, std::string process_name);
void init(const std::string& logconf, std::string process_name,
          const std::function<void(const LogRecord& record)>& log_callback);
void update_process_name(std::string process_name);
std::string trace();
} // namespace Logging

// clang-format off
#define LOG_METADATA                                                                                                   \
  logging::attribute_cast<attrs::mutable_constant<std::string>>(                                                       \
    logging::core::get()->get_global_attributes()["Function"]).set(BOOST_CURRENT_FUNCTION);                            \
  logging::attribute_cast<attrs::mutable_constant<int>>(                                                               \
    logging::core::get()->get_global_attributes()["Line"]).set(__LINE__);                                              \
  logging::attribute_cast<attrs::mutable_constant<std::string>>(                                                       \
    logging::core::get()->get_global_attributes()["File"]).set(__FILE__);

#define EVLOG_verbose                                                                                                  \
    LOG_METADATA;                                                                                                      \
    BOOST_LOG_SEV(::global_logger::get(), ::Everest::Logging::verbose)                                                 \
        << logging::add_value("file", __FILE__)                                                                        \
        << logging::add_value("line", __LINE__)                                                                        \
        << logging::add_value("function", BOOST_CURRENT_FUNCTION)

#define EVLOG_debug                                                                                                    \
    LOG_METADATA;                                                                                                      \
    BOOST_LOG_SEV(::global_logger::get(), ::Everest::Logging::debug)                                                   \
        << logging::add_value("function", BOOST_CURRENT_FUNCTION)

#define EVLOG_info                                                                                                     \
    LOG_METADATA;                                                                                                      \
    BOOST_LOG_SEV(::global_logger::get(), ::Everest::Logging::info)

#define EVLOG_warning                                                                                                  \
    LOG_METADATA;                                                                                                      \
    BOOST_LOG_SEV(::global_logger::get(), ::Everest::Logging::warning)                                                 \
        << logging::add_value("function", BOOST_CURRENT_FUNCTION)

#define EVLOG_error                                                                                                    \
    LOG_METADATA;                                                                                                      \
    BOOST_LOG_SEV(::global_logger::get(), ::Everest::Logging::error)                                                   \
        << logging::add_value("function", BOOST_CURRENT_FUNCTION)

#define EVLOG_critical                                                                                                 \
    LOG_METADATA;                                                                                                      \
    BOOST_LOG_SEV(::global_logger::get(), ::Everest::Logging::critical)                                                \
        << logging::add_value("function", BOOST_CURRENT_FUNCTION)
// clang-format on

#define EVLOG_AND_THROW(ex)                                                                                            \
    do {                                                                                                               \
        try {                                                                                                          \
            BOOST_THROW_EXCEPTION(boost::enable_error_info(ex)                                                         \
                                  << logging::current_scope());                                                        \
        } catch (std::exception & e) {                                                                                 \
            EVLOG_error << e.what();                                                                                   \
            throw;                                                                                                     \
        }                                                                                                              \
    } while (0)

#define EVTHROW(ex)                                                                                                    \
    do {                                                                                                               \
        BOOST_THROW_EXCEPTION(boost::enable_error_info(ex)                                                             \
                              << logging::current_scope());                                                            \
    } while (0)
} // namespace Everest

BOOST_LOG_INLINE_GLOBAL_LOGGER_DEFAULT(
    global_logger,
    logging::sources::severity_logger_mt<Everest::Logging::severity_level>)

#endif // LOGGING_HPP
