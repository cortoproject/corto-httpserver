/* This is a managed file. Do not delete this comment. */

#include <corto/httpserver/httpserver.h>
static
corto_bool parseCookie(const char *header, const char *key, char * out, size_t *out_len)
{
    corto_bool found = FALSE;
    size_t size = strlen(key);
    out[0] = '\0';
    char *ptr = (char *)header;
    while (ptr && *ptr != '\0') {
        if (strncmp(ptr, key, size) == 0) {
            ptr = ptr + size;
            if ( *ptr == '=') {
                ptr++;
                size = strlen(ptr);
                char *end = strchr(ptr, ';');
                if (end) {
                    size = end - ptr;
                }

                if (size < *out_len) {
                    memcpy(out, ptr, size);
                    out[size] = '\0';
                } else {
                    *out_len = size + 1;
                }

                found = TRUE;
                ptr = NULL;
                break;
            }

        }

        ptr = strchr(ptr, ';');
        if (ptr) {
            ptr++;
            while (*ptr == ' '){
                ptr++;
            }

        }

    }

    return found;
}

void httpserver_HTTP_Request_badRequest(
    httpserver_HTTP_Request* this,
    const char *msg)
{
    httpserver_HTTP_Request_setStatus(this, 400);
    httpserver_HTTP_Request_setHeader(this, "Content-Type", "text/html; charset=UTF-8");
    httpserver_HTTP_Request_reply(this, msg);
}

corto_string httpserver_HTTP_Request_getCookie(
    httpserver_HTTP_Request* this,
    const char *key)
{
    size_t size = 0;
    char *result = NULL;
    char *header = httpserver_HTTP_Request_getHeader(this, "Cookie");

    if (parseCookie(header, key, result, &size)) {
        result = corto_alloc(size);
        corto_assert(parseCookie(header, key, result, &size), "cookie disappeared :(");
    }

    return result;
}

corto_string httpserver_HTTP_Request_getHeader(
    httpserver_HTTP_Request* this,
    const char *key)
{
    corto_string result;
    httpserver_HTTP_Request_d_getHeader__call(&this->m_getHeader, &result, this, key);
    return result;
}

corto_string httpserver_HTTP_Request_getVar(
    httpserver_HTTP_Request* this,
    const char *key)
{
    corto_string result = NULL;
    httpserver_HTTP_Request_d_getVar__call(&this->m_getVar, &result, this, key);
    return result;
}

void httpserver_HTTP_Request_reply(
    httpserver_HTTP_Request* this,
    const char *msg)
{
    httpserver_HTTP_Request_d_reply__call(&this->m_reply, this, msg);
}

void httpserver_HTTP_Request_sendfile(
    httpserver_HTTP_Request* this,
    const char *file)
{
    httpserver_HTTP_Request_d_sendFile__call(&this->m_sendFile, this, file);
}

void httpserver_HTTP_Request_setCookie(
    httpserver_HTTP_Request* this,
    const char *key,
    const char *value)
{
    char *cookie = corto_asprintf("%s=%s", key, value);
    httpserver_HTTP_Request_setHeader(this, "Set-Cookie", cookie);
    corto_dealloc(cookie);
}

void httpserver_HTTP_Request_setHeader(
    httpserver_HTTP_Request* this,
    const char *key,
    const char *val)
{
    httpserver_HTTP_Request_d_setHeader__call(&this->m_setHeader, this, key, val);
}

void httpserver_HTTP_Request_setStatus(
    httpserver_HTTP_Request* this,
    uint16_t status)
{
    this->status = status;
    httpserver_HTTP_Request_d_setStatus__call(&this->m_setStatus, this, status);
}

uint16_t httpserver_HTTP_Request_getStatus(
    httpserver_HTTP_Request* this)
{
    return this->status;
}
