#include "status.h"

// The texts come from the config: a project's, or the bridge's own
const char *const status_text[] = {
    [STATUS_OK]             = STATUS_TEXT_OK,
    [STATUS_CON_ERR_WIFI]   = STATUS_TEXT_CON_ERR_WIFI,
    [STATUS_CON_ERR_BROKER] = STATUS_TEXT_CON_ERR_BROKER,
    [STATUS_ERR_BROKER]     = STATUS_TEXT_ERR_BROKER,
    [STATUS_ANS_ERR_LONG]   = STATUS_TEXT_ANS_ERR_LONG,
};
