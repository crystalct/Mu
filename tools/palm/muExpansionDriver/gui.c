/*dont include this anywhere else*/
#include "MuExpDriverRsc.h"

#include "debug.h"
#include "config.h"
#include "sdkPatch/PalmOSPatched.h"
#include <stdint.h>


static Boolean appHandleEvent(EventPtr eventP){
   switch(eventP->eType){
      case frmLoadEvent:{
         FormType* form = FrmInitForm(eventP->data.frmLoad.formID);
         
         FrmSetActiveForm(form);
         return true;
      }
         
      case frmOpenEvent:{
         FormType* form = FrmGetActiveForm();
         
         FrmDrawForm(form);
         return true;
      }
         
      case ctlExitEvent:
         if(eventP->data.ctlExit.controlID == GUI_BUTTON_SAVE_AND_QUIT){
            /*save and quit button released*/
            /*TODO*/
         }
         return true;
         
      default:
         return false;
   }
}

void showGui(uint32_t* configFile){
   EventType event;
   Err unused;
   FormType* currentWindow = NULL;
   
   debugLog("Attempting to load GUI.\n");
   
   /*popup warning dialog*/
   if(!configFile[USER_WARNING_GIVEN]){
      /*continue if user pressed "No", not a real device*/
      if(!FrmAlert(GUI_ALERT_USER_WARNING)){
         configFile[USER_WARNING_GIVEN] = true;
         configFile[DRIVER_ENABLED] = true;
      }
   }
   
   /*set starting window*/
   FrmGotoForm(GUI_FORM_MAIN_WINDOW);
   
   do{
      EvtGetEvent(&event, evtWaitForever);
      
      if(!appHandleEvent(&event))
         if(!SysHandleEvent(&event))
            if(!MenuHandleEvent(0, &event, &unused))
               FrmDispatchEvent(&event);
   }
   while(event.eType != appStopEvent);
}
