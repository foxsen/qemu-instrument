#include "pin_state.h"
#include <gmodule.h>
#include "qapi/error.h"


typedef int (*pintool_install_func_t)(int argc, char* argv[]);

static int load_pintool(const char *optarg, Error **errp)
{
    pintool_install_func_t install;
    const char *path = optarg;
    GModule *handle;
    gpointer sym;
    int rc;

    handle = g_module_open(path, G_MODULE_BIND_LOCAL);
    if (handle == NULL) {
        error_setg(errp, "Could not load pintool %s: %s", path, g_module_error());
        goto err_dlopen;
    }

    if (!g_module_symbol(handle, "main", &sym)) {
        error_setg(errp, "Could not load pintool %s: %s", path, g_module_error());
        goto err_symbol;
    }
    install = (pintool_install_func_t) sym;
    /* symbol was found; it could be NULL though */
    if (install == NULL) {
        error_setg(errp, "Could not load pintool %s: pintool_install is NULL",
                   path);
        goto err_symbol;
    }

    int argc = 0;
    char **argv = NULL;


    /* qemu_rec_mutex_lock(&plugin.lock); */

    rc = install(argc, argv);
    if (rc) {
        error_setg(errp, "Could not load pintool %s: qemu_pintool_install returned error code %d",
                   path, rc);
        /*
         * we cannot rely on the pintool doing its own cleanup, so
         * call a full uninstall if the pintool did not yet call it.
         */
        /* if (!ctx->uninstalling) { */
        /*     pintool_reset_uninstall(ctx->id, NULL, false); */
        /* } */
    }

    /* qemu_rec_mutex_unlock(&plugin.lock); */
    return rc;

 err_symbol:
    g_module_close(handle);
 err_dlopen:
    return 1;
}

void qemu_pintool_opt_parse(const char *optarg, Error **errp)
{
    load_pintool(optarg, errp);
}

