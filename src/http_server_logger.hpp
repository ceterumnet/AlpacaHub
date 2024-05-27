#ifndef HTTP_SERVER_LOGGER_HPP
#define HTTP_SERVER_LOGGER_HPP

#include <spdlog/spdlog.h>

class http_server_logger_t {
public:
  http_server_logger_t(std::shared_ptr<spdlog::logger> logger)
      : m_logger{std::move(logger)} {}

  template <typename Builder> void trace(Builder &&msg_builder) {
    log_if_enabled(spdlog::level::trace, std::forward<Builder>(msg_builder));
  }

  template <typename Builder> void info(Builder &&msg_builder) {
    log_if_enabled(spdlog::level::info, std::forward<Builder>(msg_builder));
  }

  template <typename Builder> void warn(Builder &&msg_builder) {
    log_if_enabled(spdlog::level::warn, std::forward<Builder>(msg_builder));
  }

  template <typename Builder> void error(Builder &&msg_builder) {
    log_if_enabled(spdlog::level::err, std::forward<Builder>(msg_builder));
  }

private:
  template <typename Builder>
  void log_if_enabled(spdlog::level::level_enum lv, Builder &&msg_builder) {
    if (m_logger->should_log(lv)) {
      m_logger->log(lv, msg_builder());
    }
  }

  //! Logger object.
  std::shared_ptr<spdlog::logger> m_logger;
};


#endif
