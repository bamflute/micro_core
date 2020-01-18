#pragma once

#include <queue>
#include <message/message.hpp>


#define DEFAULT_PRIORITY_COUNT      3


namespace micro
{
    namespace core
    {

        template<typename T>
        class multi_priority_queue
        {
        public:

            typedef T value_type;

            typedef T& reference;

            typedef const T& const_reference;

            typedef size_t size_type;

            multi_priority_queue(uint32_t priority_count = DEFAULT_PRIORITY_COUNT) 
                : m_priority_count(priority_count < DEFAULT_PRIORITY_COUNT ? DEFAULT_PRIORITY_COUNT : priority_count)
            {
                m_queues = new std::queue<T> *[m_priority_count];
                for (uint32_t i = 0; i < m_priority_count; i++)
                {
                    m_queues[i] = new std::queue<T>();
                }
            }

            ~multi_priority_queue()
            {
                for (uint32_t i = 0; i < m_priority_count; i++)
                {
                    //m_queues[i]->clear();
                    delete m_queues[i];
                }
                delete [] m_queues;
            }

            bool empty() const
            {
                for (uint32_t i = 0; i < m_priority_count; i++)
                {
                    if (!m_queues[i]->empty())
                    {
                        return false;
                    }
                }
                return true;
            }

            size_type size() const
            {
                size_type total_size = 0;
                for (uint32_t i = 0; i < m_priority_count; i++)
                {
                    total_size += m_queues[i]->size();
                }
                return total_size;
            }

            void push(const value_type& value, uint32_t priority = LOW_PRIORITY)
            {
                assert(priority < m_priority_count);
                m_queues[priority]->push(value);
            }

            void push(value_type&& value, uint32_t priority = LOW_PRIORITY)
            {
                assert(priority < m_priority_count);
                m_queues[priority]->push(value);
            }

            reference front()
            {
                for (uint32_t i = 0; i < m_priority_count; i++)
                {
                    if (!m_queues[i]->empty())
                    {
                        return m_queues[i]->front();
                    }
                }

                throw new std::runtime_error("multi priority queue fron error");
            }

            const_reference front() const
            {
                for (uint32_t i = 0; i < m_priority_count; i++)
                {
                    if (!m_queues[i]->empty())
                    {
                        return m_queues[i]->front();
                    }
                }

                throw new std::runtime_error("multi priority queue fron error");
            }

            void pop()
            {
                for (uint32_t i = 0; i < m_priority_count; i++)
                {
                    if (!m_queues[i]->empty())
                    {
                        return m_queues[i]->pop();
                    }
                }
            }

        protected:

            const uint32_t m_priority_count;

            std::queue<T> ** m_queues;           //0-->top priority, 1, 2-->less priority
        };

    }

};
