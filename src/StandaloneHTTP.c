/* This is a managed file. Do not delete this comment. */

#include <corto/httpserver/httpserver.h>
#include <src/civetweb.h>
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

static int
cb_wsConnect(const struct mg_connection *conn, void *cbdata)
{
    httpserver_HTTP_Connection c = mg_get_user_connection_data(conn);
    const struct mg_request_info *req_info = mg_get_request_info(conn);
    httpserver_StandaloneHTTP this = req_info->user_data;    
    if (this) {
        corto_assertObject(this);
    }

    if (!c) {
        c = httpserver_HTTP_ConnectionCreate(NULL, this);
        c->conn = (corto_word)conn;
        mg_set_user_connection_data((struct mg_connection *)conn, c);
    } else {
        corto_assertObject(c);
    }

    safe_httpserver_HTTP_doOpen(this, c);
    return 0;
}

static void
cb_wsReady(struct mg_connection *conn, void *cbdata)
{
}

static int
cb_wsData(struct mg_connection *conn,
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

static void
cb_wsClose(const struct mg_connection *conn, void *cbdata)
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
static void 
cb_setHeader(httpserver_HTTP_Request *r, corto_string key, corto_string val) {
    struct RequestCtx *ctx = (struct RequestCtx*)r->ctx;
    corto_buffer_append(&ctx->headers, "%s: %s\r\n", key, val); 
}

static void 
cb_setStatus(httpserver_HTTP_Request *r, corto_uint16 status) {
    struct RequestCtx *ctx = (struct RequestCtx*)r->ctx;
    ctx->status = status;
}

static corto_string 
cb_getHeader(httpserver_HTTP_Request *r, corto_string key) {
    return (char*)mg_get_header((struct mg_connection*)r->conn, key);
}

static void 
cb_reply(httpserver_HTTP_Request *r, corto_string msg) {
    struct RequestCtx *ctx = (struct RequestCtx*)r->ctx;
    corto_buffer_appendstr(&ctx->msg, msg);
}

static void 
cb_sendFile(httpserver_HTTP_Request *r, corto_string file) {
    mg_send_file((struct mg_connection *)r->conn, file);
    r->file = TRUE;
}

static corto_string 
cb_getVar(httpserver_HTTP_Request *r, corto_string key) {
    /* This is a bit costly for variables with large values, but better than
     * returning a partial result. Should modify/extend civetweb so that it is
     * possible to pass NULL to data to obtain length of variable. */
    const struct mg_request_info *req_info = mg_get_request_info(((struct mg_connection*)r->conn));
    const char *query = req_info->query_string;
    char *data = NULL;

    if (query) {
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

    if (!data) {
        data = corto_strdup("");
    }

    return data;
}

static int
cb_onRequest(struct mg_connection *conn, void *cbdata)
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
    /* Set request callbacks */
    httpserver_HTTP_Request_d_setHeaderInitC(&r.m_setHeader, cb_setHeader);
    httpserver_HTTP_Request_d_setStatusInitC(&r.m_setStatus, cb_setStatus);
    httpserver_HTTP_Request_d_getHeaderInitC(&r.m_getHeader, cb_getHeader);
    httpserver_HTTP_Request_d_replyInitC(&r.m_reply, cb_reply);
    httpserver_HTTP_Request_d_sendFileInitC(&r.m_sendFile, cb_sendFile);
    httpserver_HTTP_Request_d_getVarInitC(&r.m_getVar, cb_getVar);
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

    return 1;
}

void* pollThread(void *ctx) {
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
        "allow_sendfile_call", this->enable_sendfile ? "true" : "false",
         NULL};
    struct mg_callbacks callbacks;
    struct mg_context *ctx;
    if (!mg_check_feature(8)) {
        corto_warning("StandaloneHTTP: IPv6 not supported");
    }

    if (!mg_check_feature(16)) {
        corto_warning("StandaloneHTTP: websockets not supported");
    }

    if (!mg_check_feature(2)) {
        corto_warning("StandaloneHTTP: SSL not supported");
    }

    if (!safe_httpserver_HTTP_set(httpserver_HTTP(this)->port, this)) {
        corto_seterr("port %d already occupied by other server",
            httpserver_HTTP(this)->port);
        goto error;
    }

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
    this->thread = (corto_word)corto_threadNew(
        pollThread,
        this);
    return 0;
error:
    return -1;
}

void httpserver_StandaloneHTTP_destruct(
    httpserver_StandaloneHTTP this)
{

    this->exiting = TRUE;
    
    mg_stop((struct mg_context*)this->server);
    corto_threadJoin((corto_thread)this->thread, NULL);
    corto_super_destruct(this);
}

void httpserver_StandaloneHTTP_write(
    httpserver_StandaloneHTTP this,
    httpserver_HTTP_Connection c,
    corto_string msg)
{
    mg_websocket_write((struct mg_connection *)c->conn, WEBSOCKET_OPCODE_TEXT, msg, strlen(msg));
}
