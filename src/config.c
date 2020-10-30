#include "config.h"

char *CONFIG_WEBHOOK_URL = NULL;
char *CONFIG_WEBHOOK_CONTENT_TYPE = NULL;
int CONFIG_WEBHOOK_SSL_INSECURE = 0;
int CONFIG_WEBHOOK_ENABLE_BULK = 1;
int CONFIG_WEBHOOK_BULK_ITEM_COUNT = 500;
char *CONFIG_WEBHOOK_BULK_TAG = NULL;
int CONFIG_WEBHOOK_ENABLE_ITEM_TAG = 0;
char *CONFIG_WEBHOOK_ITEM_TAG = NULL;
int CONFIG_WEBHOOK_ENABLE_DEBUG = 0;

int CONFIG_WEBHOOK_ENABLE_FLOAT = 1;
int CONFIG_WEBHOOK_ENABLE_INTEGER = 1;
int CONFIG_WEBHOOK_ENABLE_STRING = 0;
int CONFIG_WEBHOOK_ENABLE_TEXT = 0;
int CONFIG_WEBHOOK_ENABLE_LOG = 0;

void zbx_module_load_config_file(void)
{
        char *MODULE_CONFIG_FILE = NULL;
        char *MODULE_LOCAL_CONFIG_FILE = NULL;

        static struct cfg_line module_cfg[] =
            {
                /* PARAMETER,			VAR,				TYPE,
                            MANDATORY,		MIN,		MAX */
                {"WebhookUrl", &CONFIG_WEBHOOK_URL, TYPE_STRING,
                 PARM_MAND, 0, 0},
                {"WebhookContentType", &CONFIG_WEBHOOK_CONTENT_TYPE, TYPE_STRING,
                 PARM_OPT, 0, 0},
                {"WebhookSslInsecure", &CONFIG_WEBHOOK_SSL_INSECURE, TYPE_INT,
                 PARM_OPT, 0, 1},
                {"WebhookEnableBulk", &CONFIG_WEBHOOK_ENABLE_BULK, TYPE_INT,
                 PARM_OPT, 0, 1},
                {"WebhookBulkItemCount", &CONFIG_WEBHOOK_BULK_ITEM_COUNT, TYPE_INT,
                 PARM_OPT, 0, 1000},
                {"WebhookBulkTag", &CONFIG_WEBHOOK_BULK_TAG, TYPE_STRING,
                 PARM_OPT, 0, 0},
                {"WebhookItemTag", &CONFIG_WEBHOOK_ITEM_TAG, TYPE_STRING,
                 PARM_OPT, 0, 0},
                {"WebhookEnableDebug", &CONFIG_WEBHOOK_ENABLE_DEBUG, TYPE_INT,
                 PARM_OPT, 0, 1},
                {"WebhookEnableFloat", &CONFIG_WEBHOOK_ENABLE_FLOAT, TYPE_INT,
                 PARM_OPT, 0, 1},
                {"WebhookEnableInteger", &CONFIG_WEBHOOK_ENABLE_INTEGER, TYPE_INT,
                 PARM_OPT, 0, 1},
                {"WebhookEnableString", &CONFIG_WEBHOOK_ENABLE_STRING, TYPE_INT,
                 PARM_OPT, 0, 1},
                {"WebhookEnableText", &CONFIG_WEBHOOK_ENABLE_TEXT, TYPE_INT,
                 PARM_OPT, 0, 1},
                {"WebhookEnableLog", &CONFIG_WEBHOOK_ENABLE_LOG, TYPE_INT,
                 PARM_OPT, 0, 1},
                {NULL}};

        MODULE_CONFIG_FILE = zbx_dsprintf(MODULE_CONFIG_FILE, "%s/%s", CONFIG_LOAD_MODULE_PATH, MODULE_CONFIG_FILE_NAME);
        MODULE_LOCAL_CONFIG_FILE = zbx_dsprintf(MODULE_LOCAL_CONFIG_FILE, "%s/%s", CONFIG_LOAD_MODULE_PATH, MODULE_LOCAL_CONFIG_FILE_NAME);

        zabbix_log(LOG_LEVEL_DEBUG, "[%s] load conf:%s", MODULE_NAME, MODULE_CONFIG_FILE);
        zabbix_log(LOG_LEVEL_DEBUG, "[%s] load local conf:%s", MODULE_NAME, MODULE_LOCAL_CONFIG_FILE);

        // load main config file
        parse_cfg_file(MODULE_CONFIG_FILE, module_cfg, ZBX_CFG_FILE_REQUIRED, ZBX_CFG_STRICT);

        // load local config file if present
        parse_cfg_file(MODULE_LOCAL_CONFIG_FILE, module_cfg, ZBX_CFG_FILE_OPTIONAL, ZBX_CFG_STRICT);

        zbx_free(MODULE_CONFIG_FILE);
        zbx_free(MODULE_LOCAL_CONFIG_FILE);

        zbx_module_set_defaults();
}

void zbx_module_load_config_env(void)
{
        if(getenv("ZBX_WEBHOOK_URL"))
        {
                CONFIG_WEBHOOK_URL = zbx_strdup(CONFIG_WEBHOOK_URL, getenv("ZBX_WEBHOOK_URL"));
        }
        if(getenv("ZBX_WEBHOOK_CONTENT_TYPE"))
        {
                CONFIG_WEBHOOK_CONTENT_TYPE = zbx_strdup(CONFIG_WEBHOOK_CONTENT_TYPE, getenv("ZBX_WEBHOOK_CONTENT_TYPE"));
        }
        if(getenv("ZBX_WEBHOOK_SSL_INSECURE"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_SSL_INSECURE, getenv("ZBX_WEBHOOK_SSL_INSECURE"));
        }
        if(getenv("ZBX_WEBHOOK_ENABLE_BULK"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_ENABLE_BULK, getenv("ZBX_WEBHOOK_ENABLE_BULK"));
        }
        if(getenv("ZBX_WEBHOOK_BULK_ITEM_COUNT"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_BULK_ITEM_COUNT, getenv("ZBX_WEBHOOK_BULK_ITEM_COUNT"));
        }
        if(getenv("ZBX_WEBHOOK_BULK_TAG"))
        {
                CONFIG_WEBHOOK_BULK_TAG = zbx_strdup(CONFIG_WEBHOOK_BULK_TAG, getenv("ZBX_WEBHOOK_BULK_TAG"));
        }
        if(getenv("ZBX_WEBHOOK_ITEM_TAG"))
        {
                CONFIG_WEBHOOK_ITEM_TAG = zbx_strdup(CONFIG_WEBHOOK_ITEM_TAG, getenv("ZBX_WEBHOOK_ITEM_TAG"));
        }
        if(getenv("ZBX_WEBHOOK_ENABLE_DEBUG"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_ENABLE_DEBUG, getenv("ZBX_WEBHOOK_ENABLE_DEBUG"));
        }
        if(getenv("ZBX_WEBHOOK_ENABLE_FLOAT"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_ENABLE_FLOAT, getenv("ZBX_WEBHOOK_ENABLE_FLOAT"));
        }
        if(getenv("ZBX_WEBHOOK_ENABLE_INTEGER"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_ENABLE_INTEGER, getenv("ZBX_WEBHOOK_ENABLE_INTEGER"));
        }
        if(getenv("ZBX_WEBHOOK_ENABLE_STRING"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_ENABLE_STRING, getenv("ZBX_WEBHOOK_ENABLE_STRING"));
        }
        if(getenv("ZBX_WEBHOOK_ENABLE_TEXT"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_ENABLE_TEXT, getenv("ZBX_WEBHOOK_ENABLE_TEXT"));
        }
        if(getenv("ZBX_WEBHOOK_ENABLE_LOG"))
        {
                ZBX_STR2UINT64(CONFIG_WEBHOOK_ENABLE_LOG, getenv("ZBX_WEBHOOK_ENABLE_LOG"));
        }
        
        zbx_module_set_defaults();
}

void zbx_module_load_config(void)
{
        if(getenv("ZBX_WEBHOOK_URL"))
        {
                /* load from environment */
                zbx_module_load_config_env();
        }
        else
        {
                /* load from config file */
                zbx_module_load_config_file();
        }

}

void zbx_module_set_defaults(void)
{
        if (NULL == CONFIG_WEBHOOK_CONTENT_TYPE)
        {
                CONFIG_WEBHOOK_CONTENT_TYPE = zbx_strdup(CONFIG_WEBHOOK_CONTENT_TYPE, "application/json");
        }
        if (NULL == CONFIG_WEBHOOK_BULK_TAG)
        {
                CONFIG_WEBHOOK_BULK_TAG = zbx_strdup(CONFIG_WEBHOOK_BULK_TAG, "records");
        }
        if (NULL == CONFIG_WEBHOOK_ITEM_TAG)
        {
                CONFIG_WEBHOOK_ITEM_TAG = zbx_strdup(CONFIG_WEBHOOK_ITEM_TAG, "value");
        }
}
