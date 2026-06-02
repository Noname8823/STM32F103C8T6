/*
 * protocol.c
 *
 *  Created on: May 28, 2026
 *      Author: tranquocvu2
 */


#include "Protocol.h"
#include "max485.h"
#include "Isolated_Input.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static void Protocol_SendOK(char *msg)
{
    char tx[64];
    snprintf(tx, sizeof(tx), "@%02d,OK,%s\r\n", DEVICE_ID, msg);
    MAX485_SendString(tx);
}

static void Protocol_SendERR(char *msg)
{
    char tx[64];
    snprintf(tx, sizeof(tx), "@%02d,ERR,%s\r\n", DEVICE_ID, msg);
    MAX485_SendString(tx);
}

void Protocol_Process(char *frame)
{
    if (frame == NULL)
    {
        return;
    }

    /*
     * Frame mẫu:
     * @01,PING
     * @01,LED,1
     * @01,GET_INPUT
     */

    if (frame[0] != '@')
    {
        Protocol_SendERR("INVALID_START");
        return;
    }

    char *addr_str = strtok(frame + 1, ",");
    char *cmd      = strtok(NULL, ",");
    char *data     = strtok(NULL, ",");

    if (addr_str == NULL || cmd == NULL)
    {
        Protocol_SendERR("INVALID_FRAME");
        return;
    }

    uint8_t addr = atoi(addr_str);

    if (addr != DEVICE_ID)
    {
        return;
    }

    if (strcmp(cmd, "PING") == 0)
    {
        Protocol_SendOK("PONG");
    }
    else if (strcmp(cmd, "LED") == 0)
    {
        if (data == NULL)
        {
            Protocol_SendERR("NO_DATA");
            return;
        }

        if (strcmp(data, "1") == 0)
        {
            /*
             * Nếu LED active low thì RESET là sáng.
             * Nếu LED bình thường thì đổi RESET thành SET.
             */
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
            Protocol_SendOK("LED=1");
        }
        else if (strcmp(data, "0") == 0)
        {
            HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);
            Protocol_SendOK("LED=0");
        }
        else
        {
            Protocol_SendERR("LED_DATA");
        }
    }
    else if (strcmp(cmd, "GET_INPUT") == 0)
    {
        uint8_t state[4];
        char msg[64];

        Input_GetAll(state);

        snprintf(msg, sizeof(msg),
                 "IN=%d,%d,%d,%d",
                 state[0],
                 state[1],
                 state[2],
                 state[3]);

        Protocol_SendOK(msg);
    }
    else
    {
        Protocol_SendERR("UNKNOWN_CMD");
    }
}
