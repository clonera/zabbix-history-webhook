# Zabbix module to export history to webhook


## Features

- Formats and writes Zabbix history to a webhook in zabbix real-time export protocol ( https://www.zabbix.com/documentation/current/manual/appendix/protocols/real_time_export) (just adds an additional tag: item key)
- Full support for float, integer, string, text and log 
- Content type can specified (default: application/json)
- SSL verification errors can be ignored
- Ability to call webhook per measurement or multiple measurements (bulk mode)
- Possibility to use custom tags while exporting in bulk mode 
- Environment variables support


# Building

Get the Zabbix source code for the intended version from https://www.zabbix.com/download_sources

```
wget https://cdn.zabbix.com/zabbix/sources/stable/5.0/zabbix-5.0.0.tar.gz
```

Uncompress the Zabbix source code.

```
tar -zxvf zabbix-5.0.0.tar.gz
```

Configure before compiling 

```
cd zabbix-5.0.0/
./configure
```

Copy this folder to src/modules

```
cp -R zabbix-history-webhook/ src/modules
```

Install dependencies

```
apt install gcc libcurl4-openssl-dev
```

Build the module

```
cd src/modules/zabbix-history-webhook/ 
make
```


# Installation

Copy history_webhook.so and history_webhook.conf to the directory that is specified as LoadModulePath in zabbix_server.conf


```
cp dist/history_webhook.so /usr/lib/zabbix/modules
cp dist/history_webhook.conf /usr/lib/zabbix/modules
```

Edit config file and specify the WebhookUrl

Alternatively you can use environment variables such as ZBX_WEBHOOK_URL, ZBX_WEBHOOK_CONTENT_TYPE etc.
Environment variable support is enabled if ZBX_WEBHOOK_URL environment variable exists


Edit zabbix_server.conf and your module for loading:

```
LoadModule=history_influxdb.so
```


Restart Zabbix server

```
systemctl restart zabbix-server
```

Check zabbix_server.log to see if the module is working properly.

```
  loaded modules: history_webhook.so
```


# Background

We needed to stream history to Apache Druid for obvious reasons and decided to use Apache Kafka. After spending some time to fix non-working kafka history module on github, we realized that it is not possible to get librdkafka working as module (it cannot send to kafka from callback).

After some investigation we decided to use a rest-to-kafka proxy such as Kafka-Pixy (https://github.com/mailgun/kafka-pixy) or Kafka Rest Proxy (https://github.com/confluentinc/kafka-rest). 

This module is designed but not limited to send to these proxies. It can feed history to a URL with some options. 


Some parts of this module is inspired from influxdb history module:
- https://github.com/LMacPhail/zabbix-history-influxdb


Non-working kafka history module:
- https://github.com/mosen/zabbix-history-kafka


[Contributors](./AUTHORS.md)
