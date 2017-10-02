#include ".\mqtt\MQTTAsync.h"

#define MAX_STRING 64

typedef struct 
{
	char server[MAX_STRING];
	char username[MAX_STRING];
	char password[MAX_STRING];
	char interval[8];
} NET_SETTING;

class CMQTTClient
{
public:
	CMQTTClient();
	~CMQTTClient();
	
	int AddTopic(int ntopic, char* topic_name);
	int RemoveTopic(int ntopic);
	int SendTopic(int ntopic, char *paiload);
	char* GetTopic(int ntopic);
	int isConnected(int ntopic);
	int GetSwitch();

	NET_SETTING m_Setting;
};

