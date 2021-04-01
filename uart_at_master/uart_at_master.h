#ifndef __UART_AT_MASTER
#define __UART_AT_MASTER

#include "mbed.h"

#define __AT_MASTER_BUF_SIZE 1024

enum _AT_RESPONSE_TYPE {
    AT_RESPONSE_NONE,
    AT_RESPONSE_OK,
    AT_RESPONSE_ERROR,
    AT_RESPONSE_INIT,
    AT_RESPONSE_CUSTOM,
    AT_RESPONSE_BLANKLINE,
    AT_RESPONSE_OTHER
};

enum _AT_NEWLINE_MODE {
    AT_NEWLINE_CRLF, // \r\n
    AT_NEWLINE_LF    //   \n
};
    
class ATMaster {
public:
    ATMaster(PinName txPin, PinName rxPin, char * ok_prefix, char * error_prefix);
    uint32_t has_data();
    _AT_RESPONSE_TYPE process(char * destination_string = 0);
    void set_init_prefix(char * init_prefix);
    void set_custom_prefix(char * custom_prefix);
    void set_newline_mode(_AT_NEWLINE_MODE new_mode);
    void _rx_irq();
    
    Serial _uart;

private:
    void _buf_add(char value);
    char _buf_take();
    uint32_t buf_compare(const char * value, bool exact_length = true);
    
    void _tx_irq();

    char * _at_ok_prefix;
    char * _at_error_prefix;
    char * _at_init_prefix;
    char * _at_custom_prefix;
    uint32_t _newline_mode;
    
    char _buf[__AT_MASTER_BUF_SIZE];
    uint32_t _buf_head;
    uint32_t _buf_tail;
    uint32_t _buf_len;
    uint32_t _buf_message_len;
    uint32_t _has_data;
    uint32_t _wait_init;
    char _last_received_char;
};




#endif