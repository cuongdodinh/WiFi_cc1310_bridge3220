//26.08.17

#include <App/global.h>
#include <App/appUtils.h>
#include <ti/sysbios/knl/Clock.h>

//----------------------------------------------------------------------------------------------------------------------------------
char HexToChar (char b)
{
    if (b <= '9')
        return b - '0';
    else
        return 10 + b - 'A';
}

char hex [16] = "0123456789ABCDEF";

//------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
_u8 ToHexString (_u8* byteArrayBuff, _u8 len, char* hexStringBuff)
{
    int s;
    int len2 = len * 2;

    for (s = 0; s < len2; s += 2)
    {
        hexStringBuff [s] = hex [*byteArrayBuff >> 4];
        hexStringBuff [s + 1] = hex [*byteArrayBuff & 0x0F];
        byteArrayBuff++;
    }

    return len2;
}

_u16 crcTable [256];

//--------------------------------------------------------------------------------------------------------------------------
void MakeCRC16Table(void)
{
  _u16 r;
  int s, s1;

  for(s = 0; s < 256; s++)
  {
    r = ((_u16)s)<<8;

    for (s1 = 0; s1 < 8; s1++)
    {
    if (r&(1<<15))
        r = (r<<1)^0x8005;
    else
        r = r<<1;
    }
     crcTable[s] = r;
   }
}

//--------------------------------------------------------------------------------------------------------------------------
_u16 GetCRC16(_u8 *buf, _u16 len)
{
    _u16 crc;
    crc = 0xFFFF;
    while (len--)
    {
        crc = crcTable [((crc>>8)^*buf++)&0xFF] ^ (crc<<8);
    }
    crc ^= 0xFFFF;

    return crc;
}


#define MAX_STORED_INTERVALS 100

static unsigned int startTicks;
static int storedIntervals[MAX_STORED_INTERVALS];
static int storedIntervalsCount = 0;
char buffer [MAX_STORED_INTERVALS * 20];

//---------------------------------------------------------------------------------------------------
void StopwatchRestart()
{
    startTicks = Clock_getTicks();
    storedIntervalsCount = 0;

    memset (storedIntervals, -1, sizeof (long) * MAX_STORED_INTERVALS);
}

//---------------------------------------------------------------------------------------------------
unsigned int StopwatchTicks()
{
    return Clock_getTicks() - startTicks;
}

//---------------------------------------------------------------------------------------------------
void StopwatchStoreInterval(int index)
{
    if (index >= MAX_STORED_INTERVALS)
    {
        LOG_ERROR(index);
        return;
    }

    storedIntervals [index] = Clock_getTicks() - startTicks;

    if (index + 1 > storedIntervalsCount)
        storedIntervalsCount = index + 1;
}

/* reverse:  переворачиваем строку s на месте */
void reverse(char s[])
{
    int i, j;
    char c;

    for (i = 0, j = strlen(s)-1; i<j; i++, j--) {
        c = s[i];
        s[i] = s[j];
        s[j] = c;
    }
}

/* itoa:  конвертируем n в символы в s */
int itoa(int n, char s[])
{
    int i, sign;

    if ((sign = n) < 0)  /* записываем знак */
        n = -n;          /* делаем n положительным числом */
    i = 0;
    do {       /* генерируем цифры в обратном порядке */
        s[i++] = n % 10 + '0';   /* берем следующую цифру */
    } while ((n /= 10) > 0);     /* удаляем */
    if (sign < 0)
        s[i++] = '-';
    s[i] = '\0';
    reverse(s);

    return i;
}

//---------------------------------------------------------------------------------------------------
char* StopwatchPrintStoredIntervals ()
{
    char* bufferPos = buffer;
    int s;
    for (s = 0; s < storedIntervalsCount; s++)
    {
        bufferPos += itoa (s, bufferPos);

        *bufferPos++ = ':';
        *bufferPos++ = ' ';

        bufferPos += itoa (storedIntervals [s], bufferPos);

        *bufferPos++ = ' ';
        *bufferPos++ = ' ';
    }

    *bufferPos = 0;

    return buffer;
}
