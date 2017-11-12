/* This is a managed file. Do not delete this comment. */

#include <corto/httpserver/httpserver.h>
int16_t httpserver_Files_construct(
    httpserver_Files this)
{
    if (!this->path) {
        corto_ptr_setstr(&this->path, ".");
    }

    return corto_super_construct(this);
}

int16_t httpserver_Files_onRequest(
    httpserver_Files this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    corto_string uri)
{
    char *request = uri;

    if (!uri || !strlen(uri) || !strcmp(uri, "/")) {
        request = "index.html";
    }

    char* file = corto_asprintf(
        "%s/%s",
        this->path,
        request);

    if (corto_file_test("%s", file)) {
        corto_trace("Files: serving '%s'", file);
        httpserver_HTTP_Request_sendfile(r, file);
    } else {
        corto_string msg = corto_asprintf("Resource '%s' not found", uri);
        httpserver_HTTP_Request_setStatus(r, 404);
        httpserver_HTTP_Request_reply(r, msg);
        corto_dealloc(msg);
    }

    corto_dealloc(file);
    return 1;
}

