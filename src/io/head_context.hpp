#pragma once


#include <io/io_context.hpp>


namespace micro
{
    namespace core
    {

        class head_context : public io_context
        {
        public:

            virtual void channel_active(context_type &ctx) { ctx.fire_channel_active(); }

            virtual void channel_inactive(context_type &ctx) { ctx.fire_channel_inactive(); }

            virtual void channel_read(context_type &ctx) { ctx.fire_channel_read(); }

            virtual void channel_read_complete(context_type &ctx) { ctx.fire_channel_read_complete(); }

            virtual void channel_writablity_changed(context_type &ctx) { ctx.fire_channel_writablity_changed(); }

            virtual void bind(context_type &ctx, const socket_address &local_addr) { ctx.bind(local_addr); }

            virtual void connect(context_type &ctx, const socket_address &local_addr, const socket_address &remote_addr) { ctx.connect(local_addr, remote_addr); }

            virtual void close(context_type &ctx) { ctx.close(); }

        };

    }

}