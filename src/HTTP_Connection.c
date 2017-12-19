/* This is a managed file. Do not delete this comment. */

#include <corto/httpserver/httpserver.h>
void httpserver_HTTP_Connection_write(
    httpserver_HTTP_Connection this,
    const char *msg)
{

    httpserver_HTTP_write(this->server, this, msg);

}

