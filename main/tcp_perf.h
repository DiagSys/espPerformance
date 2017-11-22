/* tcp_perf Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#ifndef __TCP_PERF_H__
#define __TCP_PERF_H__

#ifdef __cplusplus
extern "C" {
#endif

/*test options*/
#define EXAMPLE_ESP_WIFI_MODE_AP TRUE	//TRUE:AP FALSE:STA
#define EXAMPLE_ESP_TCP_MODE_SERVER TRUE //TRUE:server FALSE:client
//#define EXAMPLE_ESP_TCP_PERF_TX CONFIG_TCP_PERF_TX //TRUE:send FALSE:receive
#define EXAMPLE_ESP_TCP_DELAY_INFO TRUE //TRUE:show delay time info

/*AP info and tcp_server info*/
#define EXAMPLE_DEFAULT_SSID "TESTAP"
#define EXAMPLE_DEFAULT_PWD ""
#define EXAMPLE_DEFAULT_PORT 4567
#define EXAMPLE_DEFAULT_PKTSIZE 13
#define EXAMPLE_MAX_STA_CONN 1 //how many sta can be connected(AP mode)

#define EXAMPLE_DEFAULT_SERVER_IP "192.168.4.1"

#define EXAMPLE_PACK_BYTE_IS 97 //'a'
#define TAG "tcp_perf:"

/* FreeRTOS event group to signal when we are connected to wifi*/
extern EventGroupHandle_t tcp_event_group;
#define WIFI_CONNECTED_BIT BIT0

extern int total_data;

#if EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO
extern int total_pack;
extern int send_success;
extern int send_fail;
extern int delay_classify[5];
#endif /*EXAMPLE_ESP_TCP_PERF_TX && EXAMPLE_ESP_TCP_DELAY_INFO*/

//using esp as station
void wifi_init_sta();
//using esp as softap
void wifi_init_softap();

//create a tcp server socket. return ESP_OK:success ESP_FAIL:error
esp_err_t create_tcp_server();
//create a tcp client socket. return ESP_OK:success ESP_FAIL:error
esp_err_t create_tcp_client();

//send data task
void send_data(void *pvParameters);
//receive data task
void recv_data(void *pvParameters);

//close all socket
void close_socket();

//get socket error code. return: error code
int get_socket_error_code(int socket);

//show socket error code. return: error code
int show_socket_error_reason(int socket);

//check working socket
int check_working_socket();

#ifdef __cplusplus
}
#endif

#endif /*#ifndef __TCP_PERF_H__*/
