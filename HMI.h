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
    writeString("screen2.name.txt=\"" + PROGRAM_NAME + "\"");

    writeString("");
    writeString("size.txt=\"" + String(LAST_SIZE - 2) + " bytes\"");
    writeString("");
    writeString("screen2.size.txt=\"" + String(LAST_SIZE - 2) + " bytes\"");

    writeString("");
    writeString("type.txt=\"" + String(LAST_TYPE) + ":" + String(LAST_NAME) + "\"");
    writeString("");
    writeString("screen2.type.txt=\"" + String(LAST_TYPE) + ":" + String(LAST_NAME) + "\"");

    writeString("");
    writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));
    writeString("");
    writeString("screen2.totalBlocks.val=" + String(TOTAL_BLOCKS));

    writeString("");
    writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));
    writeString("");
    writeString("screen2.currentBlock.val=" + String(BLOCK_SELECTED + 1));
  }

  writeString("");
  writeString("g0.txt=\"" + LAST_MESSAGE + "\"");
}

void testFile1()
{

  // Open root directory
  if (!dir.open("/")) {
    Serial.println("dir.open failed");
  }
  // Open next file in root.
  // Warning, openNext starts at the current position of dir so a
  // rewind may be necessary in your application.
  while (file.openNext(&dir, O_RDONLY)) {
    file.printFileSize(&Serial);
    Serial.write(' ');
    file.printModifyDateTime(&Serial);
    Serial.write(' ');
    file.printName(&Serial);
    if (file.isDir()) {
      // Indicate a directory.
      Serial.write('/');
    }
    Serial.println();
    file.close();
  }
  if (dir.getError()) {
    Serial.println("openNext failed");
  } else {
    Serial.println("Done!");
    dir.close();
  }

}

void putInHome()
{
    FILE_LAST_DIR="/";
    // Open root directory
    if (!dir.open(FILE_LAST_DIR)) 
    {
      Serial.println("dir.open failed");
    }
    
    dir.rewind();
    file.rewind();

    dir.close();
    file.close();

}

void getFilesFromSD()
{
    char* szName = (char*)calloc(255,sizeof(char));
    char* szDirName = (char*)calloc(255,sizeof(char));

    //Serial.println(FILE_LAST_DIR);
    int j = 0;
    int i = 0;
    
    FILE_TOTAL_FILES = 0;


    if (!dir.open(FILE_LAST_DIR)) 
    {
      Serial.println("");
      Serial.println("dir.open failed");
    }
      
    dir.rewind();
    file.rewind();

    Serial.println();  
    Serial.println("------------------------------------------------------");
    Serial.println(FILE_LAST_DIR);    

    while (file.openNext(&dir, O_RDONLY))
    {
        if (file.isDir())
        {
            file.getName(szName,255);
            //Cambiamos 
            sdf.chdir(szName);
            file.rewind();
            file.close();
            strcpy(szDirName,"[");
            strcat(szDirName,szName);
            strcat(szDirName,"]");
            //Ahora transfiero todo el nombre
            strcpy(szName,szDirName);
            //continue;
        }
        else
        {
            // Es un fichero
            file.getName(szName,255);
            file.close();
        }

        // Guardamos el fichero en el buffer de ficheros mostrados
        FILES_BUFF[j] = FILE_LAST_DIR + String(szName);

        j++;
        FILE_TOTAL_FILES++;

    }

    //writeString("");
    //writeString("com_stop");
    Serial.println("Total files");
    Serial.println(FILE_TOTAL_FILES);   

    dir.close();
    file.close();
}

void putFilesInScreen()
{

  int pos_in_HMI_file = 0;
  String szName = "";

  for (int i=FILE_PTR_POS;i<FILE_PTR_POS+13;i++)
  {
        szName = FILES_BUFF[FILE_PTR_POS + pos_in_HMI_file];
        // Lo trasladamos a la pantalla
        switch(pos_in_HMI_file)
        {
          case 0:
              writeString("");
              writeString("file0.txt=\"" + String(szName) + "\"");
              break;

          case 1:
              writeString("");
              writeString("file1.txt=\"" + String(szName) + "\"");
              break;

          case 2:
              writeString("");
              writeString("file2.txt=\"" + String(szName) + "\"");
              break;

          case 3:
              writeString("");
              writeString("file3.txt=\"" + String(szName) + "\"");
              break;

          case 4:
              writeString("");
              writeString("file4.txt=\"" + String(szName) + "\"");
              break;

          case 5:
              writeString("");
              writeString("file5.txt=\"" + String(szName) + "\"");
              break;

          case 6:
              writeString("");
              writeString("file6.txt=\"" + String(szName) + "\"");
              break;

          case 7:
              writeString("");
              writeString("file7.txt=\"" + String(szName) + "\"");
              break;

          case 8:
              writeString("");
              writeString("file8.txt=\"" + String(szName) + "\"");
              break;

          case 9:
              writeString("");
              writeString("file9.txt=\"" + String(szName) + "\"");
              break;

          case 10:
              writeString("");
              writeString("file10.txt=\"" + String(szName) + "\"");
              break;

          case 11:
              writeString("");
              writeString("file11.txt=\"" + String(szName) + "\"");
              break;

          case 12:
              writeString("");
              writeString("file12.txt=\"" + String(szName) + "\"");
              break;

          case 13:
              writeString("");
              writeString("file13.txt=\"" + String(szName) + "\"");
              break;
        }
        pos_in_HMI_file++;
  }

}

void verifyCommand(String strCmd) 
{
   
  if (strCmd.indexOf("BKX=") != -1) 
  {
      // Con este procedimiento capturamos el bloque seleccionado
      // desde la pantalla.
      byte buff[7];
      strCmd.getBytes(buff, 7);
      String num = String(buff[4]+buff[5]+buff[6]+buff[7]);
      BLOCK_SELECTED = num.toInt();

      updateInformationMainPage();
  }

  if (strCmd.indexOf("FILE=") != -1) 
  {
      // Con este comando nos indica la pantalla que está en modo FILESYSTEM
      // y nos ha devuelto el numero de la fila pulsada
      byte buff[7];
      strCmd.getBytes(buff, 7);
      String num = String(buff[4]+buff[5]);
      FILE_INDEX = num.toInt();

      //updateInformationMainPage();
  }

  if (strCmd.indexOf("GFIL") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      putInHome();
      getFilesFromSD();
      putFilesInScreen();
      //updateInformationMainPage();
      FILE_STATUS = 1;
  }

  if (strCmd.indexOf("FPUP") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      FILE_PTR_POS++;

      // if (FILE_PTR_POS > FILE_TOTAL_FILES-14)
      // {
      //     FILE_PTR_POS = FILE_TOTAL_FILES-14;
      // }

      putFilesInScreen();
      //updateInformationMainPage();


  }

  if (strCmd.indexOf("FPDONW") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      
      //updateInformationMainPage();
  }

  if (strCmd.indexOf("FDIRUP") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      
      //updateInformationMainPage();
  }

  if (strCmd.indexOf("FPHOME") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      putInHome();
      getFilesFromSD();
      //updateInformationMainPage();
  }

  if (strCmd.indexOf("LFI=") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      // Cogemos el valor

      // Cargamos el fichero
      byte buff[7];
      strCmd.getBytes(buff, 7);
      String num = String(buff[4]+buff[5]+buff[6]+buff[7]);

      FILE_IDX_SELECTED = num.toInt();

      //Cogemos el fichero
      FILE_TO_LOAD = FILES_BUFF[FILE_IDX_SELECTED];
      if (FILE_TO_LOAD != "")
      {
          FILE_SELECTED = true;
      }
      else
      {
          FILE_SELECTED = false;
      }

      //updateInformationMainPage();
      FILE_STATUS = 0;
  }

  if (strCmd.indexOf("LCDON") != -1) 
  {
    LCD_ON = true;
  }

  if (strCmd.indexOf("FFWD") != -1) 
  {
    if (LOADING_STATE == 2 || LOADING_STATE == 0) 
    {
      BLOCK_SELECTED++;

      if (BLOCK_SELECTED > TOTAL_BLOCKS - 1) 
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
        BLOCK_SELECTED = TOTAL_BLOCKS - 1;
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

  if (strCmd.indexOf("EJECT") != -1) 
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
    //getFilesFromSD();
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
    char str = (char)buff[n];
    while (str != '@') 
    {
      phrase[n - 12] += (char)buff[n];
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
