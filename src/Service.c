/* This is a managed file. Do not delete this comment. */

#include <corto/httpserver/httpserver.h>
int16_t httpserver_Service_construct(
    httpserver_Service this)
{
    httpserver_HTTP server = NULL;

    if (!this->prefix) {
        corto_set_str(&this->prefix, "");
    }

    /* Fetch HTTP server for port. If none is registered, automatically create
     * a standalone server */
    while (!server) {
        server = httpserver_HTTP_get(httpserver_Service(this)->port);
        if (!server) {
            server = httpserver_HTTP(httpserver_StandaloneHTTPCreate(
                httpserver_Service(this)->port, 50, 5, FALSE));
            /* Server can be NULL if another thread registered a server for the
             * same port. Therefore, keep trying until a valid server object has
             * been obtained. */
        }

    }

    corto_set_ref(&this->server, server);
    httpserver_HTTP_addService(server, this);
    return 0;
}

void httpserver_Service_destruct(
    httpserver_Service this)
{

    httpserver_HTTP_removeService(this->server, this);

}

void httpserver_Service_onClose_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c)
{

}

int16_t httpserver_Service_onDelete_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    return 0;
}

int16_t httpserver_Service_onGet_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    return 0;
}

void httpserver_Service_onMessage_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    const char *msg)
{

    /* virtual method */

}

void httpserver_Service_onOpen_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c)
{

    /* virtual method */

}

void httpserver_Service_onPoll_v(
    httpserver_Service this)
{

    /* virtual method */

}

int16_t httpserver_Service_onPost_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    return 0;
}

int16_t httpserver_Service_onPut_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    return 0;
}

int16_t httpserver_Service_onRequest_v(
    httpserver_Service this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{

    return 0;

}

