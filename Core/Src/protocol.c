#include "protocol.h"
#include "max485.h"
#include "Isolated_Input.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <stddef.h>
#include <string.h>

static uint8_t nodeId = 1;
static uint8_t destId = 2;
static char role[4] = "TX";

static uint32_t freq = 433000000;
static uint8_t bw = 0;
static uint8_t sf = 10;
static uint8_t cr = 1;
static int8_t pwr = 14;

#define CONFIG_FLASH_ADDR   ((uint32_t)0x0800FC00)
#define CONFIG_MAGIC        ((uint32_t)0x43464731)   // "CFG1"

typedef struct __attribute__((packed))
{
    uint32_t magic;

    uint8_t nodeId;
    uint8_t destId;
    char role[4];

    uint32_t freq;

    uint8_t bw;
    uint8_t sf;
    uint8_t cr;
    int8_t pwr;

    uint16_t checksum;
} AppConfig_t;

static uint16_t Config_CalcChecksum(AppConfig_t *cfg)
{
    uint16_t sum = 0;
    uint8_t *p = (uint8_t *)cfg;

    for (uint32_t i = 0; i < offsetof(AppConfig_t, checksum); i++)
    {
        sum += p[i];
    }

    return sum;
}

static void Config_LoadDefault(void)
{
    nodeId = 1;
    destId = 2;
    strcpy(role, "TX");

    freq = 433000000;
    bw = 0;
    sf = 10;
    cr = 1;
    pwr = 14;
}

void Config_LoadFromFlash(void)
{
    AppConfig_t *cfg = (AppConfig_t *)CONFIG_FLASH_ADDR;

    if (cfg->magic != CONFIG_MAGIC)
    {
        Config_LoadDefault();
        return;
    }

    if (Config_CalcChecksum(cfg) != cfg->checksum)
    {
        Config_LoadDefault();
        return;
    }

    nodeId = cfg->nodeId;
    destId = cfg->destId;
    strncpy(role, cfg->role, 3);
    role[3] = '\0';

    freq = cfg->freq;
    bw = cfg->bw;
    sf = cfg->sf;
    cr = cfg->cr;
    pwr = cfg->pwr;
}

static uint8_t Config_SaveToFlash(void)
{
    AppConfig_t cfg;

    cfg.magic = CONFIG_MAGIC;

    cfg.nodeId = nodeId;
    cfg.destId = destId;
    strncpy(cfg.role, role, 3);
    cfg.role[3] = '\0';

    cfg.freq = freq;
    cfg.bw = bw;
    cfg.sf = sf;
    cfg.cr = cr;
    cfg.pwr = pwr;

    cfg.checksum = Config_CalcChecksum(&cfg);

    HAL_FLASH_Unlock();

    FLASH_EraseInitTypeDef erase;
    uint32_t pageError = 0;

    erase.TypeErase = FLASH_TYPEERASE_PAGES;
    erase.PageAddress = CONFIG_FLASH_ADDR;
    erase.NbPages = 1;

    if (HAL_FLASHEx_Erase(&erase, &pageError) != HAL_OK)
    {
        HAL_FLASH_Lock();
        return 0;
    }

    uint8_t *data = (uint8_t *)&cfg;

    for (uint32_t i = 0; i < sizeof(AppConfig_t); i += 2)
    {
        uint16_t halfword = data[i];

        if ((i + 1) < sizeof(AppConfig_t))
        {
            halfword |= ((uint16_t)data[i + 1] << 8);
        }

        if (HAL_FLASH_Program(FLASH_TYPEPROGRAM_HALFWORD,
                              CONFIG_FLASH_ADDR + i,
                              halfword) != HAL_OK)
        {
            HAL_FLASH_Lock();
            return 0;
        }
    }

    HAL_FLASH_Lock();

    return 1;
}

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
        if (Config_SaveToFlash())
        {
            MAX485_SendString("OK SAVE FLASH\r\n");
        }
        else
        {
            MAX485_SendString("ERROR SAVE FLASH\r\n");
        }
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
