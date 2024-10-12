#ifndef CONSOLE_HEADER
#define CONSOLE_HEADER

#define CONSOLE_CONTROLLER_PLIC 10
#define CONSOLE_BUFFER_SIZE 1024
#define CONSOLE_STATUS_SEND 0x20
#define CONSOLE_STATUS_RECEIVE 0x1

void __console_init();
int __putc(char c);
// int __getc(char *c);
int __getc();
void __console_receive();
void __console_send();

#endif //CONSOLE_HEADER
