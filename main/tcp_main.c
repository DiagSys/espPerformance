/* tcp_perf Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
tcp_perf example

Using this example to test tcp throughput performance.
esp<->esp or esp<->ap

step1:
    init wifi as AP/STA using config SSID/PASSWORD.

step2:
    create a tcp server/client socket using config PORT/(IP).
    if server: wating for connect.
    if client connect to server.
step3:
    send/receive data to/from each other.
    if the tcp connect established. esp will send or receive data.
    you can see the info in serial output.
*/

#include <errno.h>
#include <sys/socket.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_log.h"
#include "esp_err.h"

#include "tcp_perf.h"
#include "CAN.h"

CAN_device_t CAN_cfg = {
	.speed = CAN_SPEED_500KBPS, // CAN Node baudrade
	.tx_pin_id = GPIO_NUM_15,   // CAN TX pin
	.rx_pin_id = GPIO_NUM_4,	// CAN RX pin
	.rx_queue = NULL,			// FreeRTOS queue for RX frames
};

//this task establish a TCP connection and receive data from TCP
static void tcp_conn(void *pvParameters)
{
	ESP_LOGI(TAG, "task tcp_conn.");
	/*wating for connecting to AP*/
	xEventGroupWaitBits(tcp_event_group, WIFI_CONNECTED_BIT, false, true, portMAX_DELAY);

	ESP_LOGI(TAG, "sta has connected to ap.");

	/*create tcp socket*/
	//int socket_ret;

	//ESP_LOGI(TAG, "tcp_server will start after 3s...");
	//vTaskDelay(3000 / portTICK_RATE_MS);
	ESP_LOGI(TAG, "create_tcp_server.");

	while (true)
	{
		ESP_LOGI(TAG, "befor create_tcp_server!");
		create_tcp_server();
		ESP_LOGI(TAG, "after create_tcp_server!");

		close_socket();
	}
	// vTaskDelete(tx_rx_task);
	// vTaskDelete(NULL);
}

extern int connect_socket;
static void task_CAN(void *pvParameters)
{
	(void)pvParameters;

	//frame buffer
	CAN_frame_t rxFrame;

	//create CAN RX Queue
	CAN_cfg.rx_queue = xQueueCreate(10, sizeof(CAN_frame_t));

	while (1)
	{
		//receive next CAN frame from queue
		if (xQueueReceive(CAN_cfg.rx_queue, &rxFrame, 30 * portTICK_PERIOD_MS) == pdTRUE)
		{
			char buffer[50];

			//int i = 4;
			int msgID_len = 4;

			memcpy(&buffer[0], &rxFrame.MsgID, msgID_len);
			memcpy(&buffer[msgID_len], &rxFrame.data, rxFrame.FIR.B.DLC);
			int messageLen = rxFrame.FIR.B.DLC + msgID_len;
			buffer[messageLen] = '\n';
			messageLen++;
			// for (int j = 0; j < rxFrame.FIR.B.DLC; i++, j++)
			// {
			// 	buffer[i] = rxFrame.data.u8[j];
			// }

			if (connect_socket != 0)
			{
				int len = 0;
				if ((len = send(connect_socket, buffer, messageLen, 0)) <= 0)
				{
					if (LOG_LOCAL_LEVEL >= ESP_LOG_DEBUG)
					{
						show_socket_error_reason(connect_socket);
					}
					printf("sendErr:%d\n", len);
				}
			}
		}
	}
}

#include "nvs_flash.h"
#include "nvs.h"

void app_main(void)
{
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES) {
        // NVS partition was truncated and needs to be erased
        // Retry nvs_flash_init
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );

	//start CAN Module
	CAN_init();

	ESP_LOGI(TAG, "EXAMPLE_ESP_WIFI_MODE_AP");
	wifi_init_softap();
	xTaskCreate(&tcp_conn, "tcp_conn", 4096, NULL, 5, NULL);
	//xTaskCreatePinnedToCore(&task_CAN, "CAN", 5048, NULL, 5, NULL, 1);
	xTaskCreate(&task_CAN, "CAN", 4096, NULL, 5, NULL);
}
