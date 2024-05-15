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
* Module        : HTML.H
* Function      : contains prototypes and definitions for html.c
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

#ifndef _HTML_H_

#define _HTML_H_


//*************
//* definitions
#define CMD_HTML_SIGN       '^'      /* Command sign which significates a
                                       command start in the HTML code */
#define FILL_STR( str, length )   while ( strlen(str) < length ) strcat(str, " ")
                                    /* useful macro to fill a string to a specified
                                       length with NULL Chars */ 
//******************
//* type definitions
typedef struct {
//                      int pos;    // pos of the VarDef relativ to the Page beginning
                  char *ptr;  // pointer to beginning of VarDef definition
                  int length; // length of the variable field
               } var_field_t;


typedef var_field_t *vf_ptr;

//************
//* prototypes

//char *Gen_HtmlCode_From_File( char *Path, var_field_t *var_field, int max_vardef );
char * Gen_HtmlCode_From_File( char *Path, var_field_t *var_field, int max_vardef );
char *load_HtmlCode_From_File( char *Path,long * size);
void Html_Patch ( var_field_t *pTab_Balise,int index, char * pChaine ) ;
int Init_Web_Server(void (*)(void) );// used to initialize web, ethernet and pointer to cgi function 
int DeInit_Web_Server(void);// used to close main server socket and web server thread
void Web_Server_Thread(void const *args) ;
#endif
