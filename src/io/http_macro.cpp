#include <io/http_macro.hpp>
#include <io/http_client.hpp>
#include <io/http_server.hpp>


const std::string CRLF = "\r\n";


void free_context(uv_handle_t* handle) 
{
    auto* context = reinterpret_cast<micro::core::http_context *>(handle->data);
    context->m_writes.clear();
    free(context);
}


namespace micro
{
    namespace core
    {

        template void attachEvents<http_client>(http_client* instance, http_parser_settings& settings);
        template void attachEvents<http_server>(http_server* instance, http_parser_settings& settings);

        //
        // Events
        //
        template <class Type>
        void attachEvents(Type * instance, http_parser_settings & settings)
        {
            // http parser callback types
            static std::function<int(http_parser* parser)> on_message_complete;

            static auto callback = instance->m_functor;
            // called once a connection has been made and the message is complete.
            on_message_complete = [&](http_parser* parser) -> int 
            {
                return instance->complete(parser, callback);
                return 0;
            };

            // called after the url has been parsed.
            settings.on_url =
                [](http_parser* parser, const char* at, size_t len) -> int {
                micro::core::http_context* context = static_cast<micro::core::http_context*>(parser->data);
                if (at && context) { context->m_url = std::string(at, len); }
                return 0;
            };

            // called when there are either fields or values in the request.
            settings.on_header_field =
                [](http_parser* parser, const char* at, size_t length) -> int {
                return 0;
            };

            // called when header value is given
            settings.on_header_value =
                [](http_parser* parser, const char* at, size_t length) -> int {
                return 0;
            };

            // called once all fields and values have been parsed.
            settings.on_headers_complete =
                [](http_parser* parser) -> int {
                micro::core::http_context* context = static_cast<micro::core::http_context*>(parser->data);
                context->m_method = std::string(http_method_str((enum http_method) parser->method));
                return 0;
            };

            // called when there is a body for the request.
            settings.on_body =
                [](http_parser* parser, const char* at, size_t len) -> int {
                micro::core::http_context* context = static_cast<micro::core::http_context*>(parser->data);
                if (at && context && (int)len > -1) {
                    context->m_body << std::string(at, len);
                }
                return 0;
            };

            // called after all other events.
            settings.on_message_complete =
                [](http_parser* parser) -> int {
                return on_message_complete(parser);
            };
        }

    }

}
