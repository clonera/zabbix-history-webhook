history_webhook: ./src/history_webhook.c
	gcc -fPIC -Wall -shared -lcurl -o dist/history_webhook.so ./src/*.c -I../../../include 
