/* httpserver.h
 * This is the main package file. Include this file in other projects.
 * Only modify inside the header-end and body-end sections.
 */

#ifndef CORTO_HTTPSERVER_H
#define CORTO_HTTPSERVER_H

#include "bake_config.h"

#define CORTO_HTTPSERVER_ETC ut_locate("corto.httpserver", NULL, UT_LOCATE_ETC)

/* $header() */
/* Definitions that are required by package headers (native types) go here. */
/* $end */

#include "_type.h"
#include "_interface.h"
#include "_load.h"
#include "_binding.h"

#include <corto.httpserver.c>

/* $body() */
/* Definitions here that need your package headers go here. */
/* $end */

#endif

