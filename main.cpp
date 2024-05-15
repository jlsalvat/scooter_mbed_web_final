#include "EthernetInterface.h"
#include <stdlib.h>
#include <string.h>
#include "mbed.h"
#include "rtos.h" // need for main thread sleep
#include "html.h" // need for html patch working with web server


/************ persistent file parameters section *****************/
LocalFileSystem local("local");               // Create the local filesystem under the name "local"


Serial pc(USBTX, USBRX); // tx, rx




/********************* web server section **********************************/
var_field_t tab_balise[10];  //une balise est présente dans le squelette


void CGI_Function(void)   // cgi function that patch web data to empty web page
{

    static int counter;
    static float dixieme;

    char ma_chaine4[20]= {}; // needed to form html response
    sprintf (ma_chaine4,"%d",counter);// convert speed as ascii string
    Html_Patch (tab_balise,0,ma_chaine4);
    counter++;

    sprintf (ma_chaine4,"%.2f",dixieme);// convert speed as ascii string
    Html_Patch (tab_balise,1,ma_chaine4);
    dixieme+=0.1;

}



//*************************** main function *****************************************
int main()
{
    char cChoix;
//***************************************** web section ********************************************/
    Init_Web_Server(&CGI_Function); // create and initialize tcp server socket and pass function pointer to local CGI function
    Thread WebThread(Web_Server_Thread);// create and launch web server thread
    /********* main cgi function used to patch data to the web server thread **********************************/
    Gen_HtmlCode_From_File("/local/pagecgi.htm",tab_balise,2);// read and localise ^VARDEF[X] tag in empty html file
//******************************************* end web section  ************************************* /

    while(cChoix!='q' and cChoix!='Q') {
        pc.printf(" veuillez saisir un choix parmi la liste proposee: \n\r");
        pc.printf(" a:Mode Manuel\n\r");
        pc.printf(" b:Mode Auto \n\r");
        pc.printf(" c:Calibration \n\r");
        pc.printf(" q:Quitter \n\r");

        /************* multithreading : main thread need to sleep in order to allow web response */
        while (pc.readable()==0) { // determine if char available
            Thread::wait(10);   // wait 10 until char available on serial input
        }
        /************* end of main thread sleep  ****************/

        pc.scanf(" %c",&cChoix);
        switch (cChoix) {
            case 'a':

                break;

            case 'b':

                break;

            case 'c':

                break;
        }
    } // end while

    //************** thread deinit *********************
    DeInit_Web_Server();

    pc.printf(" fin programme scooter mbed \n\r");
} // end main
