#pragma once


#include <unordered_map>
#include <boost/any.hpp>


namespace micro
{
    namespace core
    {

        class any_map
        {
        public:

            typedef std::unordered_map<std::string, boost::any> map_type;

            virtual ~any_map() = default;

            void clear()
            {
                m_opts.clear();
            }

            template<typename T>
            void set(std::string name, T value)
            {
                m_opts[name] = value;
            }

            boost::any get(std::string name)
            {
                auto it = m_opts.find(name);
                if (it == m_opts.end())
                {
                    return nullptr;
                }

                return m_opts[name];
            }
            
            template<typename T>
            T get(std::string name, T default_value)
            {
                auto it = m_opts.find(name);
                if (it == m_opts.end())
                {
                    return default_value;
                }

                return boost::any_cast<T>(m_opts[name]);
            }
            
            template<typename T>
            bool get_a(std::string name, T & value)
            {
                auto it = m_opts.find(name);
                if (it == m_opts.end())
                {
                    return false;
                }

                value =  boost::any_cast<T>(m_opts[name]);
                return true;
            }

            int count(std::string name)
            {
                return m_opts.find(name) == m_opts.end() ? 0 : 1;
            }

            map_type & opts() { return m_opts; }

        protected:

            map_type m_opts;
        };

    }

}
