#pragma once


#include <unordered_map>
#include <thread/lock.hpp>
#include <boost/any.hpp>
#include <boost/serialization/singleton.hpp>
#include <bus/func_traits.hpp>
#include <logger/logger.hpp>
#include <timer/timer_message.hpp>


#define MSG_BUS boost::serialization::singleton<micro::core::message_bus>::get_mutable_instance()
#define MSG_BUS_SUB MSG_BUS.subscribe
#define MSG_BUS_PUB MSG_BUS.publish


namespace micro
{
    namespace core
    {

        class message_bus
        {
        public:

            typedef boost::any any_type;
            typedef std::unordered_multimap<std::string, any_type> invokers_type;
            typedef std::unordered_multimap<std::string, any_type>::iterator iterator_type;

            message_bus() {}
            virtual ~message_bus() { w_lock_guard lock_guard(m_mutex); m_msg_invokers.clear(); }

        public:

            //subscribe topic from bus
            template<typename function_type>
            void subscribe(const std::string &topic, function_type &&f)
            {
                auto func = to_function(std::forward<function_type>(f));

                std::string msg_type = topic + "|" + typeid(func).name();
                LOG_DEBUG << "message bus subscribe: " << msg_type;

                w_lock_guard lock_guard(m_mutex);
                m_msg_invokers.emplace(std::move(msg_type), std::move(func));
            }

            //unsubscribe
            template<typename ret_type, typename...args_type>
            void unsubscribe(const std::string &topic)
            {
                using function_type = std::function<ret_type(args_type...)>;
                std::string msg_type = topic + "|" + typeid(function_type).name();

                w_lock_guard lock_guard(m_mutex);
                int count = m_msg_invokers.count(msg_type);
                if (0 == count)
                {
                    return;
                }
                auto range = m_msg_invokers.equal_range(msg_type);
                m_msg_invokers.erase(range.first, range.second);
            }

            //publish topic to bus with args function
            template<typename ret_type, typename... args_type>
            void publish(const std::string &topic, args_type&&... args)
            {
                using function_type = std::function<ret_type(args_type...)>;
                std::string msg_type = topic + "|" + typeid(function_type).name();

                if (topic != BROADCAST_TIMER_TICK)
                {
                    //LOG_DEBUG << "message bus publish: " << msg_type;
                }

                r_lock_guard lock_guard(m_mutex);
                auto range = m_msg_invokers.equal_range(msg_type);
                if (range.first == range.second)
                {
                    LOG_ERROR << "could not find topic invoke function: " << topic;
                    return;
                }

                for (iterator_type it = range.first; it != range.second; it++)
                {
                    auto f = boost::any_cast<function_type>(it->second);
                    f(std::forward<args_type>(args)...);
                }
            }

            //publish topic to bus with no args function
            template<typename function_type>
            void publish(const std::string &topic)
            {
                std::string msg_type = topic + "|" + typeid(function_type).name();

                r_lock_guard lock_guard(m_mutex);
                auto range = m_msg_invokers.equal_range(msg_type);
                if (range.first == m_msg_invokers.end())
                {
                    LOG_ERROR << "could not find topic invoke function: " << topic;
                    return;
                }

                for (iterator_type it = range.first; it != range.second; it++)
                {
                    auto f = boost::any_cast<function_type>(it->second);
                    f();
                }
            }

        protected:

            rw_mutex m_mutex;

            invokers_type m_msg_invokers;

        };

    }

}
