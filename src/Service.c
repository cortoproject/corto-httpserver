/* This is a managed file. Do not delete this comment. */

#include <corto.httpserver>

int16_t httpserver_Service_construct(
    httpserver_Service this)
{
    httpserver_HTTP server = NULL;

    if (!this->endpoint) {
        corto_set_str(&this->endpoint, "");
    }

    /* Fetch HTTP server for port. If none is registered, automatically create
     * a standalone server */
    while (!server) {
        server = httpserver_HTTP_get_server(httpserver_Service(this)->port);
        if (!server) {
            server = httpserver_HTTP(httpserver_StandaloneHTTP__create(NULL, NULL,
                httpserver_Service(this)->port, 30, 5, FALSE));
            /* Server can be NULL if another thread registered a server for the
             * same port. Therefore, keep trying until a valid server object has
             * been obtained. */
        }
    }

    corto_set_ref(&this->server, server);
    httpserver_HTTP_add_service(server, this);
    return 0;
}

void httpserver_Service_destruct(
    httpserver_Service this)
{
    httpserver_HTTP_remove_service(this->server, this);
}

void httpserver_Service_on_close_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c)
{
    /* pure virtual method */
}

int16_t httpserver_Service_on_delete_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    /* pure virtual method */
    return 0;
}

int16_t httpserver_Service_on_get_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    /* pure virtual method */
    return 0;
}

void httpserver_Service_on_message_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    const char *msg)
{
    /* pure virtual method */
}

void httpserver_Service_on_open_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c)
{
    /* pure virtual method */
}

void httpserver_Service_on_poll_v(
    httpserver_Service this)
{
    /* pure virtual method */
}

int16_t httpserver_Service_on_post_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    /* pure virtual method */
    return 0;
}

int16_t httpserver_Service_on_put_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    /* pure virtual method */
    return 0;
}

int16_t httpserver_Service_on_request_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    /* pure virtual method */
    return 0;
}

void httpserver_Service_on_post_request_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    uintptr_t ctx)
{
    /* pure virtual method */
}

uintptr_t httpserver_Service_on_pre_request_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r)
{
    /* pure virtual method */
    return 0;
}
