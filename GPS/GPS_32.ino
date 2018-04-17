#include <time.h>
#include <sys/time.h>

// FreeRTOS includes
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
// UART driver
#include "driver/uart.h"
// Error library
#include "esp_err.h"
// minmea
#include "minmea.h"
#define MAX_LINE_SIZE 255

  float latitude = -1.0;
  float longitude = -1.0;
  int fix_quality = -1;
  int satellites_tracked = -1;

  struct timeval tv;
  struct tm mytm;
  uint8_t line[MAX_LINE_SIZE+1];

  const int ledPin = 2;


void setup() {
  Serial.begin(115200);
  Serial.println(".............................");
  pinMode (ledPin, OUTPUT);
  digitalWrite (ledPin, LOW);

         //setenv("TZ", "CET-1CEST-2,M3.5.0/02:00:00,M10.5.0/03:00:00", 1);
        setenv("TZ", "GMT0GMT0", 1);
        tzset();

}

char* read_line(uart_port_t uart_controller) {

  static char line[MINMEA_MAX_LENGTH];
  char *ptr = line;
  while(1) {
  
    int num_read = uart_read_bytes(uart_controller, (unsigned char *)ptr, 1, portMAX_DELAY);
    if(num_read == 1) {
    
      // new line found, terminate the string and return
      if(*ptr == '\n') {
        ptr++;
        *ptr = '\0';
        return line;
      }
      
      // else move to the next char
      ptr++;
    }
  }
}




void loop() {

  printf("GPS Demo\r\n\r\n");
  

  
  // configure the UART1 controller, connected to the GPS receiver
  uart_config_t uart_config = {
        .baud_rate = 9600,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE
    };
    uart_param_config(UART_NUM_1, &uart_config);
    uart_set_pin(UART_NUM_1, 4, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    uart_driver_install(UART_NUM_1, 1024, 0, 0, NULL, 0);
  
  // GPS variables and initial state

  uint8_t notimeset=1;
  
  // parse any incoming messages and print it
  while(1) {
    
    // read a line from the receiver
    char *line = read_line(UART_NUM_1);
    
    //printf("\nNMEA: %s", line); ///////////////////////////////////////////////////////////////
    
    // parse the line
    switch (minmea_sentence_id(line, false)) {
      
            case MINMEA_SENTENCE_RMC: {
                
        struct minmea_sentence_rmc frame;
        
    if (minmea_parse_rmc(&frame, line)) {
      
      //printf("\nTime: %d:%d:%d %d-%d-%d",frame.time.hours, frame.time.minutes, frame.time.seconds, frame.date.day, frame.date.month, frame.date.year);
        
         
        mytm.tm_hour = frame.time.hours ;
        mytm.tm_min = frame.time.minutes;
        mytm.tm_sec = frame.time.seconds ;
        mytm.tm_mday = frame.date.day;
        mytm.tm_mon = frame.date.month-1;
        mytm.tm_year = frame.date.year+100;


        time_t t =  mktime(&mytm);

        
        //Serial.println(t);
        tv.tv_sec = t;
        tv.tv_usec = 0;
        settimeofday(&tv, 0); 
        time(&t);
        notimeset=0;
        Serial.print("Epoch: ");Serial.print(t);Serial.print("\tSat: ");Serial.println(satellites_tracked);
        
                
        if (getLocalTime(&mytm)) {Serial.println(&mytm, "%Y-%m-%d %H:%M:%S");}
        
                          } else {
        printf("$xxRMC sentence is not parsed\n");
      }
      break;
    
          
          // latitude valid and changed? apply a threshold
          float new_latitude = minmea_tocoord(&frame.latitude);
          if((new_latitude != NAN) && (abs(new_latitude - latitude) > 0.001)) {
            latitude = new_latitude;
            printf("New latitude: %f\n", latitude);
          }
          
          // longitude valid and changed? apply a threshold
          float new_longitude = minmea_tocoord(&frame.longitude);
          if((new_longitude != NAN) && (abs(new_longitude - longitude) > 0.001)) {
            longitude = minmea_tocoord(&frame.longitude);
            printf("New longitude: %f\n", longitude);
          }
                   } break;

            case MINMEA_SENTENCE_GGA: {
                
        struct minmea_sentence_gga frame;
                if (minmea_parse_gga(&frame, line)) {
          
          // fix quality changed?
          if(frame.fix_quality != fix_quality) {
            fix_quality = frame.fix_quality;
            printf("\nNew fix quality: %d\n", fix_quality);
          }
                }
            } break;

            case MINMEA_SENTENCE_GSV: {
                
        struct minmea_sentence_gsv frame;
                if (minmea_parse_gsv(&frame, line)) {

          // number of satellites changed?

if(frame.total_sats > 3 ) {digitalWrite(ledPin, millis()>>4 &1);} else digitalWrite (ledPin, LOW);
          
          if(frame.total_sats != satellites_tracked) {
            satellites_tracked = frame.total_sats;
            
          }
        }
            } break;
      
      default: break;
        }
    }

}
