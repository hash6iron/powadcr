void writeString(String stringData) 
{ 

    // Controlamos el buffer overflow.
    if ((stringData.length() + Serial.available()) < 64)
    {
        for (int i = 0; i < stringData.length(); i++)
        {
          // Enviamos los datos
          Serial.write(stringData[i]);   
        }

        // Indicamos a la pantalla que ya hemos enviado todos 
        // los datos, con este triple byte 0xFF
        Serial.write(0xff); 
        Serial.write(0xff); 
        Serial.write(0xff);
    }
    else
    {
        Serial.flush();
        Serial.println("");
        Serial.println("OVERFLOW FLUSHED!!!");
        Serial.println("");
    }
}

void updateInformationMainPage()
{

    if (TOTAL_BLOCKS != 0)
    {

        // Cargamos la información
        LAST_NAME = globalTAP.descriptor[BLOCK_SELECTED].name;
        LAST_SIZE = globalTAP.descriptor[BLOCK_SELECTED].size;
        LAST_TYPE = globalTAP.descriptor[BLOCK_SELECTED].typeName;

        // Enviamos información al HMI
        writeString("");
        writeString("name.txt=\"" + PROGRAM_NAME + "\"");

        writeString("");
        writeString("size.txt=\"" + String(LAST_SIZE-2) + " bytes\"");

        writeString("");
        writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));

        writeString("");
        writeString("currentBlock.val=" + String(BLOCK_SELECTED+1));

        writeString("");
        writeString("type.txt=\""+ String(LAST_TYPE) + ":" + String(LAST_NAME) + "\"");
    }

    writeString("");
    writeString("g0.txt=\"" + LAST_MESSAGE + "\"");
}

void verifyCommand(String strCmd)
{
    
    if(strCmd.indexOf("LCDON") != -1)
    {
        LCD_ON = true;
    }
    
    if (strCmd.indexOf("FFWD") != -1)
    {
        if (LOADING_STATE == 2 || LOADING_STATE == 0)
        {
            BLOCK_SELECTED++;

            if (BLOCK_SELECTED > TOTAL_BLOCKS-1)
            {
                BLOCK_SELECTED = 0;
            }

            updateInformationMainPage();      
        }

    }

    if (strCmd.indexOf("RWD") != -1)
    {
        if (LOADING_STATE == 2 || LOADING_STATE == 0)
        {

            BLOCK_SELECTED--;

            if (BLOCK_SELECTED < 0)
            {
                BLOCK_SELECTED = TOTAL_BLOCKS-1;
            }

            updateInformationMainPage();           
        }
    }

    if (strCmd.indexOf("PLAY") != -1)
    {
        PLAY = true;
        PAUSE = false;
        STOP = false;

        Serial.println("");
        Serial.print("PLAY pressed");

        LAST_MESSAGE = "Loading in progress. Please wait.";
        updateInformationMainPage();           
    }

    if (strCmd.indexOf("PAUSE") != -1)
    {
        PLAY = false;
        PAUSE = true;
        STOP = false;

        Serial.println("");
        Serial.print("PAUSE pressed");

        LAST_MESSAGE = "Tape paused. Press play to continue load.";
        updateInformationMainPage();           
    }

    if (strCmd.indexOf("STOP") != -1)
    {
        PLAY = false;
        PAUSE = false;
        STOP = true;
        BLOCK_SELECTED = 0;
        BYTES_LOADED = 0;

        Serial.println("");
        Serial.print("STOP pressed");    

        LAST_MESSAGE = "Tape stop. Press play to start again.";
        writeString("");
        writeString("currentBlock.val=1");   
        writeString("");
        writeString("progressTotal.val=0");      
        writeString("");
        writeString("progression.val=0");                   
        updateInformationMainPage();           
    }

    if (strCmd.indexOf("VOL") != -1)
    {
        //Cogemos el valor
        byte buff[7];
        strCmd.getBytes(buff, 7);
        int valVol = (int)buff[4];
        MAIN_VOL = valVol;
        ESP32kit.setVolume(MAIN_VOL);
    }

    if (strCmd.indexOf("TXT_SEARCH") != -1)
    {
        //Cogemos el valor
        byte buff[50];
        strCmd.getBytes(buff, 50);
        int n = 12;
        char phrase[50];
        char str=(char)buff[n];
        while (str !='@')
        {
            phrase[n-12] += (char)buff[n];
            n++;
            str = (char)buff[n];
        }

        Serial.println("TXT TO SEARCH: " + String(phrase));
    }

}

void readUART()
{


  if (Serial.available() > 0) 
  {
    // get the new byte:
    String strCmd = Serial.readString();
    verifyCommand(strCmd);
    //Serial.flush();
  }  
}

void serialSendData(String data)
{

    if (Serial.available() + data.length() < 64)
    { 
      // Lo envio
      Serial.println(data);
    }
    else
    {
      // Lo descarto
    }
}

