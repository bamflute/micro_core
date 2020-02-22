#pragma once


#include <mutex>
#include <queue>
#include <memory>
#include <unordered_map>
#include <module/base_module.hpp>
#include <message/message.hpp>
#include <message/message.hpp>
#include <logger/logger.hpp>


#define WORKER_THREAD_COUNT                             10
#define RANDOM_SEND_MSG_COUNT_DOWN          10
#define MAX_THREAD_MSG_COUNT                        500000
#define MULTI_THREADS_COUNT                     "multi_threads_count"

extern void worker_task_func(void *arg);


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

            virtual int32_t init() 
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

            int32_t send(std::shared_ptr<message> msg)
            {
                std::unique_lock<std::mutex> lock(m_mutex);

                if (m_queue.size() >= MAX_THREAD_MSG_COUNT)
                {
                    LOG_ERROR << "worker thread message queue full: " << msg->get_name();
                    return ERR_FAILED;
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

                    while (!msg_queue.empty())
                    {
                        msg = msg_queue.front();
                        on_invoke(msg);
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

            typedef std::mutex mutex_type;

            virtual ~multi_thread_module() {}

            virtual std::string name() const { return "multi thread module"; };

            virtual int32_t init(any_map &vars) 
            { 
                int threads_count = boost::any_cast<int>(vars.get(MULTI_THREADS_COUNT));
                for (int i = 0; i < threads_count; i++)
                {
                    std::shared_ptr<worker_thread> worker = std::make_shared<worker_thread>(i);
                    worker->init();
                    m_workers.push_back(worker);
                }

                return service_init(vars);
            }

            virtual int32_t exit() 
            {
                for (auto worker : m_workers)
                {
                    worker->exit();
                }

                return service_exit();
            }

            virtual int32_t start() 
            {
                for (auto worker : m_workers)
                {
                    worker->start();
                }

                return ERR_SUCCESS; 
            }

            virtual int32_t stop()
            {
                for (auto worker : m_workers)
                {
                    worker->stop();
                }

                return ERR_SUCCESS;
            }

            virtual int32_t on_timer(std::shared_ptr<base_timer> timer) { return ERR_SUCCESS; }

            int32_t send(std::shared_ptr<message> msg)
            {
                static int count_down = RANDOM_SEND_MSG_COUNT_DOWN;

                int32_t ret = ERR_SUCCESS;

                std::unique_lock<std::mutex> lock(m_mutex);
                ret = m_workers.front()->send(msg);

                if (--count_down <= 0)
                {
                    auto worker = m_workers.front();

                    m_workers.pop_front();
                    m_workers.push_back(worker);

                    count_down = RANDOM_SEND_MSG_COUNT_DOWN;
                }

                return ret;
            }

            void register_msg_functor(const std::string & msg_name, worker_thread::msg_functor_type functor)
            {
                for (auto worker : m_workers)
                {
                    worker->register_msg_functor(msg_name, functor);
                }
            }

        protected:

            virtual int32_t service_init(any_map &vars) { return ERR_SUCCESS; }

            virtual int32_t service_exit() { return ERR_SUCCESS; }

        protected:

            mutex_type m_mutex;

            std::list<std::shared_ptr<worker_thread>> m_workers;

        };

    }

}
