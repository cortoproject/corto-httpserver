/* This is a managed file. Do not delete this comment. */

#include <corto.httpserver>

int16_t httpserver_StandaloneHTTPS_construct(
    httpserver_StandaloneHTTPS this)
{
    httpserver_StandaloneHTTP(this)->enable_ssl = 1;
    return httpserver_StandaloneHTTP_construct(httpserver_StandaloneHTTP(this));
}

void httpserver_StandaloneHTTPS_destruct(
    httpserver_StandaloneHTTPS this)
{
    httpserver_StandaloneHTTP_destruct(httpserver_StandaloneHTTP(this));
}
