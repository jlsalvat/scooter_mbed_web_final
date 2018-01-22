#include "EthernetInterface.h"
#include <stdlib.h>
#include <string.h>
#include "mbed.h"
#include "rtos.h" // need for main thread sleep
#include "html.h" // need for html patch working with web server
#include "bloc_io.h"
#define RADIUS  0.22F // wheel size
#define NBPOLES 19 // magnetic pole number
#define DELTA_T 0.2F // speed measurement counting period
#define T_TICKER 0.05 // time for ticker to call function Gaz
#define PI 3.1415 // pie value
#define RESISTANCE // resistance value
#define T_COEF 0.01 // constant for temperature calculation
#define T_REST 0.2 // constant for current mesure 
#define INIT_IREST 60000 // capacite initial en mAs

Bloc_IO MyPLD(p25,p26,p5,p6,p7,p8,p9,p10,p23,p24);// instantiate object needed to communicate with PLD
AnalogIn Val_Poignee(p17);// analog input for gaz connected to mbed
AnalogIn Vbat(p18);// analog input for battery connected to mbed
AnalogIn Vtemp(p19);// analog input for temperature connected to mbed
AnalogIn VImes(p20);// analog input for current connected to mbed
DigitalOut Valid_PWM(p21);// valid pmw mbed pin
InterruptIn Valid_PWM_In(p22); // count value for pole number
Serial pc(USBTX, USBRX); // tx, rx
// Top_Hall Pin




/************ persistent file parameters section *****************/
LocalFileSystem local("local");               // Create the local filesystem under the name "local"







/********************* web server section **********************************/
var_field_t tab_balise[10];  //une balise est présente dans le squelette
int giCounter=0,giCounter2=0;// acces counting
int giMode=0,giFlag=0;//to validate between mode automatic or manual
float gfNmax=0.0,gfNmin=0.0,gfVit=0.0;//initialising values minimum and maximum for equation
int giBrake = 0;
int giPwm=0; //value initial for PWM mode manuel
float gfPwm=0.0; //value initial for PWM mode auto
float gfPwm_final=0.0; //value initial for incrementation PWM mode auto
float gfVit_bride; //speed value (bride) in mode auto
float gfVbat,gfIbat,gfTemp,gfPourc,gfGIrest,gfImes;
float gfIrest = 60000;

Ticker Tick;//Ticker to call function Gaz
Ticker Tick2;//Ticker to call function Vitesse
Ticker Tick3;//Ticker to call function Imesure

/*********************** can bus section  ************/
// determine message ID used to send Gaz ref over can bus
#define _CAN_DEBUG // used to debug can bus activity
#define USE_CAN_REF // uncomment to receive gaz ref over can_bus
CAN can_port (p30, p29); // initialisation du Bus CAN sur les broches 30 (rd) et 29(td) for lpc1768 + mbed shield
bool bCan_Active=false;
int iCounter=0;// a counter for speed counting


DigitalOut led1(LED1); //initialisation des Leds présentes sur le micro-controleur Mbed*/
DigitalOut led2(LED2);
DigitalOut led3(LED3); // blink when can message is sent
DigitalOut led4(LED4); // blink when can message is received





//************ local function prototypes *******************

void Imessure ()  //function for mesuring battery current capacity to be called every 200ms
{

    int ic; // local counter

    ic++;
    if(ic>5) { // to wait for calculation after 1s
        gfIrest = gfIrest - 1000; // because every 1s, the battery consume 1000
        gfGIrest = (gfIrest/INIT_IREST)*100;

        if(gfGIrest>100)
            gfGIrest = 100;
        if(gfGIrest<0)
            gfGIrest=0;

        ic=1;
        gfImes = 0;
    }
    //gfImes = (gfImes + gfVbat)/ic ;
}


void Affiche(void) //function only for display
{
    gfVbat=(25*Vbat.read())/0.33; // battery tension value
    gfIbat=(33000*VImes.read())-13510; // current value
    gfTemp=((Vtemp.read()*3.3)/T_COEF)-273; // surrounding temperature
    gfPourc=((gfPwm*100)/255); // pedal turning percentage

    pc.printf(" Vitesse = %g m/h\n\r",gfVit); //speed diplay
    pc.printf(" Vbat=%.3g \n\r",gfVbat); // battery tension display
    pc.printf(" Ibat=%.2f mA\n\r",gfIbat); //current value display
    pc.printf(" Temperature = %g C\n\r",gfTemp); // surrounding temperature display
    pc.printf(" Pourcentage = %.0f %% \n\r",gfPourc); //pedal turning percentage display
    pc.printf(" Capacite = %.0f %% \n\r",gfGIrest); // battery capacity display

}

void Registre_PLD(void) // function for internal checkups display 
{

    /**** association with block io****/
    int iSect = MyPLD.read() & 7;
    int iDir = MyPLD.read() & 8;
    int iFLTA = MyPLD.read() & 16;
    int iOver_Current = MyPLD.read() & 64;

    pc.printf("\n\r\n\r");

    pc.printf(" Secteur : %d\n\r" , iSect);
    
    //direction
    if ( iDir == 0)
        pc.printf(" Direction = ARRIERE \n\r");
    else {
        pc.printf(" Direction = AVANCE \n\r");
    }
    //braking system
    if ( giBrake == 0)
        pc.printf(" Brake = ON \n\r");
    if ( giBrake != 0)
        pc.printf(" Brake = OFF \n\r");
    //internal error
    if ( iFLTA == 1)
        pc.printf(" Il y a un erreur \n\r");
    if ( iFLTA != 1)
        pc.printf(" Pas d'erreur \n\r");
    //overcurrent
    if ( iOver_Current == 0)
        pc.printf(" Over Current \n\r");
    if ( iOver_Current != 0)
        pc.printf(" Normal Current  \n\r");

    pc.printf("\n\r");

}



/**************** Read persistent data from text file located on local file system ****************/
void Read_file(void)
{
    FILE* Calibrage = fopen("/local/TEST.txt","r");

    if(Calibrage != NULL) {
        pc.printf(" OK\n\r");
        fscanf (Calibrage, "%g %g %g %g", &gfNmin,&gfNmax,&gfVit_bride);
        pc.printf(" Nmin = %.3g \n\r",gfNmin);
        pc.printf(" Nmax = %.3g \n\r",gfNmax);
        pc.printf(" Vbride = %.2f\n\r",gfVit_bride);

    } else {

        pc.printf(" KO\n\r");
    }

    fclose(Calibrage);
}

//************** calibation gaz function needed to record min_gaz and max_gaz value to persistent text file  ******************
void Calibration(void)
{
    char cNext=0; //a variable just to exit command scanf

    FILE* Calibrage =fopen("/local/TEST.txt","w"); //opening file
    if(Calibrage != NULL) { //validation for file existence
        pc.printf(" OK\n\r");
        switch (giFlag) {
            case 0 ://calibration of pedal

                pc.printf(" lachez la poignee, appuyez 'k' et 'entree' \n\r");
                pc.scanf(" %c",&cNext);
                gfNmin = Val_Poignee.read();
                pc.printf(" Nmin = %.3g \n\r",gfNmin);

                pc.printf(" enfoncez la poignee, appuyez 'k' et 'entree'\n\r");
                pc.scanf(" %c",&cNext);
                gfNmax = Val_Poignee.read();
                pc.printf(" Nmax = %.3g \n\r",gfNmax);
                break;

            case 1 ://calibration of maximum speed
                pc.printf(" veuillez saisir la vitesse de la bride : \n\r");
                pc.scanf(" %f",&gfVit_bride);
                break;


        }
    } else {
        pc.printf(" KO\n\r");
    }
    fprintf(Calibrage,"%f %f %f %f",gfNmin,gfNmax,gfVit_bride,gfIrest);//saving infos in text file
    fclose(Calibrage);
}



// ************top hall counting interrupt needed for speed measurement
void toggle(void)
{
    giCounter=giCounter+1;

}

//********************** timer interrupt for speed measurement each 100ms  *************************
void Vitesse(void)
{
    giCounter2=giCounter;
    giCounter=0;

    gfVit=((float)giCounter2*2*PI*RADIUS*3600)/(6*NBPOLES*DELTA_T);// speed calculation
}

/*********************** INITIALISATION **********************/
void Initial(void)
{
    Valid_PWM.write(0);
    MyPLD.write(0);
    Valid_PWM.write(1);
    Read_file();
    Valid_PWM_In.mode(PullUp);

}


//********************* Timer Interrupt for gaz ref management each 10ms   ********************
void Gaz(void)
{
    giBrake = MyPLD.read() & 32;


    switch (giMode) {
        case 0 ://mode manuel

            if (giPwm<0)
                giPwm=0;
            if (giPwm>255)
                giPwm=255;

            MyPLD.write(giPwm);
            break;
        case 1 :// mode auto

            gfPwm = 255*(Val_Poignee.read()-gfNmin)/(gfNmax-gfNmin);
            if (gfPwm<0)
                gfPwm=0;
            if (gfPwm>255)
                gfPwm=255;

            //safety
            if ( giBrake == 0) {
                gfPwm_final = 0;
            } else  if (gfVit >= gfVit_bride) {
                if (gfPwm_final >= 1.0)
                    gfPwm_final = gfPwm_final-1.0;
            } else if (gfPwm_final < gfPwm) {
                gfPwm_final = gfPwm_final+1.0;
            }  else if (gfPwm_final >= gfPwm) {
                gfPwm_final = gfPwm_final-1.0;
            }


            MyPLD.write(gfPwm_final);
            break;
    }
}


/********* main cgi function used to patch data to the web server thread **********************************/





void CGI_Function(void)   // cgi function that patch web data to empty web page
{

    gfVbat=(25*Vbat.read())/0.33;
    gfIbat=(33000*VImes.read())-13510;
    gfTemp=((Vtemp.read()*3.3)/T_COEF)-273;
    gfPourc=((gfPwm*100)/255);

    char ma_chaine4[20]= {}; // needed to form html response
    sprintf (ma_chaine4,"%d",iCounter);// convert speed as ascii string
    Html_Patch (tab_balise,0,ma_chaine4);
    iCounter++;

    sprintf (ma_chaine4,"%f",gfVbat);
    Html_Patch (tab_balise,1,ma_chaine4);// patch first label with dyn.string

    sprintf (ma_chaine4,"%.4f",gfIbat);
    Html_Patch (tab_balise,2,ma_chaine4);

    sprintf (ma_chaine4,"%.4f",gfTemp);
    Html_Patch (tab_balise,3,ma_chaine4);

    sprintf (ma_chaine4,"%.4f",gfPourc);
    Html_Patch (tab_balise,4,ma_chaine4);

    sprintf (ma_chaine4,"%.4f",gfVit);
    Html_Patch (tab_balise,5,ma_chaine4);
    
    sprintf (ma_chaine4,"%.4f",gfGIrest);
    Html_Patch (tab_balise,6,ma_chaine4);


}


/*********************** CAN BUS SECTION  **********************/



void CAN_REC_THREAD(void const *args)
{
    int iCount,iError;

    while (bCan_Active) {
        Thread::wait(100);// wait 100ms

    }

}



//*************************** main function *****************************************
int main()
{
    //pc.printf("\n demarrage\n\r");
    Initial();

    Valid_PWM_In.rise(&toggle);
    Tick.attach(&Gaz,T_TICKER);
    Tick2.attach(&Vitesse,DELTA_T);
    Tick3.attach(&Imessure,T_REST);

    char cChoix=0;

//***************************************** web section ********************************************/
    Init_Web_Server(&CGI_Function); // create and initialize tcp server socket and pass function pointer to local CGI function
    Thread WebThread(Web_Server_Thread);// create and launch web server thread
    /********* main cgi function used to patch data to the web server thread **********************************/
    Gen_HtmlCode_From_File("/local/pagecgi4.htm",tab_balise,7);// read and localise ^VARDEF[X] tag in empty html file
//******************************************* end web section  ************************************* /




//********************* can bus section initialisation *******************************************
    bCan_Active=true;// needed to lauchn CAN thread
    Thread CanThread(CAN_REC_THREAD);// create and launch can receiver  thread
//********************* end can bus section *****************************************************


    while(cChoix!='q' and cChoix!='Q') {
        pc.printf(" veuillez saisir un choix parmi la liste proposee: \n\r");
        pc.printf(" a:Mode Manuel\n\r");
        pc.printf(" b:Mode Auto \n\r");
        pc.printf(" c:Calibration \n\r");
        pc.printf(" d:Registre PLD \n\r");
        pc.printf(" e:Affiche vitesse \n\r");
        pc.printf(" f:Reset capacite \n\r");
        pc.printf(" q:Quitter \n\r");

        /************* multithreading : main thread need to sleep in order to allow web response */
        while (pc.readable()==0) { // determine if char available
            Thread::wait(10);   // wait 10 until char available on serial input
        }
        /************* end of main thread sleep  ****************/



        pc.scanf(" %c",&cChoix);
        switch (cChoix) {
            case 'a':
                giMode=0;//activation mode manual 
                pc.printf(" veuillez saisir la valeur Pwm (0 jusqu'au 255) : \n\r");
                pc.scanf("%d",&giPwm);
                break;

            case 'b':
                giMode=1;//activation mode auto
                giFlag=1;//activation calibration for speed limit
                Calibration();
                break;

            case 'c':
                giMode=0;//activation mode manual to stop speed ticker function
                giFlag=0;//activation calibration for pedal 
                Calibration();
                break;

            case 'd':
                giMode=1;//activation mode auto to start speed ticker function
                Registre_PLD();
                break;

            case 'e':
                giMode=1;//activation mode auto
                Affiche();
                break;
                
            case 'f':
                giMode=1;//activation mode auto
                gfIrest=60000; // initialisation 
                break;

            case 'q':
                giMode=0;//activation mode manual
                Valid_PWM.write(0);
                break;
        }
    } // end while

    //************** thread deinit *********************
    // DeInit_Web_Server();
    //bCan_Active=false;
    //CanThread=false;// close can received thread
    pc.printf(" fin programme scooter mbed \n\r");
} // end main
