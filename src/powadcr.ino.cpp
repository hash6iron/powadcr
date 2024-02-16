# 1 "C:\\Users\\atama\\AppData\\Local\\Temp\\tmplu802ybc"
#include <Arduino.h>
# 1 "C:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr/src/powadcr.ino"
# 75 "C:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr/src/powadcr.ino"
#define AUDIOKIT_BOARD 5





#define WORKAROUND_ES8388_LINE1_GAIN MIC_GAIN_MAX
#define WORKAROUND_ES8388_LINE2_GAIN MIC_GAIN_MAX


#define WORKAROUND_MIC_LINEIN_MIXED false


#define AI_THINKER_ES8388_VOLUME_HACK 1


#include <Arduino.h>
#include "esp32-hal-psram.h"


TaskHandle_t Task0;
TaskHandle_t Task1;

#define SerialHWDataBits 921600

#include <esp_task_wdt.h>

#include <HardwareSerial.h>
HardwareSerial SerialHW(0);

#include "config.h"

#include "SdFat.h"
#include "globales.h"


#include "AudioKitHAL.h"
AudioKit ESP32kit;


#include "SDmanager.h"


SDmanager sdm;

#include "HMI.h"
HMI hmi;

#include "interface.h"
#include "ZXProcessor.h"


ZXProcessor zxp;


#include "TZXprocessor.h"
#include "TAPprocessor.h"




SdFat sd;
SdFat32 sdf;
SdFile sdFile;
File32 sdFile32;


TZXprocessor pTZX(ESP32kit);
TAPprocessor pTAP(ESP32kit);


#include "TAPrecorder.h"
TAPrecorder taprec;


TAPprocessor::tTAP myTAP;

TZXprocessor::tTZX myTZX;

bool last_headPhoneDetection = false;
uint8_t tapeState = 0;
void proccesingTAP(char* file_ch);
void proccesingTZX(char* file_ch);
void sendStatus(int action, int value);
void setSDFrequency(int SD_Speed);
void test();
void waitForHMI(bool waitAndNotForze);
void setAudioOutput();
void setAudioInput();
void setAudioInOut();
void pauseRecording();
void stopRecording();
void setup();
void setSTOP();
void playingFile();
void loadingFile();
void stopFile();
void pauseFile();
void ejectingFile();
void prepareRecording();
void recordingFile();
void tapeControl();
bool headPhoneDetection();
void Task0code( void * pvParameters );
void loop();
#line 157 "C:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr/src/powadcr.ino"
void proccesingTAP(char* file_ch)
{

    pTAP.initialize();

    if (!PLAY)
    {

        pTAP.getInfoFileTAP(file_ch);

        if (!FILE_CORRUPTED)
        {
          LAST_MESSAGE = "Press PLAY to enjoy!";

          FILE_PREPARED = true;
        }
        else
        {
          LAST_MESSAGE = "ERROR! Selected file is CORRUPTED.";

          FILE_PREPARED = false;
        }

        FILE_NOTIFIED = true;



    }
    else
    {
          LAST_MESSAGE = "Error. PLAY in progress. Try select file again.";

          FILE_PREPARED = false;
          FILE_NOTIFIED = false;
    }
}

void proccesingTZX(char* file_ch)
{
    pTZX.initialize();

    pTZX.getInfoFileTZX(file_ch);

    if (ABORT)
    {
      LAST_MESSAGE = "Aborting proccess.";

      FILE_PREPARED = false;
      ABORT=false;
    }
    else
    {
      LAST_MESSAGE = "Press PLAY to enjoy!";

      FILE_PREPARED = true;
    }

    FILE_NOTIFIED = true;

}

void sendStatus(int action, int value) {

  switch (action) {
    case PLAY_ST:

      hmi.writeString("PLAYst.val=" + String(value));
      break;

    case STOP_ST:

      hmi.writeString("STOPst.val=" + String(value));
      break;

    case PAUSE_ST:

      hmi.writeString("PAUSEst.val=" + String(value));
      break;

    case END_ST:

      hmi.writeString("ENDst.val=" + String(value));
      break;

    case REC_ST:
      hmi.writeString("RECst.val=" + String(value));
      break;

    case READY_ST:

      hmi.writeString("READYst.val=" + String(value));
      break;

    case ACK_LCD:

      hmi.writeString("tape.LCDACK.val=" + String(value));

      hmi.writeString("statusLCD.txt=\"READY. PRESS SCREEN\"");

      hmi.writeString("menu.verFirmware.txt=\" PowaDCR " + String(VERSION) + "\"");

      break;

    case RESET:

      hmi.writeString("statusLCD.txt=\"SYSTEM REBOOT\"");
  }
}
void setSDFrequency(int SD_Speed)
{
  bool SD_ok = false;
  bool lastStatus = false;
  while (!SD_ok)
  {
    if (!sdf.begin(ESP32kit.pinSpiCs(), SD_SCK_MHZ(SD_Speed)) || lastStatus)
    {

      sdFile32.close();
# 284 "C:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr/src/powadcr.ino"
      hmi.writeString("statusLCD.txt=\"SD FREQ. AT " + String(SD_Speed) + " MHz\"" );



      SD_Speed = SD_Speed - 1;

      if (SD_Speed < 4)
      {
          if (SD_Speed < 2)
          {

            SD_Speed = 1;
            lastStatus = true;
          }
          else
          {







              while(true)
              {
                hmi.writeString("statusLCD.txt=\"SD NOT COMPATIBLE\"" );
                delay(1500);
                hmi.writeString("statusLCD.txt=\"CHANGE SD AND RESET\"" );
                delay(1500);
              }
          }
      }
    }
    else
    {





        hmi.writeString("statusLCD.txt=\"SD DETECTED AT " + String(SD_Speed) + " MHz\"" );


        if (!sdm.dir.open("/"))
        {
            sdm.dir.close();
            SD_ok = false;
            lastStatus = false;






            while(true)
            {
              hmi.writeString("statusLCD.txt=\"SD NOT COMPATIBLE\"" );
              delay(1500);
              hmi.writeString("statusLCD.txt=\"CHANGE SD AND RESET\"" );
              delay(1500);
            }
        }
        else
        {
            sdm.dir.close();


            if (!sdFile32.open("_test", O_WRITE | O_CREAT | O_TRUNC))
            {
                SD_ok = false;
                lastStatus = false;






                while(true)
                {
                  hmi.writeString("statusLCD.txt=\"SD CORRUPTED\"" );
                  delay(1500);
                  hmi.writeString("statusLCD.txt=\"FORMAT OR CHANGE SD\"" );
                  delay(1500);
                }
            }
            else
            {
              sdFile32.close();
              SD_ok = true;
              lastStatus = true;
            }
        }
    }
  }

  delay(1250);
}

void test()
{

}

void waitForHMI(bool waitAndNotForze)
{


    if (!waitAndNotForze)
    {
        LCD_ON = true;
        sendStatus(ACK_LCD, 1);
    }
    else
    {

      while (!LCD_ON)
      {
          hmi.readUART();
      }

      LCD_ON = true;





      sendStatus(ACK_LCD, 1);
    }
}


void setAudioOutput()
{
  auto cfg = ESP32kit.defaultConfig(KitOutput);

  if(!ESP32kit.begin(cfg))
  {
    log("Error in Audiokit output setting");
  }

  if(!ESP32kit.setSampleRate(AUDIO_HAL_44K_SAMPLES))
  {
    log("Error in Audiokit sampling rate setting");
  }

  if (!ESP32kit.setVolume(100))
  {
    log("Error in volumen setting");
  }
}

void setAudioInput()
{
  auto cfg = ESP32kit.defaultConfig(KitInput);

  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2;
  cfg.sample_rate = AUDIO_HAL_48K_SAMPLES;

  if(!ESP32kit.begin(cfg))
  {
    log("Error in Audiokit input setting");
  }

  if(!ESP32kit.setSampleRate(AUDIO_HAL_48K_SAMPLES))
  {
    log("Error in Audiokit sampling rate setting");
  }
}

void setAudioInOut()
{
  auto cfg = ESP32kit.defaultConfig(KitInputOutput);
  cfg.adc_input = AUDIO_HAL_ADC_INPUT_LINE2;

  if(!ESP32kit.begin(cfg))
  {
    log("Error in Audiokit output setting");
  }

  if(!ESP32kit.setSampleRate(AUDIO_HAL_48K_SAMPLES))
  {
    log("Error in Audiokit sampling rate setting");
  }

  if (!ESP32kit.setVolume(100))
  {
    log("Error in volumen setting");
  }
}

void pauseRecording()
{

    setAudioOutput();
    waitingRecMessageShown = false;
    REC = false;

    LAST_MESSAGE = "Recording paused.";





}

void stopRecording()
{

    hmi.writeString("tape.tmAnimation.en=0");


    setAudioOutput();


    if (!taprec.errorInDataRecording && !taprec.wasFileNotCreated)
    {




      int tbt = taprec.totalBlockTransfered;

      if (tbt > 1)
      {
        taprec.terminate(false);
        LAST_MESSAGE = "Recording STOP. File succesfully saved.";

        delay(1000);
      }
      else
      {
        taprec.terminate(true);
        LAST_MESSAGE = "Recording STOP. No file saved";

        delay(1000);
      }
    }
    else if (taprec.errorInDataRecording)
    {




      taprec.terminate(true);
      LAST_MESSAGE = "Recording STOP. No file saved.";


      delay(1000);
    }
    else if (taprec.wasFileNotCreated)
    {

        taprec.terminate(false);
        LAST_MESSAGE = "Error in filesystem or SD.";


        delay(1000);
    }


    waitingRecMessageShown = false;
    REC = false;
# 554 "C:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr/src/powadcr.ino"
}

void setup()
{





  SerialHW.setRxBufferSize(2048);
  SerialHW.begin(921600);
  delay(250);




  hmi.writeString("rest");
  delay(250);


  sendStatus(RESET, 1);
  delay(750);

  hmi.writeString("statusLCD.txt=\"POWADCR " + String(VERSION) + "\"" );

  delay(1250);




  configureButtons();


  LOGLEVEL_AUDIOKIT = AudioKitError;


  setAudioOutput();






  hmi.writeString("statusLCD.txt=\"WAITING FOR SD CARD\"" );
  delay(750);

  int SD_Speed = SD_FRQ_MHZ_INITIAL;
  setSDFrequency(SD_Speed);





  hmi.set_sdf(sdf);
# 616 "C:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr/src/powadcr.ino"
  if(psramInit()){

    hmi.writeString("statusLCD.txt=\"PSRAM OK\"" );

  }else{

    hmi.writeString("statusLCD.txt=\"PSRAM FAILED!\"" );
  }
  delay(750);

  hmi.writeString("statusLCD.txt=\"WAITING FOR HMI\"" );



  waitForHMI(CFG_FORZE_SINC_HMI);


  hmi.writeString("menuAudio.volL.val=" + String(MAIN_VOL_L));
  hmi.writeString("menuAudio.volR.val=" + String(MAIN_VOL_R));
  hmi.writeString("menuAudio.volLevelL.val=" + String(MAIN_VOL_L));
  hmi.writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_R));


  pTAP.set_HMI(hmi);
  pTZX.set_HMI(hmi);

  pTAP.set_SDM(sdm);
  pTZX.set_SDM(sdm);

  zxp.set_ESP32kit(ESP32kit);



#ifdef TEST
  TEST_RUNNING = true;
  hmi.writeString("statusLCD.txt=\"TEST RUNNING\"" );
  test();
  hmi.writeString("statusLCD.txt=\"PRESS SCREEN\"" );
  TEST_RUNNING = false;
#endif






  LOADING_STATE = 0;
  BLOCK_SELECTED = 0;
  FILE_SELECTED = false;


  STOP = true;
  PLAY = false;
  PAUSE = false;
  REC = false;






  sendStatus(REC_ST, 0);


  LAST_MESSAGE = "Press EJECT to select a file.";
# 701 "C:/Users/atama/Documents/200.SPECTRUM/500. Proyectos/PowaDCR - General/powadcr/src/powadcr.ino"
  void Task0code( void * pvParameters );


  xTaskCreatePinnedToCore(Task0code, "TaskCORE0", 4096, NULL, 5, &Task0, 0);
  delay(500);


  disableCore0WDT();
  disableCore1WDT();


  taprec.set_HMI(hmi);
  taprec.set_SdFat32(sdf);


}

void setSTOP()
{
  STOP = true;
  PLAY = false;
  PAUSE = false;
  REC = false;

  BLOCK_SELECTED = 0;

  hmi.writeString("tape.tmAnimation.en=0");
  LAST_MESSAGE = "Tape stop.";


  hmi.writeString("currentBlock.val=1");
  hmi.writeString("progressTotal.val=0");
  hmi.writeString("progression.val=0");




  sendStatus(REC_ST, 0);



}

void playingFile()
{
  setAudioOutput();
  ESP32kit.setVolume(MAX_MAIN_VOL);

  sendStatus(REC_ST, 0);


  hmi.writeString("tape.tmAnimation.en=1");


  LAST_MESSAGE = "Loading in progress. Please wait.";


  if (TYPE_FILE_LOAD == "TAP")
  {

      pTAP.play();

      hmi.writeString("tape.tmAnimation.en=0");

  }
  else if (TYPE_FILE_LOAD = "TZX")
  {

      pTZX.play();

      hmi.writeString("tape.tmAnimation.en=0");
  }
}

void loadingFile()
{

  char* file_ch = (char*)malloc(256 * sizeof(char));
  FILE_TO_LOAD.toCharArray(file_ch, 256);


  if (FILE_TO_LOAD != "") {


    hmi.clearInformationFile();


    FILE_TO_LOAD.toUpperCase();

    if (FILE_TO_LOAD.indexOf(".TAP") != -1)
    {

        myTAP.descriptor = (TAPprocessor::tBlockDescriptor*)ps_malloc(MAX_BLOCKS_IN_TAP * sizeof(struct TAPprocessor::tBlockDescriptor));

        pTAP.setTAP(myTAP);


        proccesingTAP(file_ch);
        TYPE_FILE_LOAD = "TAP";
    }
    else if (FILE_TO_LOAD.indexOf(".TZX") != -1)
    {

        myTZX.descriptor = (TZXprocessor::tTZXBlockDescriptor*)ps_malloc(MAX_BLOCKS_IN_TZX * sizeof(struct TZXprocessor::tTZXBlockDescriptor));

        pTZX.setTZX(myTZX);


        proccesingTZX(file_ch);
        TYPE_FILE_LOAD = "TZX";
    }
    else if (FILE_TO_LOAD.indexOf(".TSX") != -1)
    {




        proccesingTZX(file_ch);
        TYPE_FILE_LOAD = "TSX";
    }
  }


  free(file_ch);
}

void stopFile()
{

  setSTOP();
  hmi.writeString("tape.tmAnimation.en=0");
}

void pauseFile()
{
  STOP = false;
  PLAY = false;
  PAUSE = true;
  REC = false;
}

void ejectingFile()
{
  log("Eject executing");


  if (TYPE_FILE_LOAD == "TAP")
  {


      free(pTAP.getDescriptor());

      pTAP.terminate();
  }
  else if (TYPE_FILE_LOAD = "TZX")
  {


      free(pTZX.getDescriptor());

      pTZX.terminate();
  }
}

void prepareRecording()
{

    if (myTAP.descriptor!= nullptr)
    {
      free(myTAP.descriptor);
    }




    hmi.writeString("tape.tmAnimation.en=1");


    setAudioInput();
    taprec.set_kit(ESP32kit);
    taprec.initialize();

    if (!taprec.createTempTAPfile())
    {
      LAST_MESSAGE = "Recorder not prepared.";

    }
    else
    {
      LAST_MESSAGE = "Recorder listening.";


      hmi.writeString("currentBlock.val=1");

      hmi.writeString("progressTotal.val=0");

      hmi.writeString("progression.val=0");
    }


}


void recordingFile()
{

    if (taprec.recording())
    {


        if (taprec.stopRecordingProccess)
        {







            LOADING_STATE = 0;

            stopRecording();
            setSTOP();
        }
    }
}

void tapeControl()
{
  switch (tapeState)
  {
    case 0:

      if (FILE_BROWSER_OPEN)
      {
        tapeState = 99;
        LOADING_STATE = 0;
      }
      else if(FILE_SELECTED)
      {

        loadingFile();
        LAST_MESSAGE = "File inside the TAPE.";


        tapeState = 1;
        LOADING_STATE = 0;
        FILE_SELECTED = false;
        FFWIND = false;
        RWIND = false;
      }
      else if(PLAY)
      {
        LAST_MESSAGE = "No file was selected.";

        LOADING_STATE = 0;
        tapeState = 0;
        FFWIND = false;
        RWIND = false;
      }
      else if(REC)
      {
        FFWIND = false;
        RWIND = false;

        prepareRecording();
        log("REC. Waiting for guide tone");
        tapeState = 200;
      }
      else
      {
        tapeState = 0;
      }
      break;

    case 1:

      if (PLAY)
      {

        FFWIND = false;
        RWIND = false;
        LOADING_STATE = 1;
        playingFile();
        tapeState = 2;

      }
      else if(EJECT)
      {
        FFWIND = false;
        RWIND = false;
        tapeState = 5;
      }
      else if (FFWIND || RWIND)
      {

        if (TYPE_FILE_LOAD=="TAP")
        {
          hmi.setBasicFileInformation(myTAP.descriptor[BLOCK_SELECTED].name,myTAP.descriptor[BLOCK_SELECTED].typeName,myTAP.descriptor[BLOCK_SELECTED].size);
        }
        else if(TYPE_FILE_LOAD=="TZX")
        {
          hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].name,myTZX.descriptor[BLOCK_SELECTED].typeName,myTZX.descriptor[BLOCK_SELECTED].size);
        }

        tapeState = 1;
        FFWIND = false;
        RWIND = false;
      }
      else
      {
        tapeState = 1;
      }
      break;

    case 2:

      if (PAUSE)
      {

        LOADING_STATE = 3;
        tapeState = 3;
        log("Estoy en PAUSA.");
      }
      else if(STOP)
      {
        stopFile();
        tapeState = 4;
        LOADING_STATE = 0;

      }
      else if(EJECT)
      {

        tapeState = 5;
      }
      else
      {
        tapeState = 2;
      }
      break;

    case 3:

      if (PLAY)
      {

        LOADING_STATE = 1;
        playingFile();
        tapeState = 2;

        FFWIND = false;
        RWIND = false;
      }
      else if(STOP)
      {
        stopFile();
        LOADING_STATE = 0;
        tapeState = 4;

        FFWIND = false;
        RWIND = false;
      }
      else if(EJECT)
      {
        tapeState = 5;

        FFWIND = false;
        RWIND = false;
      }
      else if (FFWIND || RWIND)
      {

        if (TYPE_FILE_LOAD=="TAP")
        {
          hmi.setBasicFileInformation(myTAP.descriptor[BLOCK_SELECTED].name,myTAP.descriptor[BLOCK_SELECTED].typeName,myTAP.descriptor[BLOCK_SELECTED].size);
        }
        else if(TYPE_FILE_LOAD=="TZX")
        {
          hmi.setBasicFileInformation(myTZX.descriptor[BLOCK_SELECTED].name,myTZX.descriptor[BLOCK_SELECTED].typeName,myTZX.descriptor[BLOCK_SELECTED].size);
        }

        tapeState = 3;
        FFWIND = false;
        RWIND = false;
      }
      else
      {
        tapeState = 3;
      }
      break;

    case 4:

      if (PLAY)
      {

        LOADING_STATE = 1;
        playingFile();
        tapeState = 2;
      }
      else if(EJECT)
      {
        tapeState = 5;
      }
      else if(REC)
      {
        FFWIND = false;
        RWIND = false;

        prepareRecording();
        log("REC. Waiting for guide tone");
        tapeState = 200;
      }
      else
      {
        tapeState = 4;
      }
      break;
    case 5:
        stopFile();
        ejectingFile();
        LOADING_STATE = 0;
        tapeState = 0;
      break;
    case 99:
      if (!FILE_BROWSER_OPEN)
      {
        tapeState = 0;
        LOADING_STATE = 0;
      }
      else
      {
        tapeState = 99;
      }
      break;

    case 200:

      if(STOP)
      {
        stopRecording();
        setSTOP();
        tapeState = 0;
      }
      else
      {
        recordingFile();
        tapeState = 200;
      }
      break;

    default:
      break;
  }
}

bool headPhoneDetection()
{
  return !gpio_get_level((gpio_num_t)HEADPHONE_DETECT);
}







void Task0code( void * pvParameters )
{

  int startTime = millis();
  while(true)
  {
      hmi.readUART();



      #ifndef DEBUGMODE
        if ((millis() - startTime) > 250)
        {
          startTime = millis();
          hmi.updateInformationMainPage();
        }
      #endif
  }
}

void loop()
{

    tapeControl();


}