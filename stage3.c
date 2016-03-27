#include "types.h"
#include "stat.h"
#include "user.h"
#include "signal.h"

static int start;
static int end;
static int count;

void ftoa(float n, char *res, int afterpoint);
int intToStr(int x, char str[], int d);
void reverse(char *str, int len);
float pow(float a, int b);

void handle_signal(int signum)
{ 
  if (count >= 1000000) {
    end = uptime();
    skip();
  } else count += 1; 
}

int main(int argc, char *argv[])
{
        count = 0, start = uptime();

        signal(-1,(uint)trampoline);
	signal(SIGFPE, handle_signal);

        int exception = 10/0;	
        float T = (float)(end - start);
        char total[20];
        char TTP[20];

        ftoa(T, total, 4);

	printf(1, "Traps Performed: %d\n", count);
	printf(1, "Total Elapsed Time: %s ms\n",  total);

        T /= count;
        ftoa(T, TTP, 5);

	printf(1, "Average Time Per Trap: %s ms\n", TTP);

	exit();
}

void reverse(char *str, int len)
{
    int i=0, j=len-1, temp;
    while (i<j)
    {
        temp = str[i];
        str[i] = str[j];
        str[j] = temp;
        i++; j--;
    }
}

int intToStr(int x, char str[], int d)
{
    int i = 0;
    while (x)
    {
        str[i++] = (x%10) + '0';
        x = x/10;
    }
 
    // If number of digits required is more, then
    // add 0s at the beginning
    while (i < d)
        str[i++] = '0';
 
    reverse(str, i);
    str[i] = '\0';
    return i;
}

void ftoa(float n, char *res, int afterpoint)
{
    // Extract integer part
    int ipart = (int)n;
 
    // Extract floating part
    float fpart = n - (float)ipart;
 
    // convert integer part to string
    int i = intToStr(ipart, res, 0);
 
    // check for display option after point
    if (afterpoint != 0)
    {
        res[i] = '.';  // add dot
 
        // Get the value of fraction part upto given no.
        // of points after dot. The third parameter is needed
        // to handle cases like 233.007
        fpart = fpart * pow(10, afterpoint);
 
        intToStr((int)fpart, res + i + 1, afterpoint);
    }
}

float pow(float x, int y)
{
    float temp;
    if( y == 0)
       return 1;
    temp = pow(x, y/2);       
    if (y%2 == 0)
        return temp*temp;
    else
    {
        if(y > 0)
            return x*temp*temp;
        else
            return (temp*temp)/x;
    }
}  

