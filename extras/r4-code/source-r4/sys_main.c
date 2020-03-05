/** @file sys_main.c 
*   @brief Application main file
*   @date 11-Dec-2018
*   @version 04.07.01
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2018 Texas Instruments Incorporated - www.ti.com 
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
#include <stdio.h>
#include <string.h>
#include "FreeRTOS.h"
#include "os_task.h"

#include <csp/csp.h>
#include <sys_common.h>
//#include <csp/interfaces/csp_if_lo.h>
#include <csp/interfaces/csp_if_can.h>
#include <csp/drivers/can.h>
#include <csp/csp_interface.h>
#include <csp/csp_types.h>

/* Using un-exported header file.
 * This is allowed since we are still in libcsp */
#include <csp/arch/csp_thread.h>

/** Example defines */
#define SERVER_ADDRESS 1
#define MY_ADDRESS  2           // Address of local CSP node
#define MY_PORT     10          // Port to send test traffic to
#define CSP_HOST_MAC 1

//#define SERVER

/* USER CODE END */

/* Include Files */

#include "sys_common.h"

/* USER CODE BEGIN (1) */
CSP_DEFINE_TASK(task_server) {

    /* Create socket without any socket options */
    csp_socket_t *sock = csp_socket(CSP_SO_NONE);

    /* Bind all ports to socket */
    csp_bind(sock, CSP_ANY);

    /* Create 10 connections backlog queue */
    csp_listen(sock, 10);

    /* Pointer to current connection and packet */
    csp_conn_t *conn;
    csp_packet_t *packet;

    /* Process incoming connections */
    while (1) {
//        csp_sleep_ms(1000);
//        fprintf(stderr, "a\n");
//        csp_sleep_ms(1500);
        /* Wait for connection, 10000 ms timeout */
        if ((conn = csp_accept(sock, 10000)) == NULL)
            continue;

        /* Read packets. Timout is 100 ms */
        while ((packet = csp_read(conn, 100)) != NULL) {
            switch (csp_conn_dport(conn)) {
            case MY_PORT:
                /* Process packet here */
                fprintf(stderr, "Packet received on MY_PORT: %s\r\n", (char *) packet->data);
                csp_buffer_free(packet);
                break;

            default:
                /* Let the service handler reply pings, buffer use, etc. */
                csp_service_handler(conn, packet);
                break;
            }
        }

        /* Close current connection, and handle next */
        csp_close(conn);

    }

    return CSP_TASK_RETURN;

}

CSP_DEFINE_TASK(task_client) {

    csp_packet_t * packet;
    csp_conn_t * conn;

    while (1) {

        /**
         * Try ping
         */

        int result = csp_ping(SERVER_ADDRESS, 100, 100, CSP_O_NONE);
        fprintf(stderr, "Ping result %d [ms]\r\n", result);

        csp_sleep_ms(100);

        /**
         * Try data packet to server
         */

        /* Get packet buffer for data */
        packet = csp_buffer_get(100);
        if (packet == NULL) {
            /* Could not get buffer element */
            fprintf(stderr, "Failed to get buffer element\n");
            return CSP_TASK_RETURN;
        }

        /* Connect to host HOST, port PORT with regular UDP-like protocol and 1000 ms timeout */
        conn = csp_connect(CSP_PRIO_NORM, SERVER_ADDRESS, MY_PORT, 1000, CSP_O_NONE);
        if (conn == NULL) {
            /* Connect failed */
            fprintf(stderr, "Connection failed\n");
            /* Remember to free packet buffer */
            csp_buffer_free(packet);
            return CSP_TASK_RETURN;
        }

        /* Copy dummy data to packet */
        char *msg = "Hello World";
        strcpy((char *) packet->data, msg);

        /* Set packet length */
        packet->length = strlen(msg);

        /* Send packet */
        if (!csp_send(conn, packet, 1000)) {
            /* Send failed */
            fprintf(stderr, "Send failed\n");
            csp_buffer_free(packet);
        }

        /* Close connection */
        csp_close(conn);

    }

    return CSP_TASK_RETURN;
}

/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */

//CAN interface struct

//csp_iface_t csp_can_tx;


/* USER CODE END */

int main(void)
{
/* USER CODE BEGIN (3) */


    struct csp_can_config can_conf = {.ifc = "can0"};

    /* Init buffer system with 10 packets of maximum 300 bytes each */
    csp_buffer_init(10, 320);
    csp_init(MY_ADDRESS);
    csp_can_init(CSP_CAN_MASKED, &can_conf);


    csp_route_set(CSP_DEFAULT_ROUTE, &csp_if_can, CSP_HOST_MAC);

    /* Start router task with 500 word stack, OS task priority 1 */
    csp_route_start_task(500, 1);


    #ifdef SERVER
        csp_thread_handle_t handle_server;
        csp_thread_create(task_server, "SERVER", 1000, NULL, 0, &handle_server);
    #else
        csp_thread_handle_t handle_client;
        csp_thread_create(task_client, "CLIENT", 1000, NULL, 0, &handle_client);
    #endif

    vTaskStartScheduler();


    while(1) {
       csp_sleep_ms(1000000);
    }

/* USER CODE END */

    return 0;
}


/* USER CODE BEGIN (4) */

/* USER CODE END */
