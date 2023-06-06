void writeString(String stringData) 
{ // Used to serially push out a String with Serial.write()

  for (int i = 0; i < stringData.length(); i++)
  {
    Serial.write(stringData[i]);   // Push each char 1 by 1 on each loop pass  
  }

  Serial.write(0xff); //We need to write the 3 ending bits to the Nextion as well
  Serial.write(0xff); //it will tell the Nextion that this is the end of what we want to send.
  Serial.write(0xff);

}// end writeString function

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

        writeString("");
        writeString("g0.txt=\"" + LAST_MESSAGE + "\"");

    }
}

void verifyCommand(String strCmd)
{
    
    if (strCmd.indexOf("RWD") != -1)
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

    if (strCmd.indexOf("FFWD") != -1)
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

        Serial.println("");
        Serial.print("STOP pressed");    

        LAST_MESSAGE = "Tape stop. Press play to start again.";
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
}

void readUART()
{
  if (Serial.available()) 
  {
    // get the new byte:
    String strCmd = Serial.readString();
    verifyCommand(strCmd);
    //Serial.flush();
  }  
}

