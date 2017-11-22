/* tcp_perf Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <string.h>
#include <sys/socket.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event_loop.h"
#include "esp_log.h"

#include "tcp_perf.h"
#include "CAN.h"

/* FreeRTOS event group to signal when we are connected to wifi */
EventGroupHandle_t tcp_event_group;

/*socket*/
static int server_socket = 0;
static struct sockaddr_in server_addr;
static struct sockaddr_in client_addr;
static unsigned int socklen = sizeof(client_addr);
int connect_socket = 0;

int total_data = 0;

#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO

int total_pack = 0;
int send_success = 0;
int send_fail = 0;
int delay_classify[5] = {0};

#endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/

static esp_err_t event_handler(void *ctx, system_event_t *event)
{
	switch (event->event_id)
	{
	case SYSTEM_EVENT_STA_START:
		esp_wifi_connect();
		break;
	case SYSTEM_EVENT_STA_DISCONNECTED:
		esp_wifi_connect();
		xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_STA_CONNECTED:
		break;
	case SYSTEM_EVENT_STA_GOT_IP:
		//ESP_LOGI(TAG, "got ip:%s\n",
		//ip4addr_ntoa(&event->event_info.got_ip.ip_info.ip));
		xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STACONNECTED:
		ESP_LOGI(TAG, "station:" MACSTR " join,AID=%d\n",
				 MAC2STR(event->event_info.sta_connected.mac),
				 event->event_info.sta_connected.aid);
		xEventGroupSetBits(tcp_event_group, WIFI_CONNECTED_BIT);
		break;
	case SYSTEM_EVENT_AP_STADISCONNECTED:
		ESP_LOGI(TAG, "station:" MACSTR "leave,AID=%d\n",
				 MAC2STR(event->event_info.sta_disconnected.mac),
				 event->event_info.sta_disconnected.aid);
		xEventGroupClearBits(tcp_event_group, WIFI_CONNECTED_BIT);

		// close connected socket because station has been disconnected.
		if (connect_socket > 0)
			close(connect_socket);
		break;
	default:
		break;
	}
	return ESP_OK;
}

int findFirstCharInByteArray(const char *data, size_t len, int ch)
{
	int index = -1;
	//void* memchr( const void* ptr, int ch, size_t count );
	char *ptr = memchr(data, ch, len);
	printf("ptr:%p \n", ptr);
	if (ptr != NULL)
	{
		// search for the corresponding index in array
		for (int i = 0; i < len; i++)
		{
			if (&data[i] == ptr)
			{
				printf("index:%d \n", i);
				index = i;
				break;
			}
		}
	}
	return index;
}

uint8_t MSGID_LEN = 4;

//receive data
void recv_data(void *pvParameters)
{
	CAN_frame_t can_frame;

	int len = 0;
	//char databuff[EXAMPLE_DEFAULT_PKTSIZE];
	char databuff[50];
	//char recv_frame[50];

	while ((len = recv(connect_socket, databuff, 50, 0)) > 0)
	{
		//printf("len:%d \n", len);

		// for (int i = 0; i < len; i++)
		// {
		// 	printf(" %02x", databuff[i]);
		// }
		// printf("\n");

		memcpy(&can_frame.MsgID, databuff, MSGID_LEN);
		//printf("msgID: %04x", can_frame.MsgID);

		uint32_t dataLen = len - MSGID_LEN;
		//printf("dataLen: %d", dataLen);
		can_frame.FIR.B.DLC = dataLen;

//		memcpy(&can_frame.data, &databuff[MSGID_LEN], dataLen);
//		 for (int i = 0; i < dataLen; i++)
//		 {
//		 	printf(" %02x", can_frame.data.u8[i]);
//		 }
//		 printf("\n");

		can_frame.FIR.B.RTR = 0;
		can_frame.FIR.B.FF = 0;

		// for (int i = MSGID_LEN, j = 0; i < dataLen; i++, j++)
		// {
		// 	memcpy(&can_frame.data.u8[j], &databuff[i], 1);
		// }
		CAN_write_frame(&can_frame);

//		//test: loop back
//		if ((len = send(connect_socket, databuff, len, 0)) <= 0)
//		{
//			if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG)
//			{
//				show_socket_error_reason(connect_socket);
//			}
//			printf("sendErr:%d\n", len);
//		}

	}

	if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG)
	{
		show_socket_error_reason(connect_socket);
	}

	close(connect_socket);
	ESP_LOGI(TAG, "tcp connection closed!");
}

//use this esp32 as a tcp server. return ESP_OK:success ESP_FAIL:error
esp_err_t create_tcp_server()
{
	int socket_ret;

	ESP_LOGI(TAG, "server socket....port=%d\n", EXAMPLE_DEFAULT_PORT);
	server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket < 0)
	{
		show_socket_error_reason(server_socket);
		socket_ret = ESP_FAIL;
	}
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(EXAMPLE_DEFAULT_PORT);
	server_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(server_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
	{
		show_socket_error_reason(server_socket);
		close(server_socket);
		socket_ret = ESP_FAIL;
	}
	if (listen(server_socket, 5) < 0)
	{
		show_socket_error_reason(server_socket);
		close(server_socket);
		socket_ret = ESP_FAIL;
	}

	while(1)
	{
		ESP_LOGI(TAG, "befor accept!");
		connect_socket = accept(server_socket, (struct sockaddr *)&client_addr, &socklen);
		ESP_LOGI(TAG, "after accept!");
		if (connect_socket < 0)
		{
			show_socket_error_reason(connect_socket);
			close(server_socket);
			socket_ret = ESP_FAIL;
			goto FAIL;
		}
		else
		{
			/*connection established，now can send/recv*/
			ESP_LOGI(TAG, "tcp connection established!");
			recv_data(NULL);
			socket_ret = ESP_OK;
		}
	}

FAIL:	if (socket_ret == ESP_FAIL)
	{
		ESP_LOGI(TAG, "create tcp socket error,stop.");
		//vTaskDelete(NULL);
	}
	return socket_ret;
}


//wifi_init_softap
void wifi_init_softap()
{
	tcp_event_group = xEventGroupCreate();

	tcpip_adapter_init();
	ESP_ERROR_CHECK(esp_event_loop_init(event_handler, NULL));

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));
	wifi_config_t wifi_config = {
		.ap = {
			.ssid = EXAMPLE_DEFAULT_SSID,
			.ssid_len = 0,
			.max_connection = EXAMPLE_MAX_STA_CONN,
			.password = EXAMPLE_DEFAULT_PWD,
			.authmode = WIFI_AUTH_WPA_WPA2_PSK},
	};
	if (strlen(EXAMPLE_DEFAULT_PWD) == 0)
	{
		wifi_config.ap.authmode = WIFI_AUTH_OPEN;
	}

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG, "wifi_init_softap finished.SSID:%s password:%s \n",
			 EXAMPLE_DEFAULT_SSID, EXAMPLE_DEFAULT_PWD);
}

int get_socket_error_code(int socket)
{
	int result;
	u32_t optlen = sizeof(int);
	if (getsockopt(socket, SOL_SOCKET, SO_ERROR, &result, &optlen) == -1)
	{
		ESP_LOGE(TAG, "getsockopt failed");
		return -1;
	}
	return result;
}

int show_socket_error_reason(int socket)
{
	int err = get_socket_error_code(socket);
	ESP_LOGW(TAG, "socket error %d %s", err, strerror(err));
	return err;
}

int check_working_socket()
{
	int ret;
#if EXAMPLE_ESP_TCP_MODE_SERVER
	ESP_LOGD(TAG, "check server_socket");
	ret = get_socket_error_code(server_socket);
	if (ret != 0)
	{
		ESP_LOGW(TAG, "server socket error %d %s", ret, strerror(ret));
	}
	if (ret == ECONNRESET)
		return ret;
#endif
	ESP_LOGD(TAG, "check connect_socket");
	ret = get_socket_error_code(connect_socket);
	if (ret != 0)
	{
		ESP_LOGW(TAG, "connect socket error %d %s", ret, strerror(ret));
	}
	if (ret != 0)
		return ret;
	return 0;
}

void close_socket()
{
	close(connect_socket);
	close(server_socket);
}
