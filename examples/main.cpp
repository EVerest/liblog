// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#include <everest/exceptions.hpp>
#include <everest/logging.hpp>

#include <iostream>
#include <thread>

#include <boost/exception/diagnostic_information.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;

struct TestStruct {
    std::string hello;
    int integer;
};

std::ostream& operator<<(std::ostream& os, const TestStruct& test) {
    os << "TestStruct: hello " << test.hello << " " << test.integer;
    return os;
}

int main(int argc, char* argv[]) {
    po::options_description desc("EVerest::log example");
    desc.add_options()("help,h", "produce help message");
    desc.add_options()("logconf", po::value<std::string>(), "The path to a custom logging.ini");

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
    Everest::Logging::init(logging_config, "hello there. EVerest");

    EVLOG_debug << "logging_config was set to " << logging_config;

    EVLOG_verbose << "This is a VERBOSE message.";
    EVLOG_debug << "This is a DEBUG message.";
    EVLOG_info << "This is a INFO message.";
    EVLOG_warning << "This is a WARNING message.";
    EVLOG_error << "This is a ERROR message.";
    EVLOG_critical << "This is a CRITICAL message.";

    TestStruct test_struct{"there", 42};
    EVLOG_info << "This logs a TestStruct using a operator<<: " << test_struct;
    EVLOG_info << "Test logs with an additional std::endl at the end" << std::endl;
    EVLOG_info << "Test logs with different types: " << 42 << " " << 12.34;

    EVLOG_critical << Everest::Logging::LogSource("file.name", 42, "function_with_file_name_and_line_nr()")
                   << "This is a CRITICAL message";
    EVLOG_critical << Everest::Logging::LogSource("function_without_file_name_or_line_nr()")
                   << "This is a CRITICAL message";

    auto t = std::thread([]() { EVLOG_info << "From another thread"; });
    t.join();

    try {
        EVTHROW(std::runtime_error("hello there"));

    } catch (...) {
    }
    try {
        EVLOG_AND_THROW(std::runtime_error("hello there"));

    } catch (...) {
    }
    try {
        EVTHROW(EVEXCEPTION(Everest::EverestInternalError, "Something", " with", " multiple", " args"));

    } catch (...) {
    }
    return 0;
}
