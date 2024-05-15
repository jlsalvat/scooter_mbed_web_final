#include "mbed.h"
#include "EthernetInterface.h"
#include <stdio.h>
#include <string.h>

#define PORT   80

EthernetInterface eth;

TCPSocketServer svr;
bool serverIsListened = false;

TCPSocketConnection client;
bool clientIsConnected = false;

DigitalOut led1(LED1); //server listning status
DigitalOut led2(LED2); //socket connecting status

Ticker ledTick;


int giCpt=0;                // compteur html
// page html
char html_page[]="<html> " "<head> <title>titre de la page html</title> </head>"
                    "<body>" "CGI : Compteur <br>" "cpt=%d <br>" "</body></html>";

void ledTickfunc()
{
    if(serverIsListened)  {
        led1 = !led1;
    } else {
        led1 = false;
    }
}

int main (void)
{
    ledTick.attach(&ledTickfunc,0.5);

    //setup ethernet interface
    eth.init(); //Use DHCP
    eth.connect();
    printf("IP Address is %s\n\r", eth.getIPAddress());

    //setup tcp socket
    if(svr.bind(PORT)< 0) {
        printf("tcp server bind failed.\n\r");
        return -1;
    } else {
        printf("tcp server bind successed.\n\r");
        serverIsListened = true;
    }

    if(svr.listen(1) < 0) {
        printf("tcp server listen failed.\n\r");
        return -1;
    } else {
        printf("tcp server is listening...\n\r");
    }

    //listening for http GET request
    while (serverIsListened) {
        //blocking mode(never timeout)
        if(svr.accept(client)<0) {
            printf("failed to accept connection.\n\r");
        } else {
            printf("connection success!\n\rIP: %s\n\r",client.get_address());
            clientIsConnected = true;
            led2 = true;
            
            while(clientIsConnected) {
                char buffer[1024] = {};
                switch(client.receive(buffer, 1023)) {
                    case 0:
                        printf("recieved buffer is empty.\n\r");
                        clientIsConnected = false;
                        break;
                    case -1:
                        printf("failed to read data from client.\n\r");
                        clientIsConnected = false;
                        break;
                    default:
                        printf("Recieved Data: %d\n\r\n\r%.*s\n\r",strlen(buffer),strlen(buffer),buffer);
                        if(buffer[0] == 'G' && buffer[1] == 'E' && buffer[2] == 'T' ) {
                            char sendHeader[256] = {};
                            char sendBody[1024];
                            sprintf(sendHeader,"HTTP/1.1 200 OK\n\rContent-Length: %d\n\rContent-Type: text\n\rConnection: Close\n\r",strlen(html_page));
                            client.send(sendHeader,strlen(sendHeader));
                            sprintf(sendBody,html_page,giCpt);
                            client.send(sendBody,strlen(sendBody));
                            #ifdef _HTTP_DEBUG
                             printf("%s",sendHeader);// debut http header sent
                             printf("%s",sendBody);// debut http body
                            #endif                          
                            //client.send(buffer,strlen(buffer));// realise echo request to client 
                            clientIsConnected = false;
                        }
                        break;
                }
            }
            printf("close connection.\n\rtcp server is listening...\n\r");
            client.close();
            led2 = false;
        }
    }
}