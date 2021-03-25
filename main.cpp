#define WIFI_SSID     "VM6B342D1"
#define WIFI_PASSWORD "4nsZcvenfktw"

// Do not use too short a period, for better data visualization later
// Too much data will result in a giant unusable data table
#define DATA_PERIOD 60.0         // in seconds

// NUCLEO
//#define ESP_TX        PD_5
//#define ESP_RX        PD_6
//#define ADC_PIN       ?

// mbed
#define ESP_TX        p28
#define ESP_RX        p27
#define ADC_PIN       p20



//======================================================================



#include "mbed.h"
//#include "esp_simple_rest/esp_simple_rest.h"

#include "uart_at_master/uart_at_master.h"

Serial pc(USBTX, USBRX);

Ticker t;
AnalogIn adc(ADC_PIN);

ATMaster at(ESP_TX, ESP_RX, "OK", "ERROR");

enum ESPStatus {
    ESP_IDLE,
    ESP_WAITING,
    ESP_RECEIVED
};

uint32_t _esp_status = ESP_IDLE;
uint32_t _esp_last_response = 0;


enum  AppStates {
    APPSTATE_RESET,
    APPSTATE_SETMODE,
    APPSTATE_CONNECT_WIFI,
    APPSTATE_SETDNS,
    APPSTATE_STANDBY,
    APPSTATE_OPEN_TCP,
    APPSTATE_SEND_DATA,
    APPSTATE_SENDING_DATA,
    APPSTATE_RECEIVING_RESPONSE,
    APPSTATE_CLOSING
};
uint32_t app_state = APPSTATE_RESET; // main state machine

uint32_t ticker_timeout = 1;
void ticker_isr() {
    ticker_timeout++;
}

char url_buf[256];
int main(void) {
    pc.baud(115200);
    pc.printf("INIT\n");
    
    t.attach(&ticker_isr, DATA_PERIOD);
    
    at.set_init_prefix("ready");

    wait(1.0);
    
    at._uart.printf("AT+RST\r\n");
    

    
    while(1) {
        //at._rx_irq();
        
        switch(app_state) {
            case APPSTATE_RESET:
                if (_esp_status == ESP_IDLE) {
                    at._uart.printf("AT+RST\r\n");
                    _esp_status = ESP_WAITING;
                }
                
                else if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_INIT) {
                        app_state++;
                        _esp_status = ESP_IDLE;
                        pc.printf("INITTED\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;
            
            case APPSTATE_SETMODE:
                if (_esp_status == ESP_IDLE) {
                    at._uart.printf("AT+CWMODE=1\r\n");
                    _esp_status = ESP_WAITING;
                }
                else if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_OK) {
                        _esp_status = ESP_IDLE;
                        app_state++;
                        pc.printf("SET MODE OK\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;
            
            case APPSTATE_CONNECT_WIFI:
                if (_esp_status == ESP_IDLE) {
                    at._uart.printf("AT+CWJAP=\"%s\",\"%s\"\r\n", WIFI_SSID, WIFI_PASSWORD);
                    _esp_status = ESP_WAITING;
                }
                else if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_OK) {
                        _esp_status = ESP_IDLE;
                        app_state++;
                        pc.printf("WIFI OK\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;
                
            case APPSTATE_SETDNS:
                if (_esp_status == ESP_IDLE) {
                    at._uart.printf("AT+CIPDNS=1,\"8.8.8.8\"\r\n");
                    _esp_status = ESP_WAITING;
                }
                else if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_OK) {
                        _esp_status = ESP_IDLE;
                        app_state++;
                        pc.printf("DNS OK\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;

            case APPSTATE_STANDBY:
                if (_esp_status == ESP_IDLE) {
                    if (ticker_timeout) {
                        ticker_timeout = 0;
                        _esp_status = ESP_IDLE;
                        app_state = APPSTATE_OPEN_TCP;
                    }
                }
                break;
                
            case APPSTATE_OPEN_TCP:
                if (_esp_status == ESP_IDLE) {
                    at._uart.printf("AT+CIPSTART=\"TCP\",\"n-blocks.net\",80\r\n");
                    _esp_status = ESP_WAITING;
                }
                else if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_OK) {
                        _esp_status = ESP_IDLE;
                        app_state++;
                        pc.printf("TCP OK\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;

            case APPSTATE_SEND_DATA:
                if (_esp_status == ESP_IDLE) {
                    sprintf(url_buf, "GET /sensor_test/receive.php?value=%f HTTP/1.1\r\nHost: www.n-blocks.net\r\n\r\n", adc.read());
                    
                    at._uart.printf("AT+CIPSEND=%d\r\n", strlen(url_buf));
                    _esp_status = ESP_WAITING;
                }
                else if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_OK) {
                        _esp_status = ESP_IDLE;
                        app_state++;
                        pc.printf("SEND OK\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;
                
            case APPSTATE_SENDING_DATA:
                if (_esp_status == ESP_IDLE) {
                    at.set_custom_prefix("SEND OK");
                    at._uart.printf(url_buf);
                    _esp_status = ESP_WAITING;
                }
                else if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_CUSTOM) {
                        at.set_custom_prefix("RECEIVE OK");
                        _esp_status = ESP_WAITING;
                        app_state = APPSTATE_RECEIVING_RESPONSE;
                        pc.printf("DATA SENT OK\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;
                
            case APPSTATE_RECEIVING_RESPONSE:
                if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_CUSTOM) {
                        _esp_status = ESP_IDLE;
                        app_state = APPSTATE_CLOSING;
                        pc.printf("DATA RECEIVED OK\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;

            case APPSTATE_CLOSING:
                if (_esp_status == ESP_IDLE) {
                    at._uart.printf("AT+CIPCLOSE\r\n");
                    _esp_status = ESP_WAITING;
                }
                else if (_esp_status == ESP_RECEIVED) {
                    if (_esp_last_response == AT_RESPONSE_OK) {
                        _esp_status = ESP_IDLE;
                        app_state = APPSTATE_STANDBY;
                        pc.printf("-- DONE\n");
                    }
                    else {
                        // Not the answer we are looking for
                    }
                }
                break;
                
        }


                        


        
        if (at.has_data()) {
            char buf[1024];
            _esp_last_response = at.process(buf);
            _esp_status = ESP_RECEIVED;
            pc.printf("Resp: %d - \"%s\"\n", _esp_last_response, buf);
        }
        else {
            
        }
    }
    
}
