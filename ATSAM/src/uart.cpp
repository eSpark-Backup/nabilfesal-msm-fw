#include "main.h"
// #include <queue>

char rxBuffer[UART_RX_BUFFER_SIZE];
char txBuffer[UART_TX_BUFFER_SIZE];
size_t rxSize = 0;
volatile bool rxComplete = false;
volatile bool rxStarted = false;
volatile bool rxTimedout = false;
volatile uint64_t rxStartTimestamp = 0;


#ifdef __cplusplus // Provide C++ Compatibility
extern "C" {
#endif

// std::queue<const char*> uart_data;
    
void ProcessCommand(void)
{
    rxBuffer[rxSize] = '\0';
    char *ch;

    if (rxBuffer[0] == '{' && rxBuffer[rxSize - 1] == '}')
    {
        // check for JSON of calibration
        ch = strstr(rxBuffer, "command");
        if (ch != NULL)
        {
            ch = strchr(ch, ':');
            if (ch)
            {
                ch++;
                while (*ch == ' ' || *ch == '\"')
                    ch++;
                char value[32];
                int i = 0;
                while (*ch && *ch != '\"')
                {
                    value[i++] = *ch++;
                }
                value[i] = '\0';
               if (strcmp(value, "set-ct-ratio") == 0)
                {
                    parse_ct_ratio_json();
                }
                else if (strcmp(value, "reboot-calibration") == 0)
                {
                    parse_reboot_calibration_json();
                }
                else if (strcmp(value, "set-v-offset") == 0)
                {
                    parse_v_offset_json();
                }
                else if (strcmp(value, "set-i-offset") == 0)
                {
                    parse_i_offset_json();
                }
                else if (strcmp(value, "v-calibration") == 0)
                {
                    parse_v_calibration_json();
                }
                else if (strcmp(value, "i-calibration") == 0)
                {
                    parse_i_calibration_json();
                }
                else
                {
                    strcpy(txBuffer, "None Of The Calibration Parameter Values Found In JSON!");
                }
            }
        }
        else
        {
            strcpy(txBuffer, "JSON does not contain command key!");
        }
    }
    else if (strcmp(rxBuffer, "channel") == 0)
    {
        for (size_t i = 1; i <= 12; i++)
        {
            sendChannelJsonData(i);
        }
    }
    else if (strcmp(rxBuffer, "channel-1") == 0)
    {
        sendChannelJsonData(1);
    }
    else if (strcmp(rxBuffer, "channel-2") == 0)
    {
        sendChannelJsonData(2);
    }
    else if (strcmp(rxBuffer, "channel-3") == 0)
    {
        sendChannelJsonData(3);
    }
    else if (strcmp(rxBuffer, "channel-4") == 0)
    {
        sendChannelJsonData(4);
    }
    else if (strcmp(rxBuffer, "channel-5") == 0)
    {
        sendChannelJsonData(5);
    }
    else if (strcmp(rxBuffer, "channel-6") == 0)
    {
        sendChannelJsonData(6);
    }
    else if (strcmp(rxBuffer, "channel-7") == 0)
    {
        sendChannelJsonData(7);
    }
    else if (strcmp(rxBuffer, "channel-8") == 0)
    {
        sendChannelJsonData(8);
    }
    else if (strcmp(rxBuffer, "channel-9") == 0)
    {
        sendChannelJsonData(9);
    }
    else if (strcmp(rxBuffer, "channel-10") == 0)
    {
        sendChannelJsonData(10);
    }
    else if (strcmp(rxBuffer, "channel-11") == 0)
    {
        sendChannelJsonData(11);
    }
    else if (strcmp(rxBuffer, "channel-12") == 0)
    {
        sendChannelJsonData(12);
    }
    else if (strcmp(rxBuffer, "readstatus") == 0)
    {
        ConvertStatusToHexAndSend();
    }
    else
    {
        strcpy(txBuffer, "again");
    }

    strcat(txBuffer, "\n");
    SERCOM2_USART_Write(txBuffer, strlen(txBuffer));

    memset(rxBuffer, 0, UART_RX_BUFFER_SIZE);
    rxSize = 0;
    rxComplete = false;

    StartUSART_Reading();
}

void USART_ReadCallback(uintptr_t context)
{
    char received = rxBuffer[rxSize];

    if (received == '\n')
    {
        rxComplete = true;
    }
    else
    {
        // SERCOM2_USART_Write(&rxBuffer[rxSize], 1);          // for debug
        rxSize++;

        // prevent overflow
        if (rxSize >= UART_RX_BUFFER_SIZE)
        {
            rxSize = 0;
        }
        StartUSART_Reading();
    }
}

void CheckUSART_Timeout(void)
{
    if(((get_msCounter() - rxStartTimestamp) > UART_READ_TIMEOUT) && rxStarted)
    {
        SERCOM2_USART_ReadAbort();      // abort hardware uart reception

        rxStarted = false;              // remove reception flag
        rxSize = 0;                     // reset the size counter
        StartUSART_Reading();           // restart uart reception
    }
}

void StartUSART_Reading(void)
{
    rxStartTimestamp = get_msCounter();
    SERCOM2_USART_Read((void *)&rxBuffer[rxSize], 1);
    rxStarted = true;
}

void SetupUSART(void)
{
    memset(rxBuffer, 0, UART_RX_BUFFER_SIZE);
    SERCOM2_USART_ReadCallbackRegister(USART_ReadCallback, 0);
    StartUSART_Reading();
}

#ifdef __cplusplus  // Provide C++ Compatibility
}
#endif