//26.08.17

#include "hw_types.h"
#include "hw_memmap.h"
#include "rom_map.h"

//#include "uart_if.h"
#include "uart.h"

#include "cc1310_interface.h"
#include "global.h"
#include "appUtils.h"

message_t messageBuff [MAX_MESSAGE_COUNT];
_u8 messageCount = 0;
_u8 messageIndex = 0;



//----------------------------------------------------------------------------------------------------------------------------------
int GETChar(char *ucBuffer)
{

/*    int i=0;
    char c;


    //
    // Wait to receive a character over UART
    //
    while(MAP_UARTCharsAvail(CONSOLE) == false)
    {
        osi_Sleep(1);
    }
    c = MAP_UARTCharGetNonBlocking(CONSOLE);

    //MAP_UARTCharPut(CONSOLE, c);
    //
    // Checking the end of line
    //
    while(c!='\r')
    {
            *(ucBuffer+i)=c;
            i++;
        while(MAP_UARTCharsAvail(CONSOLE) == false)
        {
            osi_Sleep(1);
        }
        c = MAP_UARTCharGetNonBlocking(CONSOLE);
//        MAP_UARTCharPut(CONSOLE, c);
    }

//    strncpy((char*)g_ucUARTBuffer,(char *)ucBuffer,ilength);
//    memset(g_ucUARTRecvBuffer, 0, sizeof(g_ucUARTRecvBuffer));
    return i;*/
    return 0;
}

//----------------------------------------------------------------------------------------------------------------------------------
void UART_Task (void *pvParameters)
{
    char UARTRXBuffer [100];
    char a;
    int length;
    int lengthVerify;

    while(1)
    {
        length = GETChar(&UARTRXBuffer[0]);

        BlinkLED (0);

        lengthVerify = 0;
        lengthVerify = HexToChar (UARTRXBuffer[0]) << 4;
        lengthVerify |= HexToChar (UARTRXBuffer[1]);

//        SendText (UARTRXBuffer, length);

        if (length != lengthVerify * 2 + 4)
        {
          //  sendError();
//            SendText (UARTRXBuffer, length);

            continue;
        }

        if (messageCount < MAX_MESSAGE_COUNT)
        {
            _u8 newIndex = messageIndex + messageCount;
            if (newIndex >= MAX_MESSAGE_COUNT)
                newIndex -= MAX_MESSAGE_COUNT;

            messageBuff[newIndex].UARTIndex = 0;
            messageBuff[newIndex].UARTIndex = HexToChar (UARTRXBuffer[2]) << 4;
            messageBuff[newIndex].UARTIndex |= HexToChar (UARTRXBuffer[3]) << 0;


            messageBuff[newIndex].radioIndex = 0;
            messageBuff[newIndex].radioIndex = HexToChar (UARTRXBuffer[4]) << 12;
            messageBuff[newIndex].radioIndex |= HexToChar (UARTRXBuffer[5]) << 8;
            messageBuff[newIndex].radioIndex |= HexToChar (UARTRXBuffer[6]) << 4;
            messageBuff[newIndex].radioIndex |= HexToChar (UARTRXBuffer[7]) << 0;

            a = 0;
            int s;
            for (s = 8; s < 24; s += 2)
            {
                messageBuff[newIndex].mac[a] = 0;
                messageBuff[newIndex].mac[a] = HexToChar (UARTRXBuffer[s]) << 4;
                messageBuff[newIndex].mac[a] |= HexToChar (UARTRXBuffer[s + 1]);
                a++;
            }

            a = 0;
            for (s = 24; s < length; s += 2)
            {
                messageBuff[newIndex].message[a] = 0;
                messageBuff[newIndex].message[a] = HexToChar (UARTRXBuffer[s]) << 4;
                messageBuff[newIndex].message[a] |= HexToChar (UARTRXBuffer[s + 1]);
                a++;
            }

            //SendNumber ("UARTIndex", strlen ("UARTIndex"), messageBuff[newIndex].UARTIndex);

            messageCount++;
        }

        BlinkLED (0);
    }
}
