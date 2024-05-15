/****************************************************************************
*
* (C) 2000 by BECK IPC GmbH
*
*  BECK IPC GmbH
*  Garbenheimerstr. 38
*  D-35578 Wetzlar
*
*  Phone : (49)-6441-905-240
*  Fax   : (49)-6441-905-245
*
* ---------------------------------------------------------------------------
* Module        : HTML.C
* Function      : contains functions to operate between HTML files and SC12
*                 Cgi Interface. It read Html files, copy it into memory
*                 block and save positions of '^VARDEF[x]' entry in var_field
*                       struct to insert dynamtic code int the static file.

                  By Example:
                  On the flash exists the following htm file (a:\exp.htm):
                  _______________________________________________________

                  <HTML><HEAD><TITLE>Example Page</TITLE></HEAD>
                  <BODY>
                  The first Dynamic code/text ^VARDEF[0] = 32 <BR>
                  And now the Dynamic code/text ^VARDEF[1] = 3 <BR>
                  </BODY>
                  </HTML>
                  _______________________________________________________

                  now you have to call the function in the following way:






                  #include "html.h"

                  vard_field_t exp_vardef[2]; // size "2": 2 var defs in htm file
                  char         exp_page[1024]; // html code buffer (1024 bytes)

                  ....
                  ....
                  ..

                  int main(void)
                  {
                     char tmpBuffer[1024];
                     ...




                     exp_page = Gen_HtmlCode_From_File( "a:\exp.htm", &exp_vardef, 2 );

                     // know, if exp != Null the page is in the memory block.
                     // the Vardef field are filled with blank spaces (ASCII 32).
                     // The exp_vardef struct points to this vardefs fields.
                     // So you can access to this empty fields in the
                     // following way (normaly the filling of the Vardefs will
                     // be done in the CGI function):

                     // 1. text:
                     strcpy( tmpBuffer, "I am the First text" );
                     FILL_STR( tmpBuffer, exp_vardef[0].length ); // this will
                                        // to fill the not used characters of
                                        // the string with blank spaces
                     memcpy( exp_vardef[0].ptr, tmpBufferm, exp_vardef[0].length );


                     // 2. text:
                     strcpy( tmpBuffer, "I am the First text" );
                     FILL_STR( tmpBuffer, exp_vardef[0].length ); // this will
                                       // to fill the not used characters of
                                       // the string with blank spaces
                     memcpy( exp_vardef[0].ptr, tmpBufferm, exp_vardef[0].length );

                     .....
                     ..
                     return;

                  }

*
* Author        : Stoidner
* Date          : 30.08.00
* Version       : V1.00
* ---------------------------------------------------------------------------
* History       :
*
*  Vx.yy        Author  Changes
*
*  V1.00          cs    Create
**************************************************************/


//*******************
//* standard includes

#include "mbed.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "EthernetInterface.h"
#include <stdlib.h>
#include <string.h>

//********************
//* specified includes
#include "html.h"

#include "rtos.h"


#define _HTML_DEBUG
#define _IP_DEBUG
#define _TCP_DEBUG
#define _HTTP_DEBUG
# define _HTML_DEBUG

extern var_field_t tab_balise[10];  //une balise est présente dans le squelette


EthernetInterface eth; // ethernet layer management object
TCPSocketServer svr; //
bool serverIsListened = false;// flag to indicate server is listening
#define PORT   80 // tcp port for socket listeningt 
TCPSocketConnection client;// socket to identify client side
bool clientIsConnected = false;// flag that indicate if a web client is avalaible


char *html_page;    // pointer to memory area of Html Page
/*************** locals prototypes ***************/






//char * Gen_HtmlCode_From_File( char *Path, var_field_t *var_field, int max_vardef )
char * Gen_HtmlCode_From_File( char *Path, var_field_t *var_field, int max_vardef )
{LocalFileSystem local("local"); 
    FILE *in;               // File Handle
    //fpos_t filepos;     // file position counter
     long  filepos;     // file position counter
   int data;            // read data from file
   char value_str[40];   // to search for '^VARDEF[x]' entrys
   int i=0;                 // VarDef Counter ( '^VARDEF[i]' )
   long  mem_pos = -1;  // actually mem offset
   //char *html_page;    // pointer to memory area of Html Page

   if ((in = fopen(Path, "rt"))   == NULL)
   {
    printf("\r\nError Open File: -%s-", Path);
    return(NULL);
   }

    #ifdef _HTML_DEBUG
   printf("\r\nOpen File.. \"%s\"", Path);
   #endif

   // checking Html Page size and filling var_field struct
   while (!feof(in))
   {
      // serach for Commands
      mem_pos++;

        // check for unexpected page size
        if (mem_pos >= 0xFFFF)
      {
        fclose(in);
         return(NULL);
      }

        if ( fgetc(in) == CMD_HTML_SIGN )
      {
         //fgetpos(in, &filepos);
         filepos= ftell ( in );// replace fgetpos call
         
         
        if ( fscanf(in , "VARDEF[ %d ] = %s", &i, &value_str ) )
         {
                #ifdef _HTML_DEBUG
               printf("\r\nFound and save entry..");
                #endif
            mem_pos += atoi(value_str)-1;
         }
         else fseek( in,  filepos, SEEK_SET );
      }

   } // while (!feof

    // allocate memory
   html_page = (char *) malloc( (int) mem_pos+1 );

   // check if alloc was successful
   if (html_page == NULL) return(NULL);

   // jump to beginning of html file
   fseek( in, 0, SEEK_SET);

   // now loading website into memory
   mem_pos = 0;
    #ifdef _HTML_DEBUG
   printf("\r\nReading Page");
   #endif
   while (!feof(in))
   {
      data = fgetc(in);

      // serach for Commands
        if ( data == CMD_HTML_SIGN )
      {
        // fgetpos(in,  &filepos);
         filepos =ftell ( in );// replace fgetpos call
         
        if ( fscanf(in , "VARDEF[ %d ] = %s", &i, &value_str ) )
         {
            if (var_field)
            {
                // save VarDef

               // check if vardef index is valid
               if ( i>max_vardef ) { free(html_page); return(NULL); };
               //var_field[i].length = atoi(value_str);
               var_field[i].length=strlen(value_str);
                var_field[i].ptr    = &html_page[(int)mem_pos];
            }

            // insert free characters
           // for (i=0; i<=atoi(value_str)-1; i++)
            for(i=0;i<=strlen(value_str);i++)
            {
                html_page[(int)mem_pos] = (char) 32;
               mem_pos++;
            }

         }
         else
         {
           // set data pointer back
            fseek( in, (long) filepos, SEEK_SET);

            // copy data into memory
            html_page[(int)mem_pos] = (char)data;
              mem_pos++;
         }

      }
        else
      {
        // copy data into memory
        html_page[(int)mem_pos] = (char)data;
          mem_pos++;
    }

   } // while (!feof

   // terminate string
   html_page[(int)mem_pos-1] = (char)'\0';



   // close HTML Page
   fclose(in);

    #ifdef _HTML_DEBUG
   printf("\r\nDone.");
   #endif


   return(html_page);
}
//simple function thatr load file from disk and return pointer

char *load_HtmlCode_From_File( char *Path,long * size)
{LocalFileSystem local("local"); 
    FILE *in;               // File Handle
   int data;            // read data from file
   long  mem_pos = -1;  // actually mem offset
   char *html_page;    // pointer to memory area of Html Page
   if ((in = fopen(Path, "rt"))   == NULL)
   {    printf("\r\nError Open File: -%s-", Path); //open file from disk
    return(NULL);
   }

   while (!feof(in))  // checking Html Page size
     {
      mem_pos++;
        if ( (fgetc(in) == EOF)||(mem_pos==0xFFFF) )
      {break;}
     }// while (!feof )
   html_page = (char *) malloc( (int) mem_pos+1 );// allocate memory

   // check if alloc was successful
   if (html_page == NULL) return(NULL);
   fseek( in, 0, SEEK_SET);// jump to beginning of html file

   mem_pos = 0; // now loading website into memory
   while (!feof(in))
   { data = fgetc(in);
      html_page[(int)mem_pos] = (char)data;
        mem_pos++;
   } // while (!feof

   html_page[(int)mem_pos-1] = (char)'\0'; // terminate string
   *size=mem_pos-1; //return size of file
   fclose(in); // close HTML Page
   return(html_page);
}

void Html_Patch ( var_field_t *pTab_Balise,int index, char * pChaine ){

    FILL_STR( pChaine,pTab_Balise[index].length );
   memcpy( pTab_Balise[index].ptr, pChaine, pTab_Balise[index].length );
}

void (*pfPtr) (void);



int Init_Web_Server(void (*fPtr)(void) )// fptr pointer to a void interrupt CGI function 
  {pfPtr=fPtr;// affectation du pointeur public
 
    //setup ethernet interface
    eth.init(); //Use DHCP
    eth.connect();
     #ifdef _IP_DEBUG
    printf("IP Address is %s\n\r", eth.getIPAddress());// display server ip address
 #endif
     // Set the callbacks for Listening
  //srv.setOnEvent(&onListeningTCPSocketEvent);  // create a call back fonction associated with listning socket
    //setup srv tcp socket
    if(svr.bind(PORT)< 0) {
        #ifdef _IP_DEBUG
        printf("tcp server bind failed.\n\r");
        #endif
        return -1;
    } else {
        #ifdef _IP_DEBUG
        printf("tcp server bind successed.\n\r");
        #endif
        serverIsListened = true;
    }
 
    if(svr.listen(1) < 0) {
        #ifdef _IP_DEBUG
        printf("tcp server listen failed.\n\r");
        #endif
        return -1;
    } else {
        #ifdef _IP_DEBUG
        printf("tcp server is listening...\n\r");
        #endif
      
return 0;
    }
    
 }
   

//************* used to close srv socket and web server thread ***********************
int DeInit_Web_Server(void)
{int iError;
    serverIsListened=false;// 
iError=svr.close (true);// close main server socket
return iError;
 
 }
 
  
          
 



/***************** thread section **************/

 
void Web_Server_Thread(void const *args) 
{ 
   
            printf("web server thread start.. \n\r");
    
    while ( serverIsListened) {
        //blocking mode(never timeout)
        if(svr.accept(client)<0) {
             #ifdef _TCP_DEBUG
            printf("failed to accept connection.\n\r");
            #endif
        } else {
            #ifdef _TCP_DEBUG
            printf("connection success!\n\rIP: %s\n\r",client.get_address());
            #endif
            clientIsConnected = true;
             // Setup the new socket events
       // client.setOnEvent(&onConnectedTCPSocketEvent);
        // We can find out from where the connection is coming by looking at the
        // Host parameter of the accept() method
            
            while(clientIsConnected) {
                char buffer[1024] = {};
              
                switch(client.receive(buffer, 1023)) {
                    case 0:
                    #ifdef _TCP_DEBUG
                        printf("recieved buffer is empty.\n\r");
                    #endif
                        clientIsConnected = false;
                        break;
                    case -1:
                    #ifdef _TCP_DEBUG
                     printf("failed to read data from client.\n\r");
                    #endif
                        clientIsConnected = false;
                        break;
                    default:
                    #ifdef _HTTP_DEBUG
                        printf("Recieved Data: %d\n\r\n\r%.*s\n\r",strlen(buffer),strlen(buffer),buffer);
                    #endif
                        if(buffer[0] == 'G' && buffer[1] == 'E' && buffer[2] == 'T' ) {
                            #ifdef _HTTP_DEBUG
                            printf("GET request incomming.\n\r");
                            #endif
                            //setup http response header & data
                            /************* patch all dynamic data in html response page ************/
                            //giCounter++;
                            
                            (*pfPtr)();// call to external CGI function located in main
                           
                        /*  char ma_chaine4[20]={};// needed to form html response  
                         sprintf (ma_chaine4,"%d",(int)15);// convert speed as ascii string
                            Html_Patch (tab_balise,0,ma_chaine4);// patch first label with dyn.string
                            
                            sprintf (ma_chaine4,"%d",(int)16);// convert count as ascii string
                            Html_Patch (tab_balise,1,ma_chaine4);// patch first label with dyn.string
                            
                             sprintf (ma_chaine4,"%d",17);// convert count as ascii string
                            Html_Patch (tab_balise,2,ma_chaine4);// patch first label with dyn.string
                            
                             sprintf (ma_chaine4,"%d",18);// convert count as ascii string
                            Html_Patch (tab_balise,3,ma_chaine4);// patch first label with dyn.string
                           
                             sprintf (ma_chaine4,"%d",19);// convert count as ascii string
                            Html_Patch (tab_balise,4,ma_chaine4);// patch first label with dyn.string
                            
                             */
    
    

                           
                            char echoHeader[256] = {};
                            sprintf(echoHeader,"HTTP/1.1 200 OK\n\rContent-Length: %d\n\rContent-Type: text\n\rConnection: Close\n\r",strlen(html_page));
                            client.send(echoHeader,strlen(echoHeader));
                            #ifdef _HTTP_DEBUG
                             printf("%s",echoHeader);// debut http header sent
                            #endif                          
                            //client.send(buffer,strlen(buffer));// realise echo request to client 
                             
                                  
                            client.send(html_page,strlen(html_page));// realise echo request to client 
                           #ifdef _HTTP_DEBUG
                            printf("%s",html_page);
                           #endif
                            
                            clientIsConnected = false;// close connection with this client at end of http response
                           #ifdef _HTTP_DEBUG
                            printf("web page sent.\n\r");
                            #endif
                        }
                        break;
                }// end case
           //Thread::wait(10);// sleep for 10 ms
            }// end while Client is Connected
           #ifdef _HTTP_DEBUG
            printf("close connection.\n\rtcp server is listening...\n\r");
            #endif
            client.close();
           
       
        }
    Thread::wait(10);// sleep for 10 ms
   } // end while ServerIs listening
}
 



    
    
    
    
    