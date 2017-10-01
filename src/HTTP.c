/* This is a managed file. Do not delete this comment. */

#include <corto/httpserver/httpserver.h>
#define SERVER_MAX_SERVERS (64) /* Maximum number of services that may be active */
/*
 * Returns a static string representing the name of the HTTP method.
 */
static const char* _server_HTTP_getMethodName(httpserver_HTTP_Method method)
{
    switch (method) {
    case Httpserver_None:
        return "NONE";
    case Httpserver_Get:
        return "GET";
    case Httpserver_Head:
        return "HEAD";
    case Httpserver_Post:
        return "POST";
    case Httpserver_Put:
        return "PUT";
    case Httpserver_Delete:
        return "DELETE";
    case Httpserver_Trace:
        return "TRACE";
    case Httpserver_Options:
        return "OPTIONS";
    case Httpserver_Connect:
        return "CONNECT";
    case Httpserver_Patch:
        return "PATCH";
    }

    return NULL;
}

static corto_mutex_s serverLock = CORTO_MUTEX_INITIALIZER;
static struct {
    httpserver_HTTP server;
    corto_uint16 port;
} servers[SERVER_MAX_SERVERS];
void httpserver_HTTP_addService(
    httpserver_HTTP this,
    httpserver_Service s)
{

    httpserver_ServiceListAppend(this->services, s);
    corto_ok("HTTP: registered '%s' service on '%s'", 
        corto_fullpath(NULL, corto_typeof(s)),
        s->prefix);

}

void httpserver_HTTP_broadcast(
    httpserver_HTTP this,
    corto_string msg)
{
    corto_debug("HTTP: broadcast '%s'", msg);

    corto_iter it = corto_ll_iter(this->connections);
    while (corto_iter_hasNext(&it)) {
        httpserver_HTTP c = corto_iter_next(&it);
        httpserver_HTTP_write(this, c, msg);
    }

}

void httpserver_HTTP_destruct(
    httpserver_HTTP this)
{

    httpserver_HTTP_set(this->port, NULL);

}

void httpserver_HTTP_doClose(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c)
{

    corto_iter it = corto_ll_iter(this->services);
    while (corto_iter_hasNext(&it)) {
        httpserver_Service s = corto_iter_next(&it);
        httpserver_Service_onClose(s, c);
    }

    httpserver_HTTP_ConnectionListRemove(this->connections, c);
}

void httpserver_HTTP_doMessage(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c,
    corto_string msg)
{

    corto_iter it = corto_ll_iter(this->services);
    while (corto_iter_hasNext(&it)) {
        httpserver_Service s = corto_iter_next(&it);
        httpserver_Service_onMessage(s, c, msg);
    }

}

void httpserver_HTTP_doOpen(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c)
{

    httpserver_HTTP_ConnectionListAppend(this->connections, c);

    corto_iter it = corto_ll_iter(this->services);
    while (corto_iter_hasNext(&it)) {
        httpserver_Service s = corto_iter_next(&it);
        httpserver_Service_onOpen(s, c);
    }

}

void httpserver_HTTP_doPoll(
    httpserver_HTTP this)
{
    this->pollCount ++;
    if (this->pollCount == this->pollServiceRate) {
        corto_iter it = corto_ll_iter(this->services);
        while (corto_iter_hasNext(&it)) {
            httpserver_Service s = corto_iter_next(&it);
            httpserver_Service_onPoll(s);
        }

        this->pollCount = 0;
    }

}

void httpserver_HTTP_doRequest(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r)
{
    int handled = 0;

    httpserver_HTTP_Request_setStatus(r, 200);

    corto_iter it = corto_ll_iter(this->services);
    while (corto_iter_hasNext(&it)) {
        httpserver_Service s = corto_iter_next(&it);
        corto_string prefix = s->prefix ? s->prefix : "";

        int prefixLength = strlen(prefix);
        int uriLength = strlen(r->uri) - 1;

        if (!prefixLength || /* No prefix */
            /* Check if prefix exactly matches whole elements at start of URI */
            (!memcmp(r->uri + 1, prefix, prefixLength) && 
            (!r->uri[prefixLength + 1] || (r->uri[prefixLength + 1] == '/'))))
        {
            corto_string uri = r->uri + (prefixLength ? (1 + prefixLength) : 0);
            bool trailingSlash = uriLength > 1 && r->uri[uriLength] == '/';
            
            corto_debug("HTTP: relative uri for '%s' is '%s' (trailingslash = %s)",
                corto_fullpath(NULL, s),
                uri,
                trailingSlash ? "true" : "false");

            /* If requesting the root of an endpoint without trailing '/',
             * redirect to URL with trailing '/'. This ensures that pages can be
             * hosted relative to their endpoint, as follow-up requests will
             * include the path to the endpoint. */
            if (s->redirectEndpointToPath && prefix[0] && !trailingSlash && !uri[0]) {
                corto_trace("auto-redirect '%s' to '%s/'", r->uri, r->uri);
                httpserver_HTTP_Request_setStatus(r, 301);
                httpserver_HTTP_Request_setHeader(r, "Location", strarg(
                    "%s/", r->uri
                ));
                handled = true;
            } else {
                corto_trace("HTTP: attempt '%s' '%s' with service '%s'",
                    _server_HTTP_getMethodName(r->method),
                    uri,
                    corto_fullpath(NULL, s));

                switch(r->method) {
                case Httpserver_Get:
                    handled = httpserver_Service_onGet(s, c, r, uri);
                    break;
                case Httpserver_Post:
                    handled = httpserver_Service_onPost(s, c, r, uri);
                    break;
                case Httpserver_Put:
                    handled = httpserver_Service_onPut(s, c, r, uri);
                    break;
                case Httpserver_Delete:
                    handled = httpserver_Service_onDelete(s, c, r, uri);
                    break;
                default:
                    break;
                }


                /* Log if method-specific handlers were invoked */
                if (handled) {
                    corto_ok(
                      "HTTP: %s: %s '%s' success",
                      corto_idof(corto_typeof(s)),
                      _server_HTTP_getMethodName(r->method),
                      r->uri);
                }

                /* Log if generic handler was invoked */
                if (httpserver_Service_onRequest(s, c, r, uri)) {
                    corto_ok("HTTP: %s: %s '%s'",
                        corto_idof(corto_typeof(s)),
                        _server_HTTP_getMethodName(r->method),
                        r->uri);
                    handled = TRUE;
                }
            }

            if (handled) {
                break;
            }

        }

    }

    if (!handled) {
        corto_string str = corto_asprintf("Resource not found: %s '%s'", _server_HTTP_getMethodName(r->method), r->uri);
        httpserver_HTTP_Request_setStatus(r, 404);
        httpserver_HTTP_Request_reply(r, str);
        corto_dealloc(str);
        corto_warning("HTTP: %s '%s' not matched (404)", _server_HTTP_getMethodName(r->method), r->uri);
    }

}

httpserver_HTTP httpserver_HTTP_get(
    uint16_t port)
{
    corto_int32 i = 0;

    corto_mutexLock(&serverLock);

    while ((i < SERVER_MAX_SERVERS) &&
           (servers[i].port != port))
    {
        i++;
    }

    corto_mutexUnlock(&serverLock);
    if (i >= SERVER_MAX_SERVERS) {
        return NULL;
    }

    return servers[i].server;
}

void httpserver_HTTP_removeService(
    httpserver_HTTP this,
    httpserver_Service s)
{

    httpserver_ServiceListRemove(this->services, s);
    corto_ok("HTTP: removed %s service", corto_fullpath(NULL, corto_typeof(s)));

}

bool httpserver_HTTP_set(
    uint16_t port,
    httpserver_HTTP server)
{
    corto_int32 i = 0;
    corto_bool result = TRUE;

    corto_mutexLock(&serverLock);

    while ((i < SERVER_MAX_SERVERS) &&
           (servers[i].port &&
           (servers[i].port != port)))
    {
        i++;
    }

    if (i >= SERVER_MAX_SERVERS) {
        result = FALSE;
    } else if (!servers[i].port || (servers[i].port == port)) {
        if (server && servers[i].server) {
            result = FALSE;
        } else if (server) {
            servers[i].server = server;
            servers[i].port = port;
        } else {
            servers[i].port = 0;
            servers[i].server = NULL;
        }

    }

    corto_mutexUnlock(&serverLock);
    corto_ok("HTTP: started server on port %d", port);
    return result;
}

void httpserver_HTTP_write_v(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c,
    corto_string msg)
{

    /* << Insert implementation >> */

}


