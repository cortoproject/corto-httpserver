/* This is a managed file. Do not delete this comment. */

#include <corto.httpserver>

int16_t httpserver_Files_construct(
    httpserver_Files this)
{
    if (!this->path) {
        corto_set_str(&this->path, ".");
    }

    /* Redirect requests to http://endpoint to http://endpoint/ */
    httpserver_Service(this)->redirectEndpointToPath = true;
    return corto_super_construct(this);
}

int16_t httpserver_Files_on_request(
    httpserver_Files this,
    httpserver_HTTP_Connection c,
    httpserver_HTTP_Request *r,
    const char *uri)
{
    const char *request = uri;
    int16_t result = 1;

    if (!uri || !strlen(uri) || !strcmp(uri, "/")) {
        request = "index.html";
    }

    char* file = ut_asprintf(
        "%s/%s",
        this->path,
        request);

    if (ut_file_test("%s", file)) {
        ut_trace("Files: serving '%s'", file);
        httpserver_HTTP_Request_sendfile(r, file);
    } else {
        corto_string msg = ut_asprintf("Resource '%s' not found", uri);
        httpserver_HTTP_Request_setStatus(r, 404);
        httpserver_HTTP_Request_reply(r, msg);
        corto_dealloc(msg);
        result = 0;
    }

    corto_dealloc(file);
    return result;
}
