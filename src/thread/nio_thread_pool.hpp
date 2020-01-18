#pragma once

#include <memory>
#include  <shared_mutex>
#include <boost/asio.hpp>
#include <boost/bind.hpp>
#include <common/error.hpp>

#define MAX_THR_POOL_SIZE    128

#define BOOTSTRAP_POOL(POOL, IOS_SIZE) \
    POOL = std::make_shared<nio_thread_pool>(); \
    POOL->init(IOS_SIZE); \
    POOL->start();

namespace micro
{
    namespace core
    {

        class io_service_helper
        {
        public:

            io_service_helper()
                : m_ios(std::make_shared<boost::asio::io_service>())
                , m_ios_work(std::make_shared<boost::asio::io_service::work>(*m_ios))
            {}

            std::shared_ptr<boost::asio::io_service> get_ios() { return m_ios; }

            void stop() { if (m_ios) m_ios->stop(); }

        protected:

            std::shared_ptr<boost::asio::io_service> m_ios;
            std::shared_ptr<boost::asio::io_service::work> m_ios_work;

        };

        class nio_thread_pool
        {
        public:

            nio_thread_pool() : m_size(0), m_idx(0) {}

            ~nio_thread_pool() = default;

            int32_t init(size_t size)
            {
                m_size = size;
                assert(m_size <= MAX_THR_POOL_SIZE);                
                for (size_t i = 0; i < m_size; i++)
                {
                    m_ioses.emplace_back(std::make_shared<io_service_helper>());
                }

                return ERR_SUCCESS;
            }

            int32_t start()
            {
                try
                {
                    for (size_t i = 0; i < m_ioses.size(); i++)
                    {
                        m_thrs.emplace_back(std::make_shared<std::thread>(boost::bind(&boost::asio::io_service::run, m_ioses[i]->get_ios())));
                    }
                }
                catch (...)
                {
                    return ERR_FAILED;
                }

                return ERR_SUCCESS;
            }

            int32_t stop()
            {
                for (size_t i = 0; i < m_ioses.size(); i++)
                {
                    m_ioses[i]->stop();
                }

                return ERR_SUCCESS;
            }

            int32_t exit()
            {
                for (size_t i = 0; i < m_ioses.size(); i++)
                {
                    try
                    {
                        m_thrs[i]->join();
                    }
                    catch (...)
                    {
                        continue;
                    }
                }

                return ERR_SUCCESS;
            }

            std::shared_ptr<boost::asio::io_service> get_ios()
            {
                std::unique_lock<std::mutex> lock(m_mutex);
                std::shared_ptr<io_service_helper> helper(m_ioses[m_idx++]);
                if (m_idx == m_ioses.size())
                {
                    m_idx = 0;
                }

                return helper->get_ios();
            }


        protected:

            std::mutex m_mutex;

            size_t m_size;

            std::size_t m_idx;

            std::vector<std::shared_ptr<std::thread>> m_thrs;

            std::vector<std::shared_ptr<io_service_helper>> m_ioses;
        };

    }

}
