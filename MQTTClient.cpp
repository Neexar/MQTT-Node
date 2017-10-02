#include "stdafx.h"
#include "MQTTClient.h"

#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <time.h>
#include <sys/timeb.h>
#include <windows.h>

#define MAX_TOPICS 10
#define LOGA_DEBUG 0
#define LOGA_INFO 1

struct Options
{
	char connection[MAX_STRING];         /**< connection to system under test. */
	int verbose;
	int test_no;
	int size;									/**< size of big message */
	int MQTTVersion;
	int iterations;
} options =
{
	"m20.cloudmqtt.com:14088",
	0,
	-1,
	10000,
	MQTTVERSION_DEFAULT,
	1,
};

typedef struct
{
	MQTTAsync c;
	int index;
	char clientid[24];
	char test_topic[100];
	int	message_count;
} client_data;

MQTTAsync_connectOptions opts = MQTTAsync_connectOptions_initializer;
MQTTAsync_willOptions wopts = MQTTAsync_willOptions_initializer;
client_data clientdata[MAX_TOPICS];

char* test_topic = "async test topic";

volatile int m_switch = 0;

CMQTTClient::CMQTTClient()
{
	strcpy_s(m_Setting.server, "server:1883");
	strcpy_s(m_Setting.username, "username");
	strcpy_s(m_Setting.password, "password");
	strcpy_s(m_Setting.interval, "1");
}

CMQTTClient::~CMQTTClient()
{
}

/*********************************************************************
Callback client function
*********************************************************************/

void onDisconnect(void* context, MQTTAsync_successData* response)
{
	client_data* cd = (client_data*)context;
}

void onPublish(void* context, MQTTAsync_successData* response)
{
	client_data* cd = (client_data*)context;
}

void onUnsubscribe(void* context, MQTTAsync_successData* response)
{
	client_data* cd = (client_data*)context;
	MQTTAsync_disconnectOptions opts = MQTTAsync_disconnectOptions_initializer;
	int rc;

	opts.onSuccess = onDisconnect;
	opts.context = cd;

	rc = MQTTAsync_disconnect(cd->c, &opts);
}

int messageArrived(void* context, char* topicName, int topicLen, MQTTAsync_message* message)
{
	char* payloadptr;

	if(strstr(topicName, "switch") != NULL)
	{
		payloadptr = (char*)message->payload;
		m_switch = payloadptr[0] - 48;
	}
	
	MQTTAsync_freeMessage(&message);
	MQTTAsync_free(topicName);

	return 1;
}

void onSubscribe(void* context, MQTTAsync_successData* response)
{
	client_data* cd = (client_data*)context;
}

void onConnect(void* context, MQTTAsync_successData* response)
{
	client_data* cd = (client_data*)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
	
	int rc;

	opts.onSuccess = onSubscribe;
	opts.context = cd;

	rc = MQTTAsync_subscribe(cd->c, cd->test_topic, 2, &opts);
}

void onFailure(void* context, MQTTAsync_failureData* response)
{
	client_data* cd = (client_data*)context;
	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;
}

/*********************************************************************
Basic connect, subscribe send and receive.
*********************************************************************/

int CMQTTClient::AddTopic(int ntopic, char* topic_name)
{
	if (ntopic < 0 || ntopic >= MAX_TOPICS) return -1;

	int rc = 0;
	
	sprintf_s(clientdata[ntopic].clientid, "stal%d", ntopic);
	strcpy_s(clientdata[ntopic].test_topic, topic_name);
	clientdata[ntopic].index = ntopic;
	clientdata[ntopic].message_count = 0;

	rc = MQTTAsync_create(&(clientdata[ntopic].c), m_Setting.server, clientdata[ntopic].clientid,
		MQTTCLIENT_PERSISTENCE_NONE, NULL);

	rc = MQTTAsync_setCallbacks(clientdata[ntopic].c, &clientdata[ntopic], NULL, messageArrived, NULL);

	opts.keepAliveInterval = 20;
	opts.cleansession = 1;
	opts.username = m_Setting.username;
	opts.password = m_Setting.password;
	opts.MQTTVersion = options.MQTTVersion;

	opts.will = &wopts;
	opts.will->message = NULL;
	opts.will->qos = 1;
	opts.will->retained = 0;
	opts.will->topicName = "lwt_topic";
	opts.onSuccess = onConnect;
	opts.onFailure = onFailure;
	opts.context = &clientdata[ntopic];

	rc = MQTTAsync_connect(clientdata[ntopic].c, &opts);

	return rc;
}

int CMQTTClient::RemoveTopic(int ntopic)
{
	if (ntopic < 0 || ntopic >= MAX_TOPICS) return -1;

	client_data* cd = &clientdata[ntopic];
	int rc = 0;

	MQTTAsync_responseOptions opts = MQTTAsync_responseOptions_initializer;

	opts.onSuccess = onUnsubscribe;
	opts.context = cd;
	rc = MQTTAsync_unsubscribe(cd->c, cd->test_topic, &opts); 

	MQTTAsync_destroy(&clientdata[ntopic].c);

	return rc;
}

int CMQTTClient::SendTopic(int ntopic, char *paiload)
{
	client_data* cd = (client_data*)&clientdata[ntopic];
	MQTTAsync_message pubmsg = MQTTAsync_message_initializer;
	int rc;

	pubmsg.payload = paiload;
	pubmsg.payloadlen = strnlen_s(paiload, MAX_STRING);
	pubmsg.qos = 0;
	pubmsg.retained = 0;

	rc = MQTTAsync_send(cd->c, cd->test_topic, pubmsg.payloadlen, pubmsg.payload, pubmsg.qos, pubmsg.retained, NULL);

	return rc;
}

char* CMQTTClient::GetTopic(int ntopic)
{
	client_data* cd = &clientdata[ntopic];

	return cd->test_topic;
}

int CMQTTClient::isConnected(int ntopic)
{
	client_data* cd = &clientdata[ntopic];

	return MQTTAsync_isConnected(cd->c);
}

int CMQTTClient::GetSwitch()
{
	return m_switch;
}
