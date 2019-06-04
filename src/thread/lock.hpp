#pragma once


#include <mutex>
#include <condition_variable>


namespace micro
{
    namespace core
    {

        class rw_mutex
        {
        public:

            typedef std::unique_lock<std::mutex> LOCK_TYPE;

            rw_mutex() 
                : m_r_count(0)
                , m_w_count(0)
                , m_w_status(false)
            {}

            ~rw_mutex() = default;

            void r_lock()
            {
                LOCK_TYPE lock(m_mutex);
                m_r_cv.wait(lock, [=]()->bool {return 0 == m_w_count;});
                ++m_r_count;
            }

            void r_unlock()
            {
                LOCK_TYPE lock(m_mutex);
                if ((--m_r_count == 0) && (m_w_count > 0))
                {
                    m_w_cv.notify_one();
                }
            }

            void w_lock()
            {
                LOCK_TYPE lock(m_mutex);
                ++m_w_count;
                m_w_cv.wait(lock, [=]()->bool {return (0 == m_r_count) && (!m_w_status);});
                m_w_status = true;
            }

            void w_unlock()
            {
                LOCK_TYPE lock(m_mutex);
                (--m_w_count == 0) ? m_r_cv.notify_all() : m_w_cv.notify_one();
                m_w_status = false;
            }

        private:

            std::mutex m_mutex;

            volatile bool m_w_status;

            volatile uint32_t m_r_count;

            volatile uint32_t m_w_count;

            std::condition_variable m_r_cv;

            std::condition_variable m_w_cv;

        };

        class r_lock_guard
        {
        public:

            r_lock_guard(rw_mutex &mutex)
                : m_mutex(mutex)
            { m_mutex.r_lock(); }

            ~r_lock_guard() { m_mutex.r_unlock(); }

            void lock() { m_mutex.r_lock(); }

            void unlock() { m_mutex.r_unlock(); }

            r_lock_guard() = delete;
            r_lock_guard(const r_lock_guard&) = delete;
            r_lock_guard& operator=(const r_lock_guard&) = delete;

        protected:

            rw_mutex & m_mutex;

        };

        class w_lock_guard
        {
        public:

            w_lock_guard(rw_mutex &mutex)
                : m_mutex(mutex)
            { m_mutex.w_lock(); }

            ~w_lock_guard() { m_mutex.w_unlock(); }

            void lock() { m_mutex.w_lock(); }

            void unlock() { m_mutex.w_unlock(); }

            w_lock_guard() = delete;
            w_lock_guard(const w_lock_guard&) = delete;
            w_lock_guard& operator=(const w_lock_guard&) = delete;

        protected:

            rw_mutex & m_mutex;
        };

    }

}
