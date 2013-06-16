#include "OscilloscopeDSP.h"

/*=================================== DATA ACQUISITION MEMORY DEFINITIONS ===================================*/
uint8_t OSC_DSP_Channel_A_DataAcquisitionMemory[OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE];
uint8_t OSC_DSP_Channel_B_DataAcquisitionMemory[OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE];

/*===================================== DMA MODE STRUCTURE DEFINITIONS ======================================*/
static const OSC_Analog_Channel_DataAcquisitionConfig_Type OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PreTrigger = {
    OSC_DSP_Channel_A_DataAcquisitionMemory,
    OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE,
    OSC_Analog_DMA_Mode_Normal
};

static const OSC_Analog_Channel_DataAcquisitionConfig_Type OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PreTrigger = {
    OSC_DSP_Channel_B_DataAcquisitionMemory,
    OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE,
    OSC_Analog_DMA_Mode_Normal
};

static const OSC_Analog_Channel_DataAcquisitionConfig_Type OSC_Analog_Channel_A_DataAcquisitionConfig_Circular_PreTrigger = {
    OSC_DSP_Channel_A_DataAcquisitionMemory,
    OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE,
    OSC_Analog_DMA_Mode_Circular
};

static const OSC_Analog_Channel_DataAcquisitionConfig_Type OSC_Analog_Channel_B_DataAcquisitionConfig_Circular_PreTrigger = {
    OSC_DSP_Channel_B_DataAcquisitionMemory,
    OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE,
    OSC_Analog_DMA_Mode_Circular
};

/*In case of postTrigger sampling the length and the start address changes from data acquisition to data acquisition*/
static OSC_Analog_Channel_DataAcquisitionConfig_Type OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger = {
    0,
    0,
    OSC_Analog_DMA_Mode_Normal
};

static OSC_Analog_Channel_DataAcquisitionConfig_Type OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PostTrigger = {
    0,
    0,
    OSC_Analog_DMA_Mode_Normal
};

/*======================================== STATE MACHINE DEFINITIONS ========================================*/
OSC_DSP_StateMachine_Type  OSC_DSP_StateMachine = {
    OSC_DSP_State_Disabled,                         /*dataAcquisitionState*/              /*COMPUTED*/
    0,                                              /*firstDataPosition*/                 /*COMPUTED*/
    0,                                              /*preTriggerMemoryLength*/            /*UPDATE-FROM-CONFIG*/
    0,                                              /*postTriggerMemoryLength*/           /*UPDATE-FROM-CONFIG*/
    0,                                              /*triggerPosition*/                   /*COMPUTED*/
    127,                                            /*triggerLevel*/                      /*UPDATE-FROM-CONFIG*/
    OSC_Analog_AnalogWatchdog_Range_Invalid,        /*triggerAnalogWatchdogRange*/        /*COMPUTED*/
    OSC_DSP_TriggerSlope_RisingEdge,                /*triggerSlope*/                      /*UPDATE-FROM-CONFIG*/
    OSC_DSP_TriggerSource_Channel_A,                /*triggerSource*/                     /*UPDATE-FROM-CONFIG*/
    OSC_DSP_TriggerState_Disabled,                  /*triggerState*/                      /*COMPUTED*/
    OSC_DSP_SAMPLE_RATE                             /*sampleRate*/                        /*UPDATE-FROM-CONFIG*/
    /*FIXME: It is not used in version 1.0 --> The timer should be adjusted according to this*/
};

/*===================================== WAVEFORM PROPERTIES DEFINITIONS =====================================*/
OSC_DSP_WaveformProperties_Type OSC_DSP_WaveformProperties = {
    0,      /*virtualTriggerPosition*/
    0,      /*samplePerPixel*/
    0,      /*verticalScaleFactorNumerator*/
    0,      /*verticalScaleFactorDenominator*/
    0       /*verticalOffset*/
};

/*====================================== EXTERNAL FUNCTION DEFINITIONS ======================================*/

void OSC_DSP_Init(void){

}

void OSC_DSP_StartDataAcquisition(void){
  OSC_DSP_StateMachine_Update();
  OSC_DSP_StateMachine.dataAcquisitionState = OSC_DSP_State_Sampling_Single_PreTriggerMemory;
  OSC_DSP_StateMachine.triggerState = OSC_DSP_TriggerState_Disabled;
  OSC_DSP_StateMachine.triggerAnalogWatchdogRange = OSC_Analog_AnalogWatchdog_Range_Invalid;

  OSC_Analog_DMA_ReConfigureBothChannelOnTheFly(
      &OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PreTrigger,
      &OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PreTrigger
  );

  OSC_Analog_Conversion_Start(OSC_Analog_ChannelSelect_ChannelBoth);
  DMA_ITConfig(OSC_ANALOG_CHANNEL_A_DMA_STREAM,OSC_ANALOG_CHANNEL_A_DMA_STATUS_REGISTER_ITFLAG_TC,ENABLE);
}

void OSC_DSP_Calculate(void){
  #ifdef OSC_DSP_CORRECTION
  static uint32_t index = 0;
  uint32_t endIndex;
  #endif

  #ifdef OSC_DSP_CORRECTION
  endIndex = index + OSC_DSP_DIGITAL_DATA_CORRECTION_COUNT_PER_INVOCATION;
  if(endIndex > OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE){
    endIndex = OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE;
  }

  for(;index < endIndex; index++){
    OSC_DSP_Channel_A_DataAcquisitionMemory[index] =
        ((OSC_DSP_Channel_A_DataAcquisitionMemory[index] * OSC_DSP_CORRECTION_SCALE_NUMERATOR) /
          OSC_DSP_CORRECTION_SCALE_DENOMINATOR) + OSC_DSP_CORRECTION_OFFSET;
    OSC_DSP_Channel_B_DataAcquisitionMemory[index] =
        ((OSC_DSP_Channel_B_DataAcquisitionMemory[index] * OSC_DSP_CORRECTION_SCALE_NUMERATOR) /
          OSC_DSP_CORRECTION_SCALE_DENOMINATOR) + OSC_DSP_CORRECTION_OFFSET;
  }

  if(endIndex != OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE) return;
  #endif

  if(OSC_DSP_StateMachine.dataAcquisitionState == OSC_DSP_State_Calculating){
    OSC_DSP_WaveformProperties_Update();
  }

}

/*
 * Horizontal resolution calculations
 *    samplePerPixel = (sampleRate * timePerDivision) / pixelPerDivision
 *    sampleCount    = samplePerPixel * pixelCount
 */

/*
 * Vertical resolution calculations
 *    The sample values should be converted into mV and this should be processed
 *    voltagePerPixel = voltagePerDivision / pixelPerDivision)
 *    valueInPixel    = (sampleValue * voltagePerLSB) / voltagePerPixel =
 *                    = (sampleValue * voltagePerLSB * pixelPerDivision) / (voltagePerDivision)
 */

void OSC_DSP_WaveformProperties_Update(void){
  int32_t   sampleRate;
  int32_t   timePerDivision;
  int32_t   horizontalOffsetInPixel;
  int32_t   voltagePerLSB;
  int32_t   voltagePerDivision;
  int32_t   verticalOffsetInPixel;

  sampleRate       = OSC_Settings_SampleRate.valueSet[OSC_Settings_SampleRate.currentIndex];                      /*sample/ms*/
  timePerDivision  = OSC_Settings_HorizontalResolution.valueSet[OSC_Settings_HorizontalResolution.currentIndex];  /*us/div*/

  OSC_DSP_WaveformProperties.samplePerPixel = (sampleRate * timePerDivision) / (1000 * OSC_DSP_WAVEFORM_PIXEL_PER_HORIZONTAL_DIVISION);
  /*division by 1000 is because of the different units (ms and us)*/

  if(OSC_Settings_DataAcquisitionMode.status == OSC_CFG_DATA_ACQUISITION_MODE_SINGLE){
    horizontalOffsetInPixel = OSC_Settings_HorizontalOffset.value;

    OSC_DSP_WaveformProperties.virtualTriggerPosition =
        OSC_DSP_StateMachine.triggerPosition + (OSC_DSP_WaveformProperties.samplePerPixel * horizontalOffsetInPixel);

    /*If the virtual trigger position runs out of the data acquisition memory then it must be saturated to the extremes*/
    if(OSC_DSP_WaveformProperties.virtualTriggerPosition < 0){
      OSC_DSP_WaveformProperties.virtualTriggerPosition = 0;
    } else if(OSC_DSP_WaveformProperties.virtualTriggerPosition >= OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE){
      OSC_DSP_WaveformProperties.virtualTriggerPosition = OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE - 1;
    }

  } else {    /*OSC_Settings_DataAcquisitionMode.status == OSC_CFG_DATA_ACQUISITION_MODE_REPETITIVE*/
    OSC_DSP_WaveformProperties.virtualTriggerPosition = OSC_DSP_StateMachine.triggerPosition;
  }

  voltagePerLSB      = OSC_Settings_VoltagePerLSB.value;
  voltagePerDivision = OSC_Settings_VerticalResolution.valueSet[OSC_Settings_VerticalResolution.currentIndex];

  verticalOffsetInPixel = OSC_Settings_VerticalOffset.value;

  OSC_DSP_WaveformProperties.verticalScaleFactorNumerator   = voltagePerLSB * OSC_DSP_WAVEFORM_PIXEL_PER_VERTICAL_DIVISION;
  OSC_DSP_WaveformProperties.verticalScaleFactorDenominator = voltagePerDivision;
  OSC_DSP_WaveformProperties.verticalOffset                 = (verticalOffsetInPixel * voltagePerDivision) / OSC_DSP_WAVEFORM_PIXEL_PER_VERTICAL_DIVISION;
}

void OSC_DSP_Waveform_Construct(void){

}

uint8_t OSC_DSP_Waveform_CalculateSampleValue(uint8_t* data,int32_t dataLength,OSC_DSP_DataProcessingMode_Type dataProcMode){
  int32_t   processedDataValue;
  uint32_t  index;

  switch(dataProcMode){
  case OSC_DSP_DataProcessingMode_Normal:
    processedDataValue = data[0];
    break;

  case OSC_DSP_DataProcessingMode_Average:
    processedDataValue = 0;
    for (index = 0; index < dataLength; ++index) {
      processedDataValue += data[index];
    }
    processedDataValue = processedDataValue / dataLength;
    break;

  case OSC_DSP_DataProcessingMode_PeakMax:
    processedDataValue = 0;
    for (index = 0; index < dataLength; ++index) {
      if(data[index] > processedDataValue) processedDataValue = data[index];
    }
    break;

  case OSC_DSP_DataProcessingMode_PeakMin:
    processedDataValue = 0xFFFF;
    for (index = 0; index < dataLength; ++index) {
      if(data[index] < processedDataValue) processedDataValue = data[index];
    }
    break;

  default:
    processedDataValue = 0;
    break;
  }
  return (uint8_t)processedDataValue;
}

void OSC_DSP_StateMachine_Update(void){    /*Updates the DSP state machine configuration dependent attributes*/
  OSC_DSP_StateMachine.dataAcquisitionState           = OSC_DSP_State_Disabled;

  OSC_DSP_StateMachine.preTriggerMemoryLength =
        (OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE * OSC_Settings_TriggerPosition.value) /
         (OSC_Settings_TriggerPosition.upperBound - OSC_Settings_TriggerPosition.lowerBound);

  OSC_DSP_StateMachine.postTriggerMemoryLength = OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE - OSC_DSP_StateMachine.preTriggerMemoryLength;

  OSC_DSP_StateMachine.triggerLevel = (uint8_t)(
          ( ((OSC_Settings_TriggerLevel.value - OSC_CFG_TRIGGER_LEVEL_LOWER_BOUND)) * OSC_DSP_MAX_DATA_VALUE) /
          (OSC_CFG_TRIGGER_LEVEL_UPPER_BOUND - OSC_CFG_TRIGGER_LEVEL_LOWER_BOUND));
                                        /*Shift is needed for precision*/

  OSC_DSP_StateMachine.triggerSlope =   (OSC_Settings_TriggerSlope.status == OSC_CFG_TRIGGER_SLOPE_RISING) ?
                                         OSC_DSP_TriggerSlope_RisingEdge :
                                         OSC_DSP_TriggerSlope_FallingEdge;

  OSC_DSP_StateMachine.triggerSource =  (OSC_Settings_TriggerSource.status == OSC_CFG_CHANNEL_SELECT_CHANNEL_A_SELECTED) ?
                                         OSC_DSP_TriggerSource_Channel_A :
                                         OSC_DSP_TriggerSource_Channel_B;

  OSC_DSP_StateMachine.triggerState  = OSC_DSP_TriggerState_Disabled;

}

/*
 * The concept is that despite the fact there are two DMA transfer only one interrupt handler will be used.
 * This works because the two ADC are triggered same time and the other DMA transfer (with lower prio) will
 * finish after one DMA operation (1-2 clock cycle) later. Because of the one interrupt handler this must check
 * if the other stream has finished or not (this could be predicted based on the stream priorities but to be on
 * the safe side the code will check this).
 */

void OSC_ANALOG_CHANNEL_A_DMA_STREAM_INTERRUPT_HANDLER(void){
  uint8_t actualValue;

  if(DMA_GetFlagStatus(OSC_ANALOG_CHANNEL_A_DMA_STREAM,OSC_ANALOG_CHANNEL_A_DMA_STATUS_REGISTER_FLAG_TC)){

    while((OSC_ANALOG_CHANNEL_B_DMA_STATUS_REGISTER & OSC_ANALOG_CHANNEL_B_DMA_STATUS_REGISTER_FLAG_TC) !=
           OSC_ANALOG_CHANNEL_B_DMA_STATUS_REGISTER_FLAG_TC);

    switch(OSC_DSP_StateMachine.dataAcquisitionState){

      case OSC_DSP_State_Sampling_Single_PreTriggerMemory:
          OSC_Analog_DMA_ReConfigureBothChannelOnTheFly(
              &OSC_Analog_Channel_A_DataAcquisitionConfig_Circular_PreTrigger,
              &OSC_Analog_Channel_B_DataAcquisitionConfig_Circular_PreTrigger
          );
          OSC_DSP_StateMachine.dataAcquisitionState = OSC_DSP_State_Sampling_Circular_PreTriggerMemory;

          actualValue = OSC_DSP_Channel_A_DataAcquisitionMemory[OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE - 1];
          /*the last converted data is in the end of the array*/

          if(actualValue > OSC_DSP_StateMachine.triggerLevel){
            OSC_DSP_StateMachine.triggerAnalogWatchdogRange = OSC_Analog_AnalogWatchdog_Range_Upper;
            if(OSC_DSP_StateMachine.triggerSlope == OSC_DSP_TriggerSlope_RisingEdge){
              OSC_DSP_StateMachine.triggerState = OSC_DSP_TriggerState_Enabled_TwoMoreInterrupt;
            } else {  /*OSC_DSP_StateMachine.triggerSlope == OSC_DSP_TriggerSlope_FallingEdge*/
              OSC_DSP_StateMachine.triggerState = OSC_DSP_TriggerState_Enabled_NextInterrupt;
            }

          } else {    /*actualValue <= OSC_DSP_StateMachine.triggerLevel*/
            OSC_DSP_StateMachine.triggerAnalogWatchdogRange = OSC_Analog_AnalogWatchdog_Range_Lower;
            if(OSC_DSP_StateMachine.triggerSlope == OSC_DSP_TriggerSlope_RisingEdge){
              OSC_DSP_StateMachine.triggerState = OSC_DSP_TriggerState_Enabled_NextInterrupt;
            } else {  /*OSC_DSP_StateMachine.triggerSlope == OSC_DSP_TriggerSlope_FallingEdge*/
              OSC_DSP_StateMachine.triggerState = OSC_DSP_TriggerState_Enabled_TwoMoreInterrupt;
            }
          }

          switch(OSC_DSP_StateMachine.triggerSource){

            case OSC_DSP_TriggerSource_Channel_A:
              OSC_Analog_AnalogWatchdog_Enable(OSC_ANALOG_CHANNEL_A_ADC,OSC_DSP_StateMachine.triggerLevel,OSC_DSP_StateMachine.triggerAnalogWatchdogRange);
              break;

            case OSC_DSP_TriggerSource_Channel_B:
              OSC_Analog_AnalogWatchdog_Enable(OSC_ANALOG_CHANNEL_B_ADC,OSC_DSP_StateMachine.triggerLevel,OSC_DSP_StateMachine.triggerAnalogWatchdogRange);
              break;

            default:
              break;
          }

          DMA_ITConfig(OSC_ANALOG_CHANNEL_A_DMA_STREAM,OSC_ANALOG_CHANNEL_A_DMA_STATUS_REGISTER_ITFLAG_TC,DISABLE);
            /* In OSC_DSP_State_Sampling_Circular_PreTriggerMemory state the transfer complete interrupt is not important because
             * the ADC's analog watchdog interrupt will determine the end of the preTrigger data acquisition ( +DMA in circular mode )*/
        break;

      case OSC_DSP_State_Sampling_Single_PostTriggerMemory_Overflow:
        OSC_DSP_StateMachine.dataAcquisitionState = OSC_DSP_State_Sampling_Single_PostTriggerMemory_NoOverflow;
        OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger.dataAcquisitionMemory   =  &OSC_DSP_Channel_A_DataAcquisitionMemory[0];
        OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PostTrigger.dataAcquisitionMemory   =  &OSC_DSP_Channel_A_DataAcquisitionMemory[0];
        OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger.dataLength              =  OSC_DSP_StateMachine.firstDataPosition;  /*because array index starts with 0*/
        OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PostTrigger.dataLength              =  OSC_DSP_StateMachine.firstDataPosition;  /*because array index starts with 0*/
        OSC_Analog_DMA_ReConfigureBothChannelOnTheFly(
            &OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger,
            &OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PostTrigger
        );
        break;
      case OSC_DSP_State_Sampling_Single_PostTriggerMemory_NoOverflow:
        OSC_Analog_Conversion_Stop(OSC_Analog_ChannelSelect_ChannelBoth);
        OSC_DSP_StateMachine.dataAcquisitionState = OSC_DSP_State_Calculating;
        break;
      default: break;
    } /*END:switch(OSC_DSP_StateMachine.dataAcquisitionState)*/
    DMA_ClearITPendingBit(OSC_ANALOG_CHANNEL_A_DMA_STREAM,OSC_ANALOG_CHANNEL_A_DMA_STATUS_REGISTER_ITFLAG_TC);
  } /*END: Transfer completed flag is set*/
}

void OSC_ANALOG_ADC_INTERRUPT_HANDLER(void){  /*There is one interrupt for all of the ADCs*/
  ADC_TypeDef* triggerSourceADC = OSC_ANALOG_CHANNEL_A_ADC;
  uint32_t initializedDataLength;
  uint32_t dataLengthRegisterDMA;
  uint32_t fistPostTriggerDataPosition;

  switch(OSC_DSP_StateMachine.triggerState){

    case OSC_DSP_TriggerState_Enabled_TwoMoreInterrupt:
      OSC_DSP_StateMachine.triggerState = OSC_DSP_TriggerState_Enabled_NextInterrupt;
      OSC_Analog_AnalogWatchdog_Disable();

      if(OSC_DSP_StateMachine.triggerSource == OSC_DSP_TriggerSource_Channel_A){
        triggerSourceADC = OSC_ANALOG_CHANNEL_A_ADC;
      } else {  /*OSC_DSP_StateMachine.triggerSource == OSC_DSP_TriggerSource_Channel_B*/
        triggerSourceADC = OSC_ANALOG_CHANNEL_B_ADC;
      }

      if(OSC_DSP_StateMachine.triggerAnalogWatchdogRange == OSC_Analog_AnalogWatchdog_Range_Upper){
        OSC_DSP_StateMachine.triggerAnalogWatchdogRange = OSC_Analog_AnalogWatchdog_Range_Lower;
      } else {    /*OSC_DSP_StateMachine.triggerAnalogWatchdogRange == OSC_Analog_AnalogWatchdog_Range_Lower*/
        OSC_DSP_StateMachine.triggerAnalogWatchdogRange = OSC_Analog_AnalogWatchdog_Range_Upper;
      }
      OSC_Analog_AnalogWatchdog_Enable(triggerSourceADC,OSC_DSP_StateMachine.triggerLevel,OSC_DSP_StateMachine.triggerAnalogWatchdogRange);
      break;

    case OSC_DSP_TriggerState_Enabled_NextInterrupt:
      OSC_DSP_StateMachine.triggerState = OSC_DSP_TriggerState_Enabled_Active;
      break;

    default:
      OSC_DSP_StateMachine.triggerState = OSC_DSP_TriggerState_Disabled;
      break;
  }

  if(OSC_DSP_StateMachine.triggerState == OSC_DSP_TriggerState_Enabled_Active){
    if(OSC_DSP_StateMachine.dataAcquisitionState == OSC_DSP_State_Sampling_Circular_PreTriggerMemory){

      /*
       * Fifo flush can be started here -> it will be ready when DMA reconfiguration will be called
       *
       * It is important because of the data consistency -> The read data could be out of date because of hardware events
       *
       * DMA transfers won't be lost because of the disable because if new data is available in the ADC register then
       * DMA request will be active as long as the DMA does not answer with an acknowledge which will be sent after
       * the reactivation of the DMA
       *
       * This interrupt is generated immediately after a conversion (new value -> analog watchdog check -> interrupt)
       *
       * This interrupt should be finished in 2 ADC conversion time because the trigger position value is passed to the
       * DMA before the deactivation so when this interrupt starts the data register is empty. After the next conversion
       * is ready a DMA request is sent and this must be serviced before the next conversion in order to avoid overflow
       * Time constraint: 2 * 168 = 336 clock cycle (168MHz and 1Msample/s)
       */
      OSC_ANALOG_CHANNEL_A_DMA_STREAM_STOP();
      OSC_ANALOG_CHANNEL_B_DMA_STREAM_STOP();

      switch(OSC_DSP_StateMachine.triggerSource){
        case OSC_DSP_TriggerSource_Channel_A:
          dataLengthRegisterDMA   = OSC_ANALOG_CHANNEL_A_DMA_STREAM_DATA_LENGTH_GET();
          initializedDataLength   = OSC_Analog_Channel_A_DataAcquisitionConfig_Circular_PreTrigger.dataLength;
          break;
        case OSC_DSP_TriggerSource_Channel_B:
          dataLengthRegisterDMA   = OSC_ANALOG_CHANNEL_B_DMA_STREAM_DATA_LENGTH_GET();
          initializedDataLength = OSC_Analog_Channel_B_DataAcquisitionConfig_Circular_PreTrigger.dataLength;
          break;
        default:
          break;
      }

      OSC_DSP_StateMachine.triggerPosition   = initializedDataLength - dataLengthRegisterDMA;   /*It can't be negative*/
      OSC_DSP_StateMachine.firstDataPosition = OSC_DSP_StateMachine.triggerPosition + OSC_DSP_StateMachine.postTriggerMemoryLength;
      fistPostTriggerDataPosition = ((OSC_DSP_StateMachine.triggerPosition + 1) > OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE) ?
                                     (OSC_DSP_StateMachine.triggerPosition + 1) : 0;

      if(OSC_DSP_StateMachine.postTriggerMemoryLength > 1){ /*triggerPosition data is the part of postTrigger memory*/
        OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger.dataAcquisitionMemory =
            &OSC_DSP_Channel_A_DataAcquisitionMemory[fistPostTriggerDataPosition];
        OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PostTrigger.dataAcquisitionMemory =
            &OSC_DSP_Channel_B_DataAcquisitionMemory[fistPostTriggerDataPosition];

        if(OSC_DSP_StateMachine.firstDataPosition < OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE){
          OSC_DSP_StateMachine.dataAcquisitionState = OSC_DSP_State_Sampling_Single_PostTriggerMemory_NoOverflow;
          OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger.dataLength =
              OSC_DSP_StateMachine.firstDataPosition - fistPostTriggerDataPosition;   /*triggerPosition data has already been transmitted*/
        } else {  /*OSC_DSP_StateMachine.firstDataPosition >= OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE*/
          OSC_DSP_StateMachine.firstDataPosition   -= OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE;
          OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger.dataLength =
              OSC_DSP_DATA_ACQUISITION_MEMORY_SIZE - fistPostTriggerDataPosition;     /*triggerPosition data has already been transmitted*/
          OSC_DSP_StateMachine.dataAcquisitionState = OSC_DSP_State_Sampling_Single_PostTriggerMemory_Overflow;
        }

        OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PostTrigger.dataLength = OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger.dataLength;
        /*The two channel has to be synchronized so the data length for the DMA must be the same*/

        OSC_Analog_DMA_ReConfigureBothChannelOnTheFly(      /*dmaMode in the structure is set at the beginning of the program and it doesn't change*/
                    &OSC_Analog_Channel_A_DataAcquisitionConfig_Normal_PostTrigger,
                    &OSC_Analog_Channel_B_DataAcquisitionConfig_Normal_PostTrigger
                );
        /*At this point it must be in 336 clock cycle since the start of the interrupt*/
      } else {  /*OSC_DSP_StateMachine.postTriggerMemoryLength <= 1*/
        /*The condition contains 1 instead of 0 because the triggerPosition data is the part of the postTrigger data*/
        OSC_DSP_StateMachine.dataAcquisitionState = OSC_DSP_State_Calculating;
      }   /*END:OSC_DSP_StateMachine.postTriggerMemoryLength <= 1*/
      OSC_DSP_StateMachine.triggerAnalogWatchdogRange = OSC_Analog_AnalogWatchdog_Range_Invalid;
      OSC_DSP_StateMachine.triggerState               = OSC_DSP_TriggerState_Disabled;
      DMA_ITConfig(OSC_ANALOG_CHANNEL_A_DMA_STREAM,OSC_ANALOG_CHANNEL_A_DMA_STATUS_REGISTER_ITFLAG_TC,ENABLE);
      OSC_Analog_AnalogWatchdog_Disable();
    }   /*END:OSC_DSP_StateMachine.dataAcquisitionState == OSC_DSP_State_Sampling_Circular_PreTriggerMemory*/
  }   /*END:OSC_DSP_StateMachine.triggerState == OSC_DSP_TriggerState_Enabled_Active*/

  ADC_ClearITPendingBit(triggerSourceADC, ADC_IT_AWD);
}
