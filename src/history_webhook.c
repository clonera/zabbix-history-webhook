
#include "common.h"
#include "zbxjson.h"
#include "dbcache.h"
#include "config.h"


#define CONTENT_TYPE_LEN 256

/* the variable keeps timeout setting for item processing */
static int	item_timeout = 0;
int MODULE_LOG_LEVEL = 0;

static ZBX_METRIC keys[] =
/*	KEY				FLAG		FUNCTION		TEST PARAMETERS */
{
	{NULL}
};

static char content_type[CONTENT_TYPE_LEN];

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_api_version                                           *
 *                                                                            *
 * Purpose: returns version number of the module interface                    *
 *                                                                            *
 * Return value: ZBX_MODULE_API_VERSION - version of module.h module is       *
 *               compiled with, in order to load module successfully Zabbix   *
 *               MUST be compiled with the same version of this header file   *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_api_version(void)
{
	return ZBX_MODULE_API_VERSION;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_timeout                                          *
 *                                                                            *
 * Purpose: set timeout value for processing of items                         *
 *                                                                            *
 * Parameters: timeout - timeout in seconds, 0 - no timeout set               *
 *                                                                            *
 ******************************************************************************/
void	zbx_module_item_timeout(int timeout)
{
	item_timeout = timeout;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_item_list                                             *
 *                                                                            *
 * Purpose: returns list of item keys supported by the module                 *
 *                                                                            *
 * Return value: list of item keys                                            *
 *                                                                            *
 ******************************************************************************/
ZBX_METRIC	*zbx_module_item_list(void)
{
	return keys;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_init                                                  *
 *                                                                            *
 * Purpose: the function is called on agent startup                           *
 *          It should be used to call any initialization routines             *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - module initialization failed               *
 *                                                                            *
 * Comment: the module won't be loaded in case of ZBX_MODULE_FAIL             *
 *                                                                            *
 ******************************************************************************/

int	zbx_module_init(void)
{
	/* Get parameters config file or environment */
	zbx_module_load_config();

	/* This will open the log for debugging */
	MODULE_LOG_LEVEL = (CONFIG_WEBHOOK_ENABLE_DEBUG ? LOG_LEVEL_INFORMATION : LOG_LEVEL_DEBUG);

	if(CONFIG_WEBHOOK_URL == NULL)
	{
		zabbix_log(LOG_LEVEL_ERR, "[%s] Webhook URL is not specified", MODULE_NAME);
        return ZBX_MODULE_FAIL;
	}

	zbx_snprintf(content_type, CONTENT_TYPE_LEN, "Content-Type: %s", CONFIG_WEBHOOK_CONTENT_TYPE);

	zabbix_log(LOG_LEVEL_INFORMATION, "[%s] History Webhook URL: %s", MODULE_NAME, CONFIG_WEBHOOK_URL);

	return ZBX_MODULE_OK;
}

/******************************************************************************
 *                                                                            *
 * Function: zbx_module_uninit                                                *
 *                                                                            *
 * Purpose: the function is called on agent shutdown                          *
 *          It should be used to cleanup used resources if there are any      *
 *                                                                            *
 * Return value: ZBX_MODULE_OK - success                                      *
 *               ZBX_MODULE_FAIL - function failed                            *
 *                                                                            *
 ******************************************************************************/
int	zbx_module_uninit(void)
{
	return ZBX_MODULE_OK;
}

/******************************************************************************
 *
 *	Function: curl_write_data
 *
 *	Purpose: For silencing the response output of libcurl
 *
 ******************************************************************************/
size_t curl_write_data(void *buffer, size_t size, size_t nmemb, void *userp)
{
   return size * nmemb;
}

/******************************************************************************
 *
 *	Function: call_webhook
 *
 *	Purpose: Sets up an http connection using curl and writes a pre-formatted
 *				string to a specified webhook url
 *
 ******************************************************************************/
static void call_webhook(char *payload) {
	CURL *curl;
	CURLcode res;
	struct curl_slist *headers=NULL;
	zabbix_log(MODULE_LOG_LEVEL, "[%s] calling webhook with payload: %s", MODULE_NAME, payload);
	curl_global_init(CURL_GLOBAL_ALL);

	curl = curl_easy_init();
	if(curl) 
	{
		headers = curl_slist_append(headers, content_type);
		curl_easy_setopt(curl, CURLOPT_URL, CONFIG_WEBHOOK_URL);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, CONFIG_WEBHOOK_SSL_INSECURE ? 0L : 1L);
		curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, CONFIG_WEBHOOK_SSL_INSECURE ? 0L : 1L);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDS, payload);
		curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_data);
		res = curl_easy_perform(curl);
		curl_slist_free_all(headers);

		if(res != CURLE_OK){
			zabbix_log(LOG_LEVEL_ERR, "[%s]     webhook call failed: %s", MODULE_NAME, curl_easy_strerror(res));
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	zabbix_log(MODULE_LOG_LEVEL, "[%s]     completed webhook call", MODULE_NAME);
}

/******************************************************************************
 *
 * Functions: send_history_batch
 *                                                                            *
 * Purpose: Prepares and sends a history batch to webhook                     *
 *                                                                            *
 * Parameters: item_type   - type of item                                     *
 *             history     - array of historical data                         *
 *             history_num - number of elements in history array              *
 *             start       - start from the history number                    *
 *             max_items   - max number of items in the batch                 *
 *                                                                            *
 ******************************************************************************/

#define ZBX_ITEM_FLOAT 1
#define ZBX_ITEM_INTEGER 2
#define ZBX_ITEM_STRING 3
#define ZBX_ITEM_TEXT 4
#define ZBX_ITEM_LOG 5

static void send_history_batch(const int item_type, const void *history, int history_num, int start, int max_items){
	int i;
	zbx_uint64_t itemid;
	zbx_uint64_t hostid;
	int		clock;
	int		ns;	
	DB_RESULT	result;
	DB_ROW		row;

	size_t		sql_offset = 0;
	static char		*sql = NULL;
	static size_t		sql_alloc = ZBX_KIBIBYTE;

	struct zbx_json		json;

	ZBX_HISTORY_FLOAT	*history_float = NULL;
	ZBX_HISTORY_INTEGER	*history_integer = NULL;
	ZBX_HISTORY_STRING	*history_string = NULL;
	ZBX_HISTORY_TEXT	*history_text = NULL;
	ZBX_HISTORY_LOG 	*history_log = NULL;

	switch(item_type)
	{
		case  ZBX_ITEM_FLOAT:
			history_float = (ZBX_HISTORY_FLOAT*)history;
			break;
		case  ZBX_ITEM_INTEGER:
			history_integer = (ZBX_HISTORY_INTEGER*)history;
			break;
		case  ZBX_ITEM_STRING:
			history_string = (ZBX_HISTORY_STRING*)history;
			break;
		case  ZBX_ITEM_TEXT:
			history_text = (ZBX_HISTORY_TEXT*)history;
			break;
		case  ZBX_ITEM_LOG:
			history_log = (ZBX_HISTORY_LOG*)history;
			break;
		default:
			THIS_SHOULD_NEVER_HAPPEN;
	}

	/* prepare json */

	zbx_json_init(&json, ZBX_JSON_STAT_BUF_LEN);

	if (CONFIG_WEBHOOK_ENABLE_BULK)
	{
		zbx_json_clean(&json);
		zbx_json_addarray(&json, CONFIG_WEBHOOK_BULK_TAG);
	}

	for(i = start; i < history_num && i < max_items; i++)
	{
		if (CONFIG_WEBHOOK_ENABLE_BULK)
		{
			zbx_json_addobject(&json, NULL);
			zbx_json_addobject(&json, CONFIG_WEBHOOK_ITEM_TAG);
		}
		else
		{
			zbx_json_clean(&json);
		}

        switch (item_type)
        {
            case ZBX_ITEM_FLOAT:
				itemid = history_float[i].itemid;
				clock = history_float[i].clock;
				ns = history_float[i].ns;
                break;
            case ZBX_ITEM_INTEGER:
				itemid = history_integer[i].itemid;
				clock = history_integer[i].clock;
				ns = history_integer[i].ns;
                break;
            case ZBX_ITEM_STRING:
				itemid = history_string[i].itemid;
				clock = history_string[i].clock;
				ns = history_string[i].ns;
                break;
            case ZBX_ITEM_TEXT:
				itemid = history_text[i].itemid;
				clock = history_text[i].clock;
				ns = history_text[i].ns;
                break;
            case ZBX_ITEM_LOG:
				itemid = history_log[i].itemid;
				clock = history_log[i].clock;
				ns = history_log[i].ns;
                break;
            default:
                THIS_SHOULD_NEVER_HAPPEN;
        }

		sql_offset = 0;
		zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset,
				"select i.name, i.key_, h.host, h.name, h.hostid"
				" from items i, hosts h"
				" where h.hostid = i.hostid and ");	

		DBadd_condition_alloc(&sql, &sql_alloc, &sql_offset, "i.itemid", &itemid, 1);
		result = DBselect("%s", sql);
		zbx_free(sql);

		if (NULL != (row = DBfetch(result)))
		{
			zbx_json_addstring(&json, ZBX_PROTO_TAG_NAME, row[0], ZBX_JSON_TYPE_STRING);
			zbx_json_addstring(&json, ZBX_PROTO_TAG_KEY, row[1], ZBX_JSON_TYPE_STRING);
			zbx_json_addstring(&json, ZBX_PROTO_TAG_HOST, row[3] ? row[3]: row[2], ZBX_JSON_TYPE_STRING);
			ZBX_STR2UINT64(hostid, row[4]);
		}
		DBfree_result(result);

		zbx_json_addarray(&json, ZBX_PROTO_TAG_GROUPS);

		sql_offset = 0;
		zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset,
					"select distinct hg.hostid,g.name"
					" from hstgrp g,hosts_groups hg"
					" where g.groupid=hg.groupid"
						" and");

		DBadd_condition_alloc(&sql, &sql_alloc, &sql_offset, "hg.hostid", &(hostid), 1);

		result = DBselect("%s", sql);
		zbx_free(sql);

		while (NULL != (row = DBfetch(result)))
		{
			zbx_json_addstring(&json, NULL, row[1], ZBX_JSON_TYPE_STRING);
		}
		DBfree_result(result);

		zbx_json_close(&json);

		zbx_json_addarray(&json, ZBX_PROTO_TAG_APPLICATIONS);

		sql_offset = 0;
		zbx_snprintf_alloc(&sql, &sql_alloc, &sql_offset,
				"select i.itemid,a.name"
				" from applications a,items_applications i"
				" where a.applicationid=i.applicationid"
					" and");	

		DBadd_condition_alloc(&sql, &sql_alloc, &sql_offset, "i.itemid", &itemid, 1);
		result = DBselect("%s", sql);
		zbx_free(sql);

		while (NULL != (row = DBfetch(result)))
		{
			zbx_json_addstring(&json, NULL, row[1], ZBX_JSON_TYPE_STRING);
		}
		DBfree_result(result);

		zbx_json_close(&json);

		zbx_json_adduint64(&json, ZBX_PROTO_TAG_ITEMID, itemid);
		zbx_json_addint64(&json, ZBX_PROTO_TAG_CLOCK, clock);
		zbx_json_addint64(&json, ZBX_PROTO_TAG_NS, ns);

        switch (item_type)
        {
            case ZBX_ITEM_FLOAT:
                zbx_json_addfloat(&json, ZBX_PROTO_TAG_VALUE, history_float[i].value);
                break;
            case ZBX_ITEM_INTEGER:
                zbx_json_adduint64(&json, ZBX_PROTO_TAG_VALUE, history_integer[i].value);
                break;
            case ZBX_ITEM_STRING:
                zbx_json_addstring(&json, ZBX_PROTO_TAG_VALUE, history_string[i].value, ZBX_JSON_TYPE_STRING);
                break;
            case ZBX_ITEM_TEXT:
                zbx_json_addstring(&json, ZBX_PROTO_TAG_VALUE, history_text[i].value, ZBX_JSON_TYPE_STRING);
                break;
            case ZBX_ITEM_LOG:
                zbx_json_addstring(&json, ZBX_PROTO_TAG_VALUE, history_log[i].value, ZBX_JSON_TYPE_STRING);
				zbx_json_addstring(&json, ZBX_PROTO_TAG_LOGSOURCE,
						ZBX_NULL2EMPTY_STR(history_log[i].source), ZBX_JSON_TYPE_STRING);
				zbx_json_addint64(&json, ZBX_PROTO_TAG_LOGSEVERITY, history_log[i].severity);
				zbx_json_addint64(&json, ZBX_PROTO_TAG_LOGEVENTID, history_log[i].logeventid);
				zbx_json_addstring(&json, ZBX_PROTO_TAG_VALUE, history_log[i].value,
						ZBX_JSON_TYPE_STRING);
                break;
            default:
                THIS_SHOULD_NEVER_HAPPEN;
        }

		if (CONFIG_WEBHOOK_ENABLE_BULK)
		{
			zbx_json_close(&json);
			zbx_json_close(&json);
		}
		else
		{
			call_webhook(json.buffer);
		}
	}

	if (CONFIG_WEBHOOK_ENABLE_BULK)
	{
		zbx_json_close(&json);
		call_webhook(json.buffer);
	}

	zbx_json_free(&json);
}

/******************************************************************************
 *
 * Functions: send_history
 *                                                                            *
 * Purpose: Prepares and sends history to webhook in batches                  *
 *                                                                            *
 * Parameters: item_type   - type of item                                     *
 *             history     - array of historical data                         *
 *             history_num - number of elements in history array              *
 *                                                                            *
 ******************************************************************************/
static void send_history(const int item_type, const void *history, int history_num){
	for(int i=0;i<history_num;i+=CONFIG_WEBHOOK_BULK_ITEM_COUNT)
	{
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Sending %d/%d", MODULE_NAME, i, history_num);
		send_history_batch(item_type, history, history_num, i, CONFIG_WEBHOOK_BULK_ITEM_COUNT);
	}
}


/******************************************************************************
 *
 * Functions: history_float_cb
 *            history_integer_cb                                        	  *
 *            history_string_cb                                         	  *
 *            history_text_cb                                           	  *
 *            history_log_cb                                            	  *
 *                                                                            *
 * Purpose: callback functions for storing historical data of types float,    *
 *          integer, string, text and log respectively in external storage    *
 *                                                                            *
 * Parameters: history     - array of historical data                         *
 *             history_num - number of elements in history array              *
 *                                                                            *
 ******************************************************************************/

static void	history_float_cb(const ZBX_HISTORY_FLOAT *history, int history_num)
{
	if (CONFIG_WEBHOOK_ENABLE_FLOAT) {
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Syncing %d float values", MODULE_NAME, history_num);
		send_history(ZBX_ITEM_FLOAT, (const void *) history, history_num);
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Finished syncing float values", MODULE_NAME);
	}
}


static void	history_integer_cb(const ZBX_HISTORY_INTEGER *history, int history_num)
{
	if (CONFIG_WEBHOOK_ENABLE_INTEGER) {
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Syncing %d int values", MODULE_NAME, history_num);
		send_history(ZBX_ITEM_INTEGER, (const void *) history, history_num);
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Finished syncing int values", MODULE_NAME);	
	}
}

static void	history_string_cb(const ZBX_HISTORY_STRING *history, int history_num)
{
	if (CONFIG_WEBHOOK_ENABLE_STRING) {
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Syncing %d string values", MODULE_NAME, history_num);
		send_history(ZBX_ITEM_STRING, (const void *) history, history_num);
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Finished syncing string values", MODULE_NAME);
	}
}

static void	history_text_cb(const ZBX_HISTORY_TEXT *history, int history_num)
{
	if (CONFIG_WEBHOOK_ENABLE_TEXT) {
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Syncing %d text values", MODULE_NAME, history_num);
		send_history(ZBX_ITEM_TEXT, (const void *) history, history_num);
		zabbix_log(MODULE_LOG_LEVEL, "[%s]     Finished syncing history values", MODULE_NAME);
	}
}
static void	history_log_cb(const ZBX_HISTORY_LOG *history, int history_num)
{
	if (CONFIG_WEBHOOK_ENABLE_LOG) {
		zabbix_log(MODULE_LOG_LEVEL, "[%s] Syncing %d log values", MODULE_NAME, history_num);
		send_history(ZBX_ITEM_LOG, (const void *) history, history_num);
		zabbix_log(MODULE_LOG_LEVEL, "[%s]     Finished syncing log values", MODULE_NAME);
	}
}



/******************************************************************************
 *                                                                            *
 * Function: zbx_module_history_write_cbs                                     *
 *                                                                            *
 * Purpose: returns a set of module functions Zabbix will call to export      *
 *          different types of historical data                                *
 *                                                                            *
 * Return value: structure with callback function pointers (can be NULL if    *
 *               module is not interested in data of certain types)           *
 *                                                                            *
 ******************************************************************************/
ZBX_HISTORY_WRITE_CBS	zbx_module_history_write_cbs(void)
{
	static ZBX_HISTORY_WRITE_CBS	callbacks =
	{
		history_float_cb,
		history_integer_cb,
		history_string_cb,
		history_text_cb,
		history_log_cb,
	};

	return callbacks;
}

