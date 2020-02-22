#pragma once

#include <string>
#include <boost/log/core.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/text_file_backend.hpp>
#include <boost/log/utility/setup/file.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/utility/setup/console.hpp>  
#include <boost/log/utility/exception_handler.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/attributes/mutable_constant.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/filesystem.hpp>
#include <boost/log/utility/setup/console.hpp>



namespace bf = boost::filesystem;

//logger control
extern bool g_enable_trace;
extern bool g_enable_debug;
extern bool g_enable_info;
extern bool g_enable_warning;
extern bool g_enable_error;
extern bool g_enable_fatal;

#define LOG_TRACE               if (g_enable_trace) BOOST_LOG_TRIVIAL(trace)
#define LOG_DEBUG              if (g_enable_debug) BOOST_LOG_TRIVIAL(debug)
#define LOG_INFO                  if (g_enable_info) BOOST_LOG_TRIVIAL(info)
#define LOG_WARNING         if (g_enable_warning) BOOST_LOG_TRIVIAL(warning)
#define LOG_ERROR              if (g_enable_error) BOOST_LOG_TRIVIAL(error)
#define LOG_FATAL               if (g_enable_fatal) BOOST_LOG_TRIVIAL(fatal)


namespace micro
{
    namespace core
    {

        template<typename ValueType>
        static ValueType set_get_attrib(const char* name, ValueType value) {
            auto attr = logging::attribute_cast<attrs::mutable_constant<ValueType>>(logging::core::get()->get_global_attributes()[name]);
            attr.set(value);
            return attr.get();
        }

        struct log_exception_handler
        {
            void operator() (std::runtime_error const& e) const
            {
                std::cout << "std::runtime_error: " << e.what() << std::endl;
            }

            void operator() (std::logic_error const& e) const
            {
                std::cout << "std::logic_error: " << e.what() << std::endl;
                throw;
            }
        };

        class log
        {
        public:

            static int32_t init(bf::path log_path)
            {

                boost::log::register_simple_formatter_factory< boost::log::trivial::severity_level, char >("Severity");

                try
                {
                    auto sink = boost::log::add_file_log
                    (
                        //attribute
                        boost::log::keywords::target = (log_path / "logs"),
                        boost::log::keywords::file_name = (log_path / "logs/%Y%m%d%H%M%S_%N.log").c_str(),
                        boost::log::keywords::max_files = 10,
                        boost::log::keywords::rotation_size = 100 * 1024 * 1024,
                        boost::log::keywords::time_based_rotation = boost::log::sinks::file::rotation_at_time_point(0, 0, 0),
                        boost::log::keywords::format = (
                            boost::log::expressions::stream
                            << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%m-%d %H:%M:%S.%f")
                            << "|" << boost::log::expressions::attr< boost::log::attributes::current_thread_id::value_type>("ThreadID")
                            << "|" << std::setw(7) << std::setfill(' ') << std::left << boost::log::trivial::severity
                            << "|" << boost::log::expressions::smessage
                            )
                    );

                    boost::log::add_console_log(std::cout, boost::log::keywords::format =  (
                            boost::log::expressions::stream
                            << boost::log::expressions::format_date_time<boost::posix_time::ptime>("TimeStamp", "%m-%d %H:%M:%S.%f")
                            << "|" << boost::log::expressions::attr< boost::log::attributes::current_thread_id::value_type>("ThreadID")
                            << "|" << std::setw(7) << std::setfill(' ') << std::left << boost::log::trivial::severity
                            << "|" << boost::log::expressions::smessage
                            ));

                    boost::log::core::get()->set_filter(boost::log::trivial::severity >= boost::log::trivial::trace);
                    sink->locked_backend()->auto_flush(true);

                    boost::log::core::get()->set_exception_handler(
                        boost::log::make_exception_handler<std::runtime_error, std::logic_error>(log_exception_handler())
                    );

                    boost::log::add_common_attributes();
                }
                catch (const std::exception & e)
                {
                    std::cout << "log error" << e.what() << std::endl;
                    return 0;
                }
                catch (const boost::exception & e)
                {
                    std::cout << "log error" << diagnostic_information(e) << std::endl;
                    return 0;
                }
                catch (...)
                {
                    std::cout << "log error" << std::endl;
                    return 0;
                }

                boost::log::core::get()->set_exception_handler(
                    boost::log::make_exception_handler<std::runtime_error, std::logic_error>(log_exception_handler())
                );

                return 0;
            }

            static void set_filter_level(boost::log::trivial::severity_level level)
            {
                switch (level)
                {
                case boost::log::trivial::trace:
                case boost::log::trivial::debug:
                case boost::log::trivial::info:
                case boost::log::trivial::warning:
                case boost::log::trivial::error:
                case boost::log::trivial::fatal:
                {
                    boost::log::core::get()->set_filter(boost::log::trivial::severity >= level);
                    break;
                }
                default:
                    break;
                }
            }

        };

    }

}
