
/**
 * main.c
 */

void initialize();
void EnableGPIO();
void EnableUART();
void EnableTimer();
void sendChar();
void delay(int);
char keyPadOutput();

int main(void)
{
    initialize();
    EnableGPIO();
    EnableUART();
    EnableTimer();
    char c = '\0';
    char lastChar = '\0';

    while(1) {
        c = keyPadOutput();
        if (lastChar != c){
            lastChar = c;
            sendChar(c);
        }
        delay(30);

    }
}
