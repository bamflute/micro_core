#pragma once


#include <memory>
#include <exception>
#include <io/mask.hpp>
#include <io/base_context.hpp>


namespace micro
{
    namespace core
    {

        typedef base_context context_type;
        typedef boost::asio::ip::tcp::endpoint endpoint_type;

        class io_handler
        {
        public:

            io_handler() : m_mask(0) {}

            virtual ~io_handler() = default;

            virtual void exception_caught(context_type & ctx, const std::exception & e) { ctx.fire_exception_caught(e); }

            uint32_t mask() { return m_mask; }

        protected:

            uint32_t m_mask;
        };

        class tcp_acceptor_handler : public io_handler
        {
        public:

            tcp_acceptor_handler() { m_mask |= MASK_ALL_ACCEPTOR; }

            virtual void accepted(context_type &ctx) { ctx.fire_accepted(); }

        };

        class tcp_connector_handler : public io_handler
        {
        public:

            tcp_connector_handler() { m_mask |= MASK_ALL_CONNECTOR; }

            virtual void bind(context_type &ctx, const endpoint_type &local_addr) { ctx.fire_bind(local_addr); }

            virtual void connect(context_type &ctx, const endpoint_type &remote_addr) { ctx.fire_connect(remote_addr); }

            virtual void connected(context_type &ctx) { ctx.fire_connected(); }

        };

        class channel_inbound_handler : virtual public io_handler
        {
        public:

            channel_inbound_handler() { m_mask |= MASK_ALL_INBOUND; }

            virtual void channel_active(context_type &ctx) { ctx.fire_channel_active(); }

            virtual void channel_inactive(context_type &ctx) { ctx.fire_channel_inactive(); }

            virtual void channel_read(context_type &ctx) { ctx.fire_channel_read(); }

            virtual void channel_read_complete(context_type &ctx) { ctx.fire_channel_read_complete(); }

            //virtual void channel_writablity_changed(context_type &ctx) { ctx.fire_channel_writablity_changed(); }
            
        };

        class channel_outbound_handler : virtual public io_handler
        {
        public:

            channel_outbound_handler() { m_mask |= MASK_ALL_OUTBOUND; }

            virtual void channel_write(context_type &ctx) { ctx.fire_channel_write(); }

            virtual void channel_write_complete(context_type &ctx) { ctx.fire_channel_write_complete(); }

            virtual void channel_batch_write(context_type &ctx) { ctx.fire_channel_batch_write(); }

            virtual void channel_batch_write_complete(context_type &ctx) { ctx.fire_channel_batch_write_complete(); }

            virtual void close(context_type &ctx) { ctx.fire_close(); }

        };

    }

}