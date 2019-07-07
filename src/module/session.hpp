#pragma once

#include <common/any_map.hpp>

#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>
#include <boost/uuid/uuid_generators.hpp>


#define TIMER_ID    "timer_id"
#define SESSION_ID  "session_id"


namespace micro
{
    namespace core
    {

        class session
        {
        public:

            session()
            {
                boost::uuids::uuid uid = boost::uuids::random_generator()();
                m_session_id = boost::uuids::to_string(uid);
            }

            virtual ~session() { m_vars.clear(); }

            const std::string & get_session_id() const { return m_session_id; }

            std::string m_session_id;

            any_map m_vars;

        };

    }

}