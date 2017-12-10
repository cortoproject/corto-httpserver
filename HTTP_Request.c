/* This is a managed file. Do not delete this comment. */

#include <corto/httpserver/httpserver.h>

void httpserver_HTTP_Request_badRequest(
    httpserver_HTTP_Request* this,
    corto_string msg)
{
    /* Insert implementation */
}

corto_string httpserver_HTTP_Request_getCookie(
    httpserver_HTTP_Request* this,
    corto_string key)
{
    /* Insert implementation */
}

corto_string httpserver_HTTP_Request_getHeader(
    httpserver_HTTP_Request* this,
    corto_string key)
{
    /* Insert implementation */
}

uint16_t httpserver_HTTP_Request_getStatus(
    httpserver_HTTP_Request* this)
{
    /* Insert implementation */
}

corto_string httpserver_HTTP_Request_getVar(
    httpserver_HTTP_Request* this,
    corto_string key)
{
    /* Insert implementation */
}

void httpserver_HTTP_Request_reply(
    httpserver_HTTP_Request* this,
    corto_string msg)
{
    /* Insert implementation */
}

void httpserver_HTTP_Request_sendfile(
    httpserver_HTTP_Request* this,
    corto_string file)
{
    /* Insert implementation */
}

void httpserver_HTTP_Request_setCookie(
    httpserver_HTTP_Request* this,
    corto_string key,
    corto_string value)
{
    /* Insert implementation */
}

void httpserver_HTTP_Request_setHeader(
    httpserver_HTTP_Request* this,
    corto_string key,
    corto_string val)
{
    /* Insert implementation */
}

void httpserver_HTTP_Request_setStatus(
    httpserver_HTTP_Request* this,
    uint16_t status)
{
    /* Insert implementation */
}

