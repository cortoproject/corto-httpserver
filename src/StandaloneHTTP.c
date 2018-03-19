/* This is a managed file. Do not delete this comment. */

#include <src/civetweb.h>
#include <corto/httpserver/httpserver.h>
#define DOCUMENT_ROOT "."

static int
cb_logMessage(const struct mg_connection *conn, const char *msg)
{
    corto_info("StandaloneHTTP: %s", (char*)msg);
    return 1;
}

static httpserver_HTTP_Method methodFromStr(const char *method) {
    httpserver_HTTP_Method result;
    if (!strcmp(method, "NONE")) result = Httpserver_None;
    else if (!strcmp(method, "GET")) result = Httpserver_Get;
    else if (!strcmp(method, "HEAD")) result = Httpserver_Head;
    else if (!strcmp(method, "POST")) result = Httpserver_Post;
    else if (!strcmp(method, "PUT")) result = Httpserver_Put;
    else if (!strcmp(method, "DELETE")) result = Httpserver_Delete;
    else if (!strcmp(method, "TRACE")) result = Httpserver_Trace;
    else if (!strcmp(method, "OPTIONS")) result = Httpserver_Options;
    else if (!strcmp(method, "CONNECT")) result = Httpserver_Connect;
    else if (!strcmp(method, "PATCH")) result = Httpserver_Patch;
    else result = Httpserver_None;
    return result;
}

static
int cb_wsConnect(
    const struct mg_connection *conn,
    void *cbdata)
{
    httpserver_HTTP_Connection c = mg_get_user_connection_data(conn);
    const struct mg_request_info *req_info = mg_get_request_info(conn);
    httpserver_StandaloneHTTP this = req_info->user_data;
    if (this) {
        corto_assert_object(this);
    }

    if (!c) {
        c = httpserver_HTTP_Connection__create(NULL, NULL, NULL, this);
        c->conn = (corto_word)conn;
        mg_set_user_connection_data((struct mg_connection *)conn, c);
    } else {
        corto_assert_object(c);
    }

    safe_httpserver_HTTP_doOpen(this, c);
    return 0;
}

static
void cb_wsReady(
    struct mg_connection *conn,
    void *cbdata)
{
}

static
int cb_wsData(struct mg_connection *conn,
             int bits,
             char *data,
             size_t len,
             void *cbdata)
{
    const struct mg_request_info *req_info = mg_get_request_info(conn);
    httpserver_StandaloneHTTP this = req_info->user_data;
    httpserver_HTTP_Connection c = mg_get_user_connection_data(conn);
    if (c && bits == 129) { /* Text message */
        char *msg = corto_alloc(len + 1);
        memcpy(msg, data, len);
        msg[len] = '\0';
        safe_httpserver_HTTP_doMessage(this, c, msg);
        corto_dealloc(msg);
    }

    return 1;
}

static
void cb_wsClose(
    const struct mg_connection *conn,
    void *cbdata)
{
    const struct mg_request_info *req_info = mg_get_request_info(conn);
    httpserver_StandaloneHTTP this = req_info->user_data;
    httpserver_HTTP_Connection c = mg_get_user_connection_data(conn);
    safe_httpserver_HTTP_doClose(this, c);
    corto_delete(c);
}

struct RequestCtx {
    corto_uint16 status;
    corto_buffer headers;
    corto_buffer msg;
};

static
void cb_setHeader(
    httpserver_HTTP_Request *r,
    corto_string key,
    corto_string val)
{
    struct RequestCtx *ctx = (struct RequestCtx*)r->ctx;
    corto_buffer_append(&ctx->headers, "%s: %s\r\n", key, val);
}

static
void cb_setStatus(
    httpserver_HTTP_Request *r,
    corto_uint16 status)
{
    struct RequestCtx *ctx = (struct RequestCtx*)r->ctx;
    ctx->status = status;
}

static
corto_string cb_getHeader(
    httpserver_HTTP_Request *r,
    corto_string key)
{
    return (char*)mg_get_header((struct mg_connection*)r->conn, key);
}

static
void cb_reply(
    httpserver_HTTP_Request *r,
    corto_string msg)
{
    struct RequestCtx *ctx = (struct RequestCtx*)r->ctx;
    corto_buffer_appendstr(&ctx->msg, msg);
}

static
void cb_sendFile(
    httpserver_HTTP_Request *r,
    corto_string file)
{
    mg_send_file((struct mg_connection *)r->conn, file);
    r->file = TRUE;
}

static
corto_string cb_getVar(
    httpserver_HTTP_Request *r,
    corto_string key)
{
    const struct mg_request_info *req_info =
        mg_get_request_info(((struct mg_connection*)r->conn));

    const char *query = req_info->query_string;
    if (!query) {
        query = r->body;
    }

    char *data = NULL;
    if (query) {
        /* This is a bit costly for variables with large values, but better than
         * returning a partial result. Should modify/extend civetweb so that it
         * is possible to pass NULL to data to obtain length of variable. For
         * the majority of variables however, a single alloc will be enough. */
        int size = 256, queryLen = strlen(query), ret = 0;
        data = corto_alloc(size);
        while ((ret = mg_get_var(query, queryLen, key, data, size)) == -2) {
            size *= 2;
            data = corto_realloc(data, size);
        }

        if (ret == -1) {
            corto_dealloc(data);
            data = NULL;
        } else {
            if (!r->garbage) {
                r->garbage = corto_ll_new();
            }
            corto_ll_append(r->garbage, data);
        }
    }

    /* Never return a NULL result */
    if (!data) {
        data = corto_strdup("");
    }

    return data;
}

static
int cb_onRequest(
    struct mg_connection *conn,
    void *cbdata)
{
    /* Handler may access the request info using mg_get_request_info */
    const struct mg_request_info *req_info = mg_get_request_info(conn);
    httpserver_StandaloneHTTP this = req_info->user_data;
    httpserver_HTTP_Method method = methodFromStr(req_info->request_method);

    corto_assert(this != NULL, "server parameter not set");

    struct RequestCtx ctx = {
        .status = 200,
        .headers = CORTO_BUFFER_INIT,
        .msg = CORTO_BUFFER_INIT
    };

    httpserver_HTTP_Request r = {
        .uri = (char*)req_info->local_uri,
        .method = method,
        .conn = (corto_word)conn,
        .file = FALSE,
        .garbage = NULL,
        .ctx = (corto_word)&ctx
    };

    /* Copy callbacks from server */
    r.m_setHeader = this->super.m_setHeader;
    r.m_setStatus = this->super.m_setStatus;
    r.m_getHeader = this->super.m_getHeader;
    r.m_reply = this->super.m_reply;
    r.m_sendFile = this->super.m_sendFile;
    r.m_getVar = this->super.m_getVar;

    if (req_info->content_length) {
        /* If content length is set, obtain body directly */
        r.body = corto_alloc(req_info->content_length + 1);
        mg_read(conn, r.body, req_info->content_length);
        r.body[req_info->content_length] = '\0';
    } else {
        /* Large bodies can be sent in multiple chunks */
        corto_buffer body_buffer = CORTO_BUFFER_INIT;
        char buf[256];
        int read = mg_read(conn, buf, sizeof(buf));
        while (read > 0) {
            corto_buffer_append(&body_buffer, buf);
            read = mg_read(conn, buf, sizeof(buf));
        }
        r.body = corto_buffer_str(&body_buffer);
    }

    /* Send request to services */
    safe_httpserver_HTTP_doRequest(this, NULL, &r);

    /* Append 'Connection: close' header */
    corto_buffer_appendstr(&ctx.headers, "Connection: close\r\n");

    /* Send message */
    char *headers = corto_buffer_str(&ctx.headers);
    char *msg = corto_buffer_str(&ctx.msg);
    mg_printf(conn, "HTTP/1.1 %d OK\r\n%s\r\n%s", ctx.status, headers, msg);

    /* Cleanup any strings from request */
    if (r.garbage) {
        corto_iter it = corto_ll_iter(r.garbage);
        while (corto_iter_hasNext(&it)) {
            corto_dealloc(corto_iter_next(&it));
        }
        corto_ll_free(r.garbage);
    }

    if (r.body) {
        corto_dealloc(r.body);
    }

    return 1;
}

static
void* pollThread(
    void *ctx)
{
    httpserver_StandaloneHTTP this = ctx;
    while (1) {
        safe_httpserver_HTTP_doPoll(this);
        if (this->exiting) {
            break;
        }
        corto_sleep(0, httpserver_HTTP(this)->pollInterval * 1000000);
    }

    return NULL;
}

int16_t httpserver_StandaloneHTTP_construct(
    httpserver_StandaloneHTTP this)
{
    char port[15];
    sprintf(port, "%d", httpserver_HTTP(this)->port);

    const char *options[] = {
        "document_root", DOCUMENT_ROOT,
        "listening_ports", port,
        "request_timeout_ms", "10000",
        "error_log_file", "error.log",
        "enable_auth_domain_check", "no",
#if defined(__linux__)
        "allow_sendfile_call", this->enable_sendfile ? "true" : "false",
#endif
         NULL};
    struct mg_callbacks callbacks;
    struct mg_context *ctx;
    if (!mg_check_feature(8)) {
        corto_trace("StandaloneHTTP: IPv6 not supported");
    }

    if (!mg_check_feature(16)) {
        corto_trace("StandaloneHTTP: websockets not supported");
    }

    if (!mg_check_feature(2)) {
        corto_trace("StandaloneHTTP: SSL not supported");
    }

    if (!safe_httpserver_HTTP_set_server(httpserver_HTTP(this)->port, this)) {
        corto_throw("port %d already occupied by other server",
            httpserver_HTTP(this)->port);
        goto error;
    }

    /* Set Request delegate callbacks */
    httpserver_HTTP_Request_d_setHeader__init_c(&this->super.m_setHeader, cb_setHeader);
    httpserver_HTTP_Request_d_setStatus__init_c(&this->super.m_setStatus, cb_setStatus);
    httpserver_HTTP_Request_d_getHeader__init_c(&this->super.m_getHeader, cb_getHeader);
    httpserver_HTTP_Request_d_reply__init_c(&this->super.m_reply, cb_reply);
    httpserver_HTTP_Request_d_sendFile__init_c(&this->super.m_sendFile, cb_sendFile);
    httpserver_HTTP_Request_d_getVar__init_c(&this->super.m_getVar, cb_getVar);

    /* Start CivetWeb web server */
    memset(&callbacks, 0, sizeof(callbacks));
    callbacks.log_message = cb_logMessage;
    ctx = mg_start(&callbacks, this, options);
    this->server = (corto_word)ctx;

    /* Add handler for requests */
    mg_set_request_handler(ctx, "**", cb_onRequest, 0);

    /* Set websocket handlers */
    mg_set_websocket_handler(ctx,
         "/",
         cb_wsConnect,
         cb_wsReady,
         cb_wsData,
         cb_wsClose,
         0);

    /* Start thread to emit poll signal */
    this->thread = (corto_word)corto_thread_new(
        pollThread,
        this);

    return safe_httpserver_HTTP_construct(this);
error:
    return -1;
}

void httpserver_StandaloneHTTP_destruct(
    httpserver_StandaloneHTTP this)
{
    this->exiting = TRUE;

    mg_stop((struct mg_context*)this->server);
    corto_thread_join((corto_thread)this->thread, NULL);
    corto_super_destruct(this);
}

void httpserver_StandaloneHTTP_write(
    httpserver_StandaloneHTTP this,
    httpserver_HTTP_Connection c,
    const char *msg)
{
    mg_websocket_write(
        (struct mg_connection *)c->conn,
        MG_WEBSOCKET_OPCODE_TEXT,
        msg,
        strlen(msg));
}
