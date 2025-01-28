// Copyright(c) 2015-present, Gabi Melman & spdlog contributors.
// Distributed under the MIT License (http://opensource.org/licenses/MIT)
// Based on
// https://github.com/gabime/spdlog/blob/5ebfc927306fd7ce551fa22244be801cf2b9fdd9/include/spdlog/sinks/rotating_file_sink.h

#pragma once

#include <fmt/printf.h>
#include <spdlog/details/file_helper.h>
#include <spdlog/details/null_mutex.h>
#include <spdlog/details/synchronous_factory.h>
#include <spdlog/sinks/base_sink.h>

#include <chrono>
#include <mutex>
#include <string>

namespace Everest {
namespace Logging {

/// \brief Rotating file sink based on size
template <typename Mutex> class rotating_file_sink : public spdlog::sinks::base_sink<Mutex> {
public:
    rotating_file_sink(spdlog::filename_t base_filename, std::size_t max_size, std::size_t max_files,
                       bool rotate_on_open) :
        base_filename_(std::move(base_filename)), max_size_(max_size), max_files_(max_files) {
        if (max_files_ > 0) {
            max_files_ = max_files_ - 1;
        }
        file_helper_.open(calc_filename(base_filename_, 0));
        current_size_ = file_helper_.size(); // expensive. called only once
        if (max_size != 0 and rotate_on_open && current_size_ > 0) {
            rotate_();
            current_size_ = 0;
        }
    }

    static spdlog::filename_t calc_filename(const spdlog::filename_t& filename, std::size_t index) {
        std::string format_str = filename;
        std::string log_file_name = filename;
        try {
            // find any %5N like constructs in the filename
            size_t pos_percent = 0;
            while ((pos_percent = format_str.find('%', pos_percent)) != std::string::npos) {
                size_t pos_n = pos_percent;
                pos_n = format_str.find('N', pos_n);
                if (pos_n != std::string::npos) {
                    format_str = format_str.replace(pos_n, 1, "d");
                    format_str = format_str.replace(pos_percent, 1, "%0");
                } else {
                    break;
                }
            }
            log_file_name = fmt::sprintf(format_str, index);
        } catch (const fmt::format_error& e) {
        }

        if (log_file_name == filename) {
            spdlog::filename_t basename, ext;
            std::tie(basename, ext) = spdlog::details::file_helper::split_by_extension(filename);
            log_file_name = fmt::format("{}{:05}{}", basename, index, ext);
        }

        return log_file_name;
    }
    spdlog::filename_t filename() {
        std::lock_guard<Mutex> lock(spdlog::sinks::base_sink<Mutex>::mutex_);
        return file_helper_.filename();
    }

protected:
    void sink_it_(const spdlog::details::log_msg& msg) override {
        spdlog::memory_buf_t formatted;
        spdlog::sinks::base_sink<Mutex>::formatter_->format(msg, formatted);
        auto new_size = current_size_ + formatted.size();

        // rotate if the new estimated file size exceeds max size.
        // rotate only if the real size > 0 to better deal with full disk (see issue #2261).
        // we only check the real size when new_size > max_size_ because it is relatively expensive.
        if (max_size_ != 0 and new_size > max_size_) {
            file_helper_.flush();
            if (file_helper_.size() > 0) {
                rotate_();
                new_size = formatted.size();
            }
        }
        file_helper_.write(formatted);
        current_size_ = new_size;
    }
    void flush_() override {
        file_helper_.flush();
    }

private:
    // Rotate files:
    // log.txt -> log.1.txt
    // log.1.txt -> log.2.txt
    // log.2.txt -> log.3.txt
    // log.3.txt -> delete
    void rotate_() {
        using spdlog::details::os::filename_to_str;
        using spdlog::details::os::path_exists;

        file_helper_.close();
        for (auto i = max_files_; i > 0; --i) {
            spdlog::filename_t src = calc_filename(base_filename_, i - 1);
            if (!path_exists(src)) {
                continue;
            }
            spdlog::filename_t target = calc_filename(base_filename_, i);

            if (!rename_file_(src, target)) {
                // if failed try again after a small delay.
                // this is a workaround to a windows issue, where very high rotation
                // rates can cause the rename to fail with permission denied (because of antivirus?).
                spdlog::details::os::sleep_for_millis(100);
                if (!rename_file_(src, target)) {
                    file_helper_.reopen(true); // truncate the log file anyway to prevent it to grow beyond its limit!
                    current_size_ = 0;
                    spdlog::throw_spdlog_ex("rotating_file_sink: failed renaming " + filename_to_str(src) + " to " +
                                                filename_to_str(target),
                                            errno);
                }
            }
        }
        file_helper_.reopen(true);
    }

    // delete the target if exists, and rename the src file  to target
    // return true on success, false otherwise.
    bool rename_file_(const spdlog::filename_t& src_filename, const spdlog::filename_t& target_filename) {
        // try to delete the target file in case it already exists.
        (void)spdlog::details::os::remove(target_filename);
        return spdlog::details::os::rename(src_filename, target_filename) == 0;
    }
    spdlog::filename_t base_filename_;
    std::size_t max_size_;
    std::size_t max_files_;
    std::size_t current_size_;
    spdlog::details::file_helper file_helper_;
};

using rotating_file_sink_mt = rotating_file_sink<std::mutex>;

} // namespace Logging
} // namespace Everest
