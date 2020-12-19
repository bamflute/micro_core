#pragma once


#include <mutex>
#include <queue>
#include <memory>
#include <unordered_map>
#include <algorithm>
#include <module/base_module.hpp>
#include <message/message.hpp>
#include <message/message.hpp>
#include <logger/logger.hpp>
#include <common/common.hpp>
#include <common/core_macro.h>


#define WORKER_THREAD_COUNT                             10
#define RANDOM_SEND_MSG_COUNT_DOWN          100
#define MAX_THREAD_MSG_COUNT                        5000000
#define MULTI_THREADS_COUNT                     "multi_threads_count"

extern void worker_task_func(void *arg);

extern thread_local uint32_t thread_local_idx;

namespace micro
{
    namespace core
    {

        class worker_thread
        {
        public:

            typedef std::mutex mutex_type;
            typedef std::condition_variable cv_type;

            typedef std::function<void(void *)> functor_type;
            typedef std::shared_ptr<std::thread> thr_ptr_type;

            typedef std::shared_ptr<message> msg_ptr_type;
            typedef std::queue<std::shared_ptr<message>> queue_type;
            typedef std::function<int32_t(msg_ptr_type)> msg_functor_type;
            typedef std::unordered_map<std::string, msg_functor_type> msg_functors_type;


            worker_thread(uint32_t thread_idx)
                : m_thread_idx(thread_idx)
                , m_exited(false)
                , m_functor(worker_task_func)
            {}

            ~worker_thread() = default;

            virtual int32_t init(any_map &vars)
            {
                return ERR_SUCCESS; 
            }

            virtual int32_t exit() 
            {
                return ERR_SUCCESS; 
            }

            virtual int32_t start() 
            { 
                m_thr = std::make_shared<std::thread>(m_functor, this);
                return ERR_SUCCESS;
            }

            virtual int32_t stop() 
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

            size_t size() { return m_queue.size(); }

            int32_t send(std::shared_ptr<message> msg)
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                if (m_queue.size() >= MAX_THREAD_MSG_COUNT)
                {
                    BEGIN_COUNT_TO_DO(MSG_QUEUE, 100000)
                    LOG_WARNING << "worker thread message overloaded: " << msg->get_name() << " size: " << std::to_string(m_queue.size());
                    END_COUNT_TO_DO
                    //return ERR_FAILED;
                }

                m_queue.push(msg);

                if (!m_queue.empty())
                {
                    m_cv.notify_all();
                }

                return ERR_SUCCESS;
            }

            void register_msg_functor(const std::string & msg_name, msg_functor_type functor)
            {
                m_msg_invokers[msg_name] = functor;
            }

            int32_t run()
            {
                thread_local_idx = m_thread_idx;

                LOG_INFO << "worker thread begins to run idx: " << thread_local_idx;

                msg_ptr_type msg;
                queue_type msg_queue;

                while (!m_exited)
                {

                    {
                        std::unique_lock<std::mutex> lock(m_mutex);
                        std::chrono::milliseconds ms(100);
                        m_cv.wait_for(lock, ms, [this]()->bool {return !(this->is_empty()); });

                        if (is_empty()) continue;
                        m_queue.swap(msg_queue);
                    }
                    
                    BEGIN_COUNT_TO_DO(MSG_QUEUE, 100000)
                        LOG_INFO << "worker thread msg queue size: " << std::to_string(msg_queue.size()) << " idx: " << std::to_string(m_thread_idx);
                    END_COUNT_TO_DO

                    while (!msg_queue.empty())
                    {
                        msg = msg_queue.front();

                        try
                        {
                            on_invoke(msg);
                        }
                        catch (...)
                        {
                            LOG_ERROR << "!!!!!! multi thread module on invoke exception: " << msg->get_name();
                        }

                        msg_queue.pop();
                    }

                }

                return ERR_SUCCESS;
            }

        protected:

            bool is_empty() const { return m_queue.empty(); }

            int32_t on_invoke(msg_ptr_type msg)
            {
                return on_msg_invoke(msg);
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

            virtual std::string name() const { return "thread woker"; };

        protected:

            uint32_t m_thread_idx;

            bool m_exited;

            mutex_type m_mutex;

            cv_type m_cv;

            thr_ptr_type m_thr;

            queue_type m_queue;

            functor_type m_functor;

            msg_functors_type m_msg_invokers;

        };

        class multi_thread_module
        {
        public:

            //typedef std::mutex mutex_type;

            multi_thread_module() : m_count_down(RANDOM_SEND_MSG_COUNT_DOWN), m_cur_thread_idx(0), m_round_robin_thread_idx(0), m_threads_count(1) {}

            virtual ~multi_thread_module() {}

            virtual std::string name() const { return "multi thread module"; };

            virtual int32_t init(any_map &vars)
            { 
                m_threads_count = boost::any_cast<uint32_t>(vars.get(MULTI_THREADS_COUNT));
                m_threads_count = std::max(m_threads_count, (uint32_t)1);

                m_workers = new std::shared_ptr<worker_thread>[m_threads_count];

                for (uint32_t i = 0; i < m_threads_count; i++)
                {
                    std::shared_ptr<worker_thread> worker = std::make_shared<worker_thread>(i);
                    worker->init(vars);

                    m_workers[i] = worker;
                }

                return service_init(vars);
            }

            template<class WORKER_THREAD_MODULE>
            int32_t init_with_template_class(any_map &vars)
            {
                m_threads_count = boost::any_cast<uint32_t>(vars.get(MULTI_THREADS_COUNT));
                m_threads_count = std::max(m_threads_count, (uint32_t)1);

                m_workers = new std::shared_ptr<worker_thread>[m_threads_count];

                create_worker_thread<WORKER_THREAD_MODULE>(vars);

                return service_init(vars);
            }

            template<class WORKER_THREAD_MODULE>
            int32_t create_worker_thread(any_map &vars)
            {
                for (uint32_t i = 0; i < m_threads_count; i++)
                {
                    std::shared_ptr<worker_thread> worker = std::make_shared<WORKER_THREAD_MODULE>(i);
                    worker->init(vars);

                    m_workers[i] = worker;
                }

                return ERR_SUCCESS;
            }

            size_t size()
            {
                size_t total_size = 0;
                for (uint32_t i = 0; i < m_threads_count; i++)
                {
                    total_size += m_workers[i]->size();
                }

                return total_size;
            }

            virtual int32_t exit() 
            {
                for (uint32_t i = 0; i < m_threads_count; i++)
                {
                    m_workers[i]->exit();
                }

                delete[]m_workers;

                return service_exit();
            }

            virtual int32_t start() 
            {
                for (uint32_t i = 0; i < m_threads_count; i++)
                {
                    m_workers[i]->start();
                }

                return ERR_SUCCESS; 
            }

            virtual int32_t stop()
            {
                for (uint32_t i = 0; i < m_threads_count; i++)
                {
                    m_workers[i]->stop();
                }

                return ERR_SUCCESS;
            }

            virtual int32_t on_timer(std::shared_ptr<base_timer> timer) { return ERR_SUCCESS; }

            virtual int32_t send(std::shared_ptr<message> msg)
            {
                int32_t ret = ERR_SUCCESS;

                //std::unique_lock<std::mutex> lock(m_mutex);
                ret = m_workers[m_cur_thread_idx % m_threads_count]->send(msg);

                if (--m_count_down <= 0)
                {
                    //auto worker = m_workers.front();

                    //m_workers.pop_front();
                    //m_workers.push_back(worker);
                    
                    m_cur_thread_idx++;

                    m_count_down = RANDOM_SEND_MSG_COUNT_DOWN;
                }

                return ret;
            }

            virtual int32_t round_robin_send(std::shared_ptr<message> msg)
            {
                //int32_t ret = ERR_SUCCESS;

                //std::unique_lock<std::mutex> lock(m_mutex);

                //auto worker = m_workers.front();
                //m_workers.pop_front();
                //m_workers.push_back(worker);
                //return ret;

                return m_workers[m_round_robin_thread_idx++  % m_threads_count]->send(msg);
            }

            virtual int32_t broadcast(std::shared_ptr<message> msg)
            {
                for (uint32_t i = 0; i < m_threads_count; i++)
                {
                    m_workers[i]->send(msg);
                }

                return ERR_SUCCESS;
            }

            void register_msg_functor(const std::string & msg_name, worker_thread::msg_functor_type functor)
            {
                for (uint32_t i = 0; i < m_threads_count; i++)
                {
                    m_workers[i]->register_msg_functor(msg_name, functor);
                }
            }

        protected:

            virtual int32_t service_init(any_map &vars) { return ERR_SUCCESS; }

            virtual int32_t service_exit() { return ERR_SUCCESS; }

        protected:

            //mutex_type m_mutex;

            std::atomic<int32_t> m_count_down;

            std::atomic<uint32_t> m_cur_thread_idx;

            std::atomic<uint32_t> m_round_robin_thread_idx;

            uint32_t m_threads_count;

            std::shared_ptr<worker_thread> * m_workers;

        };

    }

}
