#include "protocol.h"
#include "max485.h"
#include "Isolated_Input.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

static uint8_t nodeId = 1;
static uint8_t destId = 2;
static char role[4] = "TX";

static uint32_t freq = 433000000;
static uint8_t bw = 0;
static uint8_t sf = 10;
static uint8_t cr = 1;
static int8_t pwr = 14;

static void Protocol_SendOK(void)
{
    MAX485_SendString("OK\r\n");
}

static void Protocol_SendError(void)
{
    MAX485_SendString("ERROR\r\n");
}

static void Protocol_SendConfig(void)
{
    char tx[128];

    sprintf(tx,
            "CFG ID=%d DST=%d ROLE=%s FREQ=%lu BW=%d SF=%d CR=%d PWR=%d\r\n",
            nodeId,
            destId,
            role,
            freq,
            bw,
            sf,
            cr,
            pwr);

    MAX485_SendString(tx);
}

static void Protocol_SendInput(void)
{
    uint8_t state[4];
    char tx[64];

    Input_GetAll(state);

    sprintf(tx,
            "IN=%d%d%d%d\r\n",
            state[0],
            state[1],
            state[2],
            state[3]);

    MAX485_SendString(tx);
}

void Protocol_Process(char *cmd)
{
    if (cmd == NULL)
    {
        return;
    }

    if (strcmp(cmd, "AT") == 0)
    {
        Protocol_SendOK();
    }
    else if (strcmp(cmd, "AT+GETCFG") == 0)
    {
        Protocol_SendConfig();
    }
    else if (strcmp(cmd, "AT+GETIN") == 0)
    {
        Protocol_SendInput();
    }
    else if (strcmp(cmd, "AT+SAVE") == 0)
    {
        MAX485_SendString("OK SAVE\r\n");
    }
    else if (strncmp(cmd, "AT+SETID=", 9) == 0)
    {
        nodeId = (uint8_t)atoi(&cmd[9]);
        Protocol_SendOK();
    }
    else if (strncmp(cmd, "AT+SETDST=", 10) == 0)
    {
        destId = (uint8_t)atoi(&cmd[10]);
        Protocol_SendOK();
    }
    else if (strncmp(cmd, "AT+SETROLE=", 11) == 0)
    {
        strncpy(role, &cmd[11], 3);
        role[3] = '\0';
        Protocol_SendOK();
    }
    else if (strncmp(cmd, "AT+SETFREQ=", 11) == 0)
    {
        freq = (uint32_t)atol(&cmd[11]);
        Protocol_SendOK();
    }
    else if (strncmp(cmd, "AT+SETBW=", 9) == 0)
    {
        bw = (uint8_t)atoi(&cmd[9]);
        Protocol_SendOK();
    }
    else if (strncmp(cmd, "AT+SETSF=", 9) == 0)
    {
        sf = (uint8_t)atoi(&cmd[9]);
        Protocol_SendOK();
    }
    else if (strncmp(cmd, "AT+SETCR=", 9) == 0)
    {
        cr = (uint8_t)atoi(&cmd[9]);
        Protocol_SendOK();
    }
    else if (strncmp(cmd, "AT+SETPWR=", 10) == 0)
    {
        pwr = (int8_t)atoi(&cmd[10]);
        Protocol_SendOK();
    }
    else
    {
        Protocol_SendError();
    }
}
