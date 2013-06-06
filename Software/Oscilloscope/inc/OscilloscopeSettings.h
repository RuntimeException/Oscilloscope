#ifndef OSCILLOSCOPESETTINGS_H_
#define OSCILLOSCOPESETTINGS_H_

#include "stm32f4xx_conf.h"
#include "OscilloscopeCommonTypes.h"
#include "stdlib.h"

/*========================================== SETTINGS INTERNAL TYPES =========================================*/

typedef int32_t   OSC_Settings_IntegerContinuousValue_Type;
typedef int32_t   OSC_Settings_IntegerDiscreteValue_Type;
typedef uint32_t  OSC_Settings_IntegerDiscreteIndex_Type;
typedef uint8_t   OSC_Settings_OnOffValue_Type;
typedef uint8_t   OSC_Settings_OptionValue_Type;

/*=============================================== SETTINGS TYPES =============================================*/

typedef struct OSC_Settings_IntegerContinuous_Type{
  OSC_Settings_IntegerContinuousValue_Type    value;
  OSC_Settings_IntegerContinuousValue_Type    lowerBound;
  OSC_Settings_IntegerContinuousValue_Type    upperBound;
  OSC_Settings_IntegerContinuousValue_Type    incrementStepSingle;
  OSC_Settings_IntegerContinuousValue_Type    incrementStepMultiple;
  OSC_Settings_IntegerContinuousValue_Type (*callback)(struct OSC_Settings_IntegerContinuous_Type*    this,
                                                       OSC_Menu_Event_Type                            menuEvent);
  char* const name;
  char* const unitName;
} OSC_Settings_IntegerContinuous_Type;


typedef struct OSC_Settings_IntegerDiscrete_Type {
  OSC_Settings_IntegerDiscreteValue_Type*   valueSet;
  OSC_Settings_IntegerDiscreteIndex_Type    currentIndex;
  OSC_Settings_IntegerDiscreteValue_Type    length;
  OSC_Settings_IntegerDiscreteIndex_Type    (*callback)(struct OSC_Settings_IntegerDiscrete_Type*      this,
                                                        OSC_Menu_Event_Type                            menuEvent);
  char** const  nameOfValues;
  char*  const   name;
} OSC_Settings_IntegerDiscrete_Type;


typedef struct OSC_Settings_OnOff_Type {
  OSC_Settings_OnOffValue_Type   status;
  OSC_Settings_OnOffValue_Type   (*callback)(struct OSC_Settings_OnOff_Type*                 this,
                                             OSC_Menu_Event_Type                             menuEvent);
  char** const                   statusNames;
  char*  const                   name;
} OSC_Settings_OnOff_Type;


typedef struct OSC_Settings_Option_Type {
  OSC_Settings_OptionValue_Type    optionID;
  OSC_Settings_OptionValue_Type    optionCount;
  OSC_Settings_OptionValue_Type    (*callback)(struct OSC_Settings_Option_Type*               this,
                                               OSC_Menu_Event_Type                            menuEvent);
  char** const   nameOfOptions;
  char*  const   name;
} OSC_Settings_Option_Type;


/*=========================================== SETTINGS CALLBACK TYPES ========================================*/

/*It must return the current value of the selected settings value*/
typedef OSC_Settings_IntegerContinuousValue_Type  (*OSC_Settings_IntegerContinuousCallback_Type)(OSC_Settings_IntegerContinuous_Type*  this,
                                                                                                 OSC_Menu_Event_Type                   menuEvent);

/*It must return the index of the selected settings value*/
typedef OSC_Settings_IntegerDiscreteIndex_Type (*OSC_Settings_IntegerDiscreteCallback_Type)(OSC_Settings_IntegerDiscrete_Type*         this,
                                                                                            OSC_Menu_Event_Type                        menuEvent);

/*It must return the status of the selected settings*/
typedef OSC_Settings_OnOffValue_Type        (*OSC_Settings_OnOffCallback_Type)(OSC_Settings_OnOff_Type*                                this,
                                                                               OSC_Menu_Event_Type                                     menuEvent);

/*It must return the status of the selected settings option*/
typedef OSC_Settings_OptionValue_Type       (*OSC_Settings_OptionCallback_Type)(OSC_Settings_Option_Type*                              this,
                                                                                OSC_Menu_Event_Type                                    menuEvent);

/*========================================= SETTINGS DEFAULT CALLBACKS =======================================*/
OSC_Settings_IntegerContinuousValue_Type   OSC_Settings_IntegerContinuousCallback_Default(OSC_Settings_IntegerContinuous_Type*  this,
                                                                                          OSC_Menu_Event_Type                   menuEvent);
OSC_Settings_IntegerDiscreteIndex_Type     OSC_Settings_IntegerDiscreteCallback_Default(OSC_Settings_IntegerDiscrete_Type*      this,
                                                                                        OSC_Menu_Event_Type                     menuEvent);
OSC_Settings_OnOffValue_Type               OSC_Settings_OnOffCallback_Default(OSC_Settings_OnOff_Type*                          this,
                                                                              OSC_Menu_Event_Type                               menuEvent);
OSC_Settings_OptionValue_Type              OSC_Settings_OptionCallback_Default(OSC_Settings_Option_Type*                        this,
                                                                               OSC_Menu_Event_Type                              menuEvent);

#endif /* OSCILLOSCOPESETTINGS_H_ */