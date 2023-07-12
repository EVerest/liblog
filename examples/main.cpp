// SPDX-License-Identifier: Apache-2.0
// Copyright 2020 - 2021 Pionix GmbH and Contributors to EVerest
#include <everest/logging.hpp>

#include <iostream>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

int main(int argc, char* argv[]) {
    po::options_description desc("EVerest::log example");
    desc.add_options()("help,h", "produce help message");
    desc.add_options()("logconf", po::value<std::string>(), "The path to a custom logging.ini");
    desc.add_options()("callback,c", "Use example log callback");

    po::variables_map vm;
    po::store(po::parse_command_line(argc, argv, desc), vm);
    po::notify(vm);

    if (vm.count("help") != 0) {
        std::cout << desc << "\n";
        return 1;
    }

    // initialize logging as early as possible
    std::string logging_config = "logging.ini";
    if (vm.count("logconf") != 0) {
        logging_config = vm["logconf"].as<std::string>();
    }

    if (vm.count("callback") != 0) {
        std::cout << "Using log callback" << std::endl;
        Everest::Logging::init(logging_config, "hello there", [](const Everest::Logging::LogRecord& record) {
            std::cout << "Message: " << record.message << std::endl;
            if (record.severity.has_value()) {
                std::cout << "Severity: " << *record.severity << std::endl;
            }
            if (record.timestamp.has_value()) {
                std::cout << "TimeStamp: " << *record.timestamp << std::endl;
            }
            if (record.process.has_value()) {
                std::cout << "Process: " << *record.process << std::endl;
            }
            if (record.function.has_value()) {
                std::cout << "Function: " << *record.function << std::endl;
            }
        });
    } else {
        Everest::Logging::init(logging_config, "hello there");
    }

    EVLOG_debug << "logging_config was set to " << logging_config;

    EVLOG_verbose << "This is a VERBOSE message.";
    EVLOG_debug << "This is a DEBUG message.";
    EVLOG_info << "This is a INFO message.";
    EVLOG_warning << "This is a WARNING message.";
    EVLOG_error << "This is a ERROR message.";
    EVLOG_critical << "This is a CRITICAL message.";

    return 0;
}
