/* This is a managed file. Do not delete this comment. */

#include <corto.httpserver>

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

static ut_mutex_s serverLock = UT_MUTEX_INIT;
static struct {
    httpserver_HTTP server;
    corto_uint16 port;
} servers[SERVER_MAX_SERVERS];

void httpserver_HTTP_add_service(
    httpserver_HTTP this,
    httpserver_Service s)
{
    httpserver_ServiceList__append(this->services, s);
    ut_ok("HTTP: registered '%s' service on '%s'",
        corto_fullpath(NULL, corto_typeof(s)),
        s->endpoint);
}

void httpserver_HTTP_broadcast(
    httpserver_HTTP this,
    const char *msg)
{
    ut_debug("HTTP: broadcast '%s'", msg);

    ut_iter it = ut_ll_iter(this->connections);
    while (ut_iter_hasNext(&it)) {
        httpserver_HTTP_Connection c = ut_iter_next(&it);
        httpserver_HTTP_write(this, c, msg);
    }
}

void httpserver_HTTP_destruct(
    httpserver_HTTP this)
{
    httpserver_HTTP_set_server(this->port, NULL);
}

void httpserver_HTTP_do_close(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c)
{
    ut_iter it = ut_ll_iter(this->services);
    while (ut_iter_hasNext(&it)) {
        httpserver_Service s = ut_iter_next(&it);
        httpserver_Service_on_close(s, c);
    }

    httpserver_HTTP_ConnectionList__remove(this->connections, c);
}

void httpserver_HTTP_do_message(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c,
    const char *msg)
{
    ut_iter it = ut_ll_iter(this->services);
    while (ut_iter_hasNext(&it)) {
        httpserver_Service s = ut_iter_next(&it);
        httpserver_Service_on_message(s, c, msg);
    }
}

void httpserver_HTTP_do_open(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c)
{
    httpserver_HTTP_ConnectionList__append(this->connections, c);

    ut_iter it = ut_ll_iter(this->services);
    while (ut_iter_hasNext(&it)) {
        httpserver_Service s = ut_iter_next(&it);
        httpserver_Service_on_open(s, c);
    }
}

void httpserver_HTTP_do_poll(
    httpserver_HTTP this)
{
    this->pollCount ++;
    if (this->pollCount == this->pollServiceRate) {
        ut_iter it = ut_ll_iter(this->services);
        while (ut_iter_hasNext(&it)) {
            httpserver_Service s = ut_iter_next(&it);
            httpserver_Service_on_poll(s);
        }

        this->pollCount = 0;
    }
}

static
void httpserver_HTTP_auto_redirect(
    httpserver_HTTP this,
    httpserver_HTTP_Request *r)
{
    ut_info("auto-redirect '%s' to '%s/'", r->uri, r->uri);
    httpserver_HTTP_Request_setStatus(r, 301);
    httpserver_HTTP_Request_setHeader(r, "Location", strarg(
        "%s/", r->uri
    ));
}

static
int httpserver_HTTP_dispatch(
    httpserver_HTTP this,
    httpserver_Service s,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    int handled = false;

    ut_debug("attempt '%s' '%s' with service '%s'",
        _server_HTTP_getMethodName(r->method),
        uri,
        corto_fullpath(NULL, s));

    switch(r->method) {
    case Httpserver_Get:
        handled = httpserver_Service_on_get(s, c, r, uri);
        break;
    case Httpserver_Post:
        handled = httpserver_Service_on_post(s, c, r, uri);
        break;
    case Httpserver_Put:
        handled = httpserver_Service_on_put(s, c, r, uri);
        break;
    case Httpserver_Delete:
        handled = httpserver_Service_on_delete(s, c, r, uri);
        break;
    default:
        break;
    }

    /* Log if method-specific handlers were invoked */
    if (handled) {
        ut_ok(
          "%s '%s' matched",
          _server_HTTP_getMethodName(r->method),
          r->uri);
    }

    /* Log if generic handler was invoked */
    if (httpserver_Service_on_request(s, c, r, uri)) {
        ut_ok("%s '%s' matched",
            _server_HTTP_getMethodName(r->method),
            r->uri);
        handled = TRUE;
    }

    return handled;
}

void httpserver_HTTP_do_request(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r)
{
    ut_iter it;
    int handled = 0;
    ut_log_push("HTTP");

    /* Set default HTTP status, in case no services are registered */
    httpserver_HTTP_Request_setStatus(r, 200);

    ut_trace("received %s '%s'",
        _server_HTTP_getMethodName(r->method),
        r->uri);

    /* Call pre-request callbacks of registered infrastructure services */
    ut_ll infra_service_ctx = NULL;
    if (this->infra_services) {
        infra_service_ctx = ut_ll_new();
        it = ut_ll_iter(this->infra_services);
        while (ut_iter_hasNext(&it)) {
            httpserver_Service s = ut_iter_next(&it);
            uintptr_t ctx = httpserver_Service_on_pre_request(s, c, r);
            /* Store context so it can be passed to post-hook */
            ut_ll_append(infra_service_ctx, (void*)ctx);
        }
    }

    it = ut_ll_iter(this->services);
    while (ut_iter_hasNext(&it)) {
        httpserver_Service s = ut_iter_next(&it);
        corto_string endpoint = s->endpoint ? s->endpoint : "";

        ut_log_push(corto_idof(s));

        /* Reset HTTP status for each attempt */
        httpserver_HTTP_Request_setStatus(r, 200);

        int endpointLength = strlen(endpoint);
        int uriLength = strlen(r->uri) - 1;

        if (!endpointLength || /* No endpoint */
            /* Check if endpoint exactly matches whole elements at start of URI */
            (!memcmp(r->uri + 1, endpoint, endpointLength) &&
            (!r->uri[endpointLength + 1] || (r->uri[endpointLength + 1] == '/'))))
        {
            corto_string uri = r->uri + (endpointLength ? (1 + endpointLength) : 0);
            bool trailingSlash = uriLength > 1 && r->uri[uriLength] == '/';

            ut_debug("relative uri = '%s' (trailingslash = '%s')",
                uri,
                trailingSlash ? "true" : "false");

            /* If requesting the root of an endpoint without trailing '/',
             * redirect to URL with trailing '/'. This ensures that pages can be
             * hosted relative to their endpoint, as follow-up requests will
             * include the path to the endpoint. */
            if (s->redirectEndpointToPath && endpoint[0] && !trailingSlash && !uri[0]) {
                httpserver_HTTP_auto_redirect(this, r);
                handled = true;
            } else {
                handled = httpserver_HTTP_dispatch(this, s, c, r, uri);
            }

            if (handled) {
                ut_log_pop();
                break;
            }
        }

        ut_log_pop();
    }

    if (!handled) {
        corto_string str = ut_asprintf(
            "Resource not found: %s '%s'",
            _server_HTTP_getMethodName(r->method), r->uri);
        httpserver_HTTP_Request_setStatus(r, 404);
        httpserver_HTTP_Request_reply(r, str);
        corto_dealloc(str);
        ut_warning(
            "%s '%s' not matched (404)",
            _server_HTTP_getMethodName(r->method), r->uri);
    }

    /* Call post-request callbacks of registered infrastructure services */
    it = ut_ll_iter(this->infra_services);
    ut_iter ctx_iter = ut_ll_iter(infra_service_ctx);
    while (ut_iter_hasNext(&it) && ut_iter_hasNext(&ctx_iter)) {
        httpserver_Service s = ut_iter_next(&it);
        uintptr_t ctx = (uintptr_t)ut_iter_next(&ctx_iter);
        httpserver_Service_on_post_request(s, c, r, ctx);
    }
    if (infra_service_ctx) {
        ut_ll_free(infra_service_ctx);
    }

    ut_log_pop();
}

httpserver_HTTP httpserver_HTTP_get_server(
    uint16_t port)
{
    corto_int32 i = 0;

    ut_mutex_lock(&serverLock);

    while ((i < SERVER_MAX_SERVERS) &&
           (servers[i].port != port))
    {
        i++;
    }

    ut_mutex_unlock(&serverLock);
    if (i >= SERVER_MAX_SERVERS) {
        return NULL;
    }

    return servers[i].server;
}

void httpserver_HTTP_remove_service(
    httpserver_HTTP this,
    httpserver_Service s)
{
    httpserver_ServiceList__remove(this->services, s);
    ut_ok("HTTP: removed %s service", corto_fullpath(NULL, corto_typeof(s)));
}

bool httpserver_HTTP_set_server(
    uint16_t port,
    httpserver_HTTP server)
{
    corto_int32 i = 0;
    corto_bool result = TRUE;

    ut_mutex_lock(&serverLock);

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

    ut_mutex_unlock(&serverLock);
    ut_ok("HTTP: started server on port %d", port);
    return result;
}

void httpserver_HTTP_write_v(
    httpserver_HTTP this,
    httpserver_HTTP_Connection c,
    const char *msg)
{
}

int16_t httpserver_HTTP_construct(
    httpserver_HTTP this)
{
    return 0;
}

void httpserver_HTTP_add_infra_service(
    httpserver_HTTP this,
    httpserver_Service s)
{
    httpserver_ServiceList__append(this->infra_services, s);
    ut_ok("HTTP: registered infrastructure service '%s'",
        corto_fullpath(NULL, corto_typeof(s)));
}

void httpserver_HTTP_remove_infra_service(
    httpserver_HTTP this,
    httpserver_Service s)
{
    httpserver_ServiceList__remove(this->infra_services, s);
    ut_ok("HTTP: removed infrastructure '%s' service",
        corto_fullpath(NULL, corto_typeof(s)));
}
