#ifndef ZABBIX_HISTORY_WEBHOOK_CONFIG_H
#define ZABBIX_HISTORY_WEBHOOK_CONFIG_H

#include "sysinc.h"
#include "module.h"
#include "common.h"
#include "log.h"
#include "cfg.h"

#define MODULE_NAME "history_webhook.so"
#define MODULE_LOCAL_CONFIG_FILE_NAME "history_webhook_local.conf"
#define MODULE_CONFIG_FILE_NAME "history_webhook.conf"

extern char *CONFIG_LOAD_MODULE_PATH;

extern void zbx_module_load_config(void);
extern void zbx_module_set_defaults(void);

extern char *CONFIG_WEBHOOK_URL;
extern char *CONFIG_WEBHOOK_CONTENT_TYPE;
extern int CONFIG_WEBHOOK_SSL_INSECURE;
extern int CONFIG_WEBHOOK_ENABLE_BULK;
extern int CONFIG_WEBHOOK_BULK_ITEM_COUNT;
extern char *CONFIG_WEBHOOK_BULK_TAG;
extern char *CONFIG_WEBHOOK_ITEM_TAG;

extern int CONFIG_WEBHOOK_ENABLE_DEBUG;

extern int CONFIG_WEBHOOK_ENABLE_FLOAT;
extern int CONFIG_WEBHOOK_ENABLE_INTEGER;
extern int CONFIG_WEBHOOK_ENABLE_STRING;
extern int CONFIG_WEBHOOK_ENABLE_TEXT;
extern int CONFIG_WEBHOOK_ENABLE_LOG;


#endif //ZABBIX_HISTORY_WEBHOOK_CONFIG_H
