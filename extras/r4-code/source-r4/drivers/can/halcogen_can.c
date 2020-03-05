/*
 * halcogen_can.c
 *
 *  Created on: Nov 15, 2019
 *      Author: arrooney
 */
#include "FreeRTOS.h"
#include <stdint.h>
#include <stdio.h>

#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
//#include <unistd.h>
#include <time.h>
#include <string.h>
#include <errno.h>

#include "os_semphr.h"
#include "os_task.h"

#include "can.h"
#include <csp/csp.h>
#include <csp/interfaces/csp_if_can.h>
#include "csp/drivers/can.h"

// the following function definitions are as defined by CSP. This file converts their functionality
// to that defined by CSP
int can_send(can_id_t id, uint8_t * data, uint8_t dlc); // The CSP definition of sending a CAN frame
int can_init(uint32_t id, uint32_t mask, struct csp_can_config *conf);
// This sends data byte by byte, dlc should always be 8

int can_send(can_id_t id, uint8_t* data, uint8_t dlc) {
    // populate data values in message box 1, on can register 1
    // TODO: make a switch case for different message boxes for different dlc values...
    //canUpdateID(canREG2, canMESSAGE_BOX1, id);
    canTransmit(canREG2, canMESSAGE_BOX2, data); // send the data?
    return 0;
}


static void can_rx_thread(void * parameters)
{
    can_frame_t frame;
    int nbytes;
    // recieve on canREG1, box 2
    // TODO: check which message box it's arriving on and
    // change dlc field depending on that.
    while (1) {

        while(!canIsRxMessageArrived(canREG2, canMESSAGE_BOX1)){
            vTaskDelay(10);
        }
        /* Read CAN frame */
        //uint8_t * rx_data = (uint8_t *)pvPortMalloc(8*sizeof(uint8_t));
        uint8_t rx_data[8] = {0};
        canGetData(canREG2, canMESSAGE_BOX1, rx_data);
        frame.data32[0] = (uint32_t) (rx_data);
        frame.data32[1] = (uint32_t) (rx_data + sizeof(uint32_t));
        frame.id = (can_id_t) canGetID(canREG2, canMESSAGE_BOX1);
        frame.dlc = 8; // TODO make this not a magic number
//        if (nbytes < 0) {
//            csp_log_error("read: %s", strerror(errno));
//            continue;
//        }
//
//        if (nbytes != sizeof(frame)) {
//            csp_log_warn("Read incomplete CAN frame");
//            continue;
//        }
//
//        /* Frame type */
//        if (frame.can_id & (CAN_ERR_FLAG | CAN_RTR_FLAG) || !(frame.can_id & CAN_EFF_FLAG)) {
//            /* Drop error and remote frames */
//            csp_log_warn("Discarding ERR/RTR/SFF frame");
//            continue;
//        }

//        /* Strip flags */
        //frame.id &= CAN_EFF_MASK;
//
//        /* Call RX callback */
        csp_can_rx_frame((can_frame_t *)&frame, NULL);
    }

    return 0;
}

int can_init(uint32_t id, uint32_t mask, struct csp_can_config *conf) {
    // must init the can reg.
    // TODO: figure out how to configure halcogen CAN on the fly
    canInit(); // the halcogen call takes no parameters, all configurations are done in the halcogen GUI
    xTaskHandle can_rx;
    xTaskCreate(can_rx_thread, "RX_CAN", 1000, ( void * ) NULL, 0, &can_rx );

    return 0;
}


