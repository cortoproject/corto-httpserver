/* This is a managed file. Do not delete this comment. */

#include <corto.httpserver>

#define MAX_COOKIE_KEY_LENGTH (256)

static
void httpserver_HTTP_Request_cookieGetVar(
    const char *header,
    const char *key,
    const char **out,
    size_t *out_len)
{
    const char *ptr;
    char buffer[MAX_COOKIE_KEY_LENGTH];
    char ch, *bptr = buffer;

    for (ptr = header; ptr && (ch = *ptr); ptr ++) {
        if (ch != '=') {
            *bptr = ch;
            bptr ++;
        } else {
            *bptr = '\0';
            if (!strcmp(buffer, key)) {
                *out = ptr + 1;
                char *end = strchr(ptr + 1, ';');
                if (end) {
                    *out_len = end - ptr;
                } else {
                    *out_len = strlen(*out);
                }
                break;
            }
            ptr = strchr(ptr + 1, ';');
        }
    }
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
    const char *value = NULL;
    char *result = NULL,
         *header = httpserver_HTTP_Request_getHeader(this, "Cookie");

    httpserver_HTTP_Request_cookieGetVar(header, key, &value, &size);

    if (value) {
        result = corto_alloc(size + 1);
        strncpy(result, value, size);
        result[size] = '\0';
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
    char *cookie = ut_asprintf("%s=%s", key, value);
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
