#pragma once


#include <mutex>
#include <queue>
#include <memory>
#include <unordered_map>
#include <module/base_module.hpp>
#include <message/message.hpp>
#include <timer/timer.hpp>
#include <logger/logger.hpp>
#include <timer/timer_message.hpp>
#include <logger/logger.hpp>
#include <module/session.hpp>
#include <timer/timer_func.h>
#include <module/module_func.h>
#include <bus/message_bus.hpp>
#include <module/multi_priority_queue.hpp>


#define MAX_MSG_COUNT                   500000
#define MAX_TRIGGER_TIMES               0xFFFFFFFFFFFFFFFF


#define INIT_TIMER(TIMER_ID, TIMER_NAME, PERIOD, REPEAT_TIMES, SESSION_ID, FUNC_PTR) \
    TIMER_ID = this->add_timer(TIMER_NAME, PERIOD, REPEAT_TIMES, SESSION_ID); \
    this->m_timer_invokers[TIMER_NAME] = std::bind(FUNC_PTR, this, std::placeholders::_1);


#define INIT_INVOKER(MSG_NAME, FUNC_PTR) \
    MSG_BUS_SUB(MSG_NAME, [this](std::shared_ptr<message> &msg) { return send(msg);  }); \
    m_msg_invokers[MSG_NAME] = std::bind(FUNC_PTR, this, std::placeholders::_1);


namespace micro
{
    namespace core
    {

        class timer_processor
        {
        public:

            typedef std::shared_ptr<timer> timer_ptr_type;
            typedef std::shared_ptr<message> msg_ptr_type;

            timer_processor(void * mdl) : m_mdl(mdl), m_timer_idx(0) {}

            virtual ~timer_processor() { clear(); }

            uint64_t add_timer(const std::string &name, uint64_t period, uint64_t trigger_times, const std::string & session_id)
            {
                if (trigger_times < 1 || period < DEFAULT_MILLISECONDS_ONE_TICK)
                {
                    return INVALID_TIMER_ID;
                }

                if (MAX_TIMER_ID == m_timer_idx)
                {
                    LOG_ERROR << "timer id allocated error: " << MAX_TIMER_ID;
                    return INVALID_TIMER_ID;
                }

                timer_ptr_type timer = std::make_shared<micro::core::timer>(name, period, trigger_times, session_id);
                timer->set_timer_id(++m_timer_idx);
                m_timers.push_back(timer);

                return timer->get_timer_id();
            }

            void remove_timer(uint64_t timer_id)
            {
                for (auto it = m_timers.begin(); it != m_timers.end(); it++)
                {
                    if (timer_id == (*it)->get_timer_id())
                    {
                        m_timers.erase(it);
                        return;
                    }
                }
            }

            int32_t process(uint64_t time_tick)
            {
                timer_ptr_type timer;
                std::list<uint64_t> remove_timers;

                for (auto it = m_timers.begin(); it != m_timers.end(); it++)
                {
                    timer = *it;

                    if (timer->get_time_out_tick() <= time_tick)
                    {
                        //callback timer function
                        timer_func(m_mdl, timer);

                        timer->minus_trigger_times();
                        if (0 == timer->get_trigger_times())
                        {
                            remove_timers.push_back(timer->get_timer_id());
                        }
                        else
                        {
                            timer->cal_time_out_tick();
                        }
                    }
                }

                for (auto it : remove_timers)
                {
                    remove_timer(it);
                }

                remove_timers.clear();

                return ERR_SUCCESS;
            }

            void clear() { m_timers.clear(); }

        protected:

            void * m_mdl;

            std::list<timer_ptr_type>  m_timers;

            uint64_t m_timer_idx;

        };

        class module : public base_module
        {
        public:

            typedef std::mutex mutex_type;
            typedef std::condition_variable cv_type;

            typedef std::function<void(void *)> functor_type;
            typedef std::shared_ptr<std::thread> thr_ptr_type;
            typedef std::shared_ptr<message> msg_ptr_type;
            typedef std::shared_ptr<timer> timer_ptr_type;
            typedef std::shared_ptr<multi_priority_queue<msg_ptr_type>> queue_type;

            typedef std::function<int32_t(msg_ptr_type)> msg_functor_type;
            typedef std::function<int32_t(timer_ptr_type)> timer_functor_type;

            typedef std::shared_ptr<timer_processor> timer_processor_type;
            typedef std::shared_ptr<session> session_ptr_type;

            typedef std::unordered_map<std::string, msg_functor_type> msg_functors_type;
            typedef std::unordered_map<std::string, timer_functor_type> timer_functors_type;
            typedef std::unordered_map<std::string, session_ptr_type> sessions_type;


            module()
                : m_exited(false)
                , m_functor(task_func)
                , m_timer_processor(std::make_shared<timer_processor>(this))
                , m_send_queue(std::make_shared<multi_priority_queue<msg_ptr_type>>())
                , m_worker_queue(std::make_shared<multi_priority_queue<msg_ptr_type>>())
            {}

            virtual ~module() = default;

            virtual int32_t init(any_map &vars)
            {
                init_timer();
                init_invoker();
                init_time_tick_subscription();

                return service_init(vars);
            }

            int32_t start()
            {
                m_thr = std::make_shared<std::thread>(m_functor, this);
                return ERR_SUCCESS;
            }

            int32_t stop()
            {
                m_exited = true;

                if (m_thr)
                {
                    try
                    {
                        m_thr->join();
                    }
                    catch (...)
                    {
                        return ERR_FAILED;
                    }
                }

                return ERR_SUCCESS;
            }

            int32_t exit() { return service_exit(); }

            int32_t run()
            {
                msg_ptr_type msg;

                while (!m_exited)
                {

                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        std::chrono::milliseconds ms(100);
                        m_cv.wait_for(lock, ms, [this]()->bool {return !(this->is_empty());});

                        if (is_empty()) continue;

                        //swap queue
                        auto tmp_queue = m_send_queue;
                        assert(m_worker_queue->empty());
                        m_send_queue = m_worker_queue;
                        m_worker_queue = tmp_queue;
                    }

                    while (!m_worker_queue->empty())
                    {
                        msg = m_worker_queue->front();
                        on_invoke(msg);
                        m_worker_queue->pop();
                    }

                }

                return ERR_SUCCESS;
            }

            int32_t on_time_out(timer_ptr_type timer)
            {
                auto it = m_timer_invokers.find(timer->get_name());
                if (it == m_timer_invokers.end())
                {
                    LOG_ERROR << this->name() << " received unknown timer: " << timer->get_name();
                    return ERR_FAILED;
                }

                return (it->second)(timer);
            }

            int32_t send(std::shared_ptr<message> msg)
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                if (m_send_queue->size() >= MAX_MSG_COUNT)
                {
                    LOG_ERROR << "module message queue full: " << this->name() << " send msg: " << msg->get_name();
                    return ERR_FAILED;
                }

                m_send_queue->push(msg, msg->m_header->m_priority);

                if (!m_send_queue->empty())
                {
                    m_cv.notify_all();
                }

                return ERR_SUCCESS;
            }

            bool is_empty() const { return m_send_queue->empty(); }

        protected:

            int32_t on_invoke(msg_ptr_type msg)
            {
                if (msg->get_name() == BROADCAST_TIMER_TICK)
                {
                    return on_timer_invoke(msg);
                }
                else
                {
                    return on_msg_invoke(msg);
                }
            }

            int32_t on_msg_invoke(msg_ptr_type msg)
            {
                auto it = m_msg_invokers.find(msg->get_name());
                if (it == m_msg_invokers.end())
                {
                    LOG_ERROR << this->name() << " unknown message: " << msg->get_name();
                    return ERR_FAILED;
                }

                return (it->second)(msg);
            }

            int32_t on_timer_invoke(msg_ptr_type msg)
            {
                auto msg_body = DYN_CAST(broadcast_timer_tick, msg->m_body);
                assert(nullptr != msg_body);
                return m_timer_processor->process(msg_body->time_tick);
            }            

        protected:

            virtual int32_t service_init(any_map &vars) { return ERR_SUCCESS; }

            virtual int32_t service_exit() { return ERR_SUCCESS; }

            virtual void init_invoker() {}

            virtual void init_timer() {}

            virtual void init_time_tick_subscription()
            {
                MSG_BUS_SUB(BROADCAST_TIMER_TICK, [this](std::shared_ptr<message> &msg) {return this->send(msg); });
            }

        protected:

            uint64_t add_timer(const std::string & name, uint64_t period, uint64_t repeat_times, const std::string & session_id)
            {
                return m_timer_processor->add_timer(name, period, repeat_times, session_id);
            }

            void remove_timer(uint64_t timer_id) { m_timer_processor->remove_timer(timer_id); }

            int32_t add_session(std::string session_id, std::shared_ptr<session> session)
            {
                if (m_sessions.find(session_id) != m_sessions.end())
                {
                    return ERR_FAILED;
                }

                m_sessions.insert({ session_id, session });
                return ERR_SUCCESS;
            }

            std::shared_ptr<session> get_session(std::string session_id)
            {
                auto it = m_sessions.find(session_id);
                if (it == m_sessions.end())
                {
                    return nullptr;
                }

                return it->second;
            }

            void remove_session(const std::string & session_id) { m_sessions.erase(session_id); }

        protected:

            bool m_exited;

            mutex_type m_mutex;

            cv_type m_cv;

            thr_ptr_type m_thr;

            queue_type m_send_queue;

            queue_type m_worker_queue;

            functor_type m_functor;

            timer_processor_type m_timer_processor;

            msg_functors_type m_msg_invokers;

            timer_functors_type m_timer_invokers;

            sessions_type m_sessions;

        };

    }

}
