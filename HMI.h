void writeString(String stringData) 
{

    //Serial.flush();

    Serial.write(0xff);
    Serial.write(0xff);
    Serial.write(0xff);

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

    if(FILE_BROWSER_OPEN)
    {
        // Metemos un delay de 10ms
        // menos da problemas.
        delay(10);       
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
    //writeString("");
    writeString("name.txt=\"" + PROGRAM_NAME + "\"");
    //writeString("");
    writeString("screen2.name.txt=\"" + PROGRAM_NAME + "\"");

    //writeString("");
    writeString("size.txt=\"" + String(LAST_SIZE - 2) + " bytes\"");
    //writeString("");
    writeString("screen2.size.txt=\"" + String(LAST_SIZE - 2) + " bytes\"");

    //writeString("");
    writeString("type.txt=\"" + String(LAST_TYPE) + ":" + String(LAST_NAME) + "\"");
    //writeString("");
    writeString("screen2.type.txt=\"" + String(LAST_TYPE) + ":" + String(LAST_NAME) + "\"");

    //writeString("");
    writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));
    //writeString("");
    writeString("screen2.totalBlocks.val=" + String(TOTAL_BLOCKS));

    //writeString("");
    writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));
    //writeString("");
    writeString("screen2.currentBlock.val=" + String(BLOCK_SELECTED + 1));
  }

  //writeString("");
  writeString("g0.txt=\"" + LAST_MESSAGE + "\"");
}

int countFiles(char* path)
{
    int fileC = -1;

    dir.rewindDirectory();

    if (dir.open(path)) 
    {
        // Si se ha abierto. Partimos de que el directorio
        // esta vacio.
        fileC = 0;

        //file.rewind();
        char* szName = (char*)calloc(255,sizeof(char));
        szName = "\0";

        while (file.openNext(&dir, O_RDONLY))
        {
            if (file.isDir())
            {
                file.getName(szName,255);
                sdf.chdir(szName);
                file.close();
            }
            else
            {
                // Es un fichero
                file.getName(szName,255);
                file.close();
            }
            fileC++;
        }
        //
        dir.close();
    }

    return fileC;
}

void getFilesFromSD()
{
    char* szName = (char*)calloc(255+1,sizeof(char));
    //szName = "\0";

    int j = 0;

    // Dimensionamos el array con calloc
    if (FILES_BUFF != NULL)
    {
        // Liberamos primero el espacio anterior
        free(FILES_BUFF);
        FILES_BUFF = NULL;
    }

    //FILE_TOTAL_FILES = countFiles(FILE_LAST_DIR);
    FILE_TOTAL_FILES = 2000;
    FILES_BUFF = (tFileBuffer*)calloc(FILE_TOTAL_FILES+1,sizeof(tFileBuffer));

    Serial.println("");
    Serial.println("");
    Serial.println("TEST 1");

    FILE_DIR_OPEN_FAILED = false;
    
    if (!dir.open(FILE_LAST_DIR)) 
    {
        Serial.println("");
        Serial.println("dir.open failed");
        FILE_DIR_OPEN_FAILED = true;
    }
    else
    {
        dir.rewindDirectory();
        file.rewind();

        Serial.println("");
        Serial.println("dir.open OK");

        FILE_DIR_OPEN_FAILED = false;

        //  AÑADIMOS AL INICIO EL PARENT DIR
        FILES_BUFF[0].isDir = true;
        FILES_BUFF[0].type = "PAR";
        FILES_BUFF[0].path = String(FILE_PREVIOUS_DIR);

        Serial.println("");
        Serial.println("");
        Serial.println("TEST 2");

        j++;
        
        while (file.openNext(&dir, O_RDONLY) && j < FILE_TOTAL_FILES)
        {
            if (file.isDir())
            {
                if (!file.isHidden())
                {

    Serial.println("");
    Serial.println("");
    Serial.println("TEST 2.2");

                    file.getName(szName,255);

    Serial.println("");
    Serial.println("");
    Serial.println("TEST 2.3");

                    //Cambiamos 
                    sdf.chdir(szName);

    Serial.println("");
    Serial.println("");
    Serial.println("TEST 3");

                    FILES_BUFF[j].isDir = true;
                    FILES_BUFF[j].type = "DIR";
                    FILES_BUFF[j].path = String(szName);

    Serial.println("");
    Serial.println("");
    Serial.println("TEST 4");

                    j++;
                }
                else
                {
                  // No se muestra el dir porque está oculto
                }

            }
            else
            {
                if (!file.isHidden())
                {
                    // Es un fichero
                    file.getName(szName,255);
                    int8_t len = strlen(szName);

    Serial.println("");
    Serial.println("");
    Serial.println("TEST 5");


                    if (strstr(strlwr(szName + (len - 4)), ".tap")) 
                    {
                        FILES_BUFF[j].type = "TAP";
                        FILES_BUFF[j].isDir = false;
                        // Guardamos el fichero en el buffer
                        FILES_BUFF[j].path = String(szName);
                        j++;
                    }
                    else if (strstr(strlwr(szName + (len - 4)), ".tzx")) 
                    {
                        FILES_BUFF[j].type = "TZX";
                        FILES_BUFF[j].isDir = false;
                        // Guardamos el fichero en el buffer
                        FILES_BUFF[j].path = String(szName);
                        j++;
                    }
                    else if (strstr(strlwr(szName + (len - 4)), ".sna")) 
                    {
                        FILES_BUFF[j].type = "SNA";
                        FILES_BUFF[j].isDir = false;
                        // Guardamos el fichero en el buffer
                        FILES_BUFF[j].path = String(szName);
                        j++;
                    }                    
                    else if (strstr(strlwr(szName + (len - 4)), ".z80")) 
                    {
                        FILES_BUFF[j].type = "Z80";
                        FILES_BUFF[j].isDir = false;
                        // Guardamos el fichero en el buffer
                        FILES_BUFF[j].path = String(szName);
                        j++;
                    } 
                    else
                    {
                        // No se guarda
                        //FILES_BUFF[j].type = "";
                        //FILES_BUFF[j].isDir = false;
                        //FILES_BUFF[j].path = "";
                    }
                }
                else
                {
                  // No se muestra el fichero porque está oculto
                }
            }

            file.close();

        }

        FILE_TOTAL_FILES = j;
        dir.close();
    }

    Serial.println("");
    Serial.println("");
    Serial.println("TOTAL FILES READ: " + String(FILE_TOTAL_FILES));
}

void printFileRows(int row, int color, String szName)
{
      switch(row)
      {
        case 0:
            //writeString("");
            writeString("file0.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file0.pco=" + String(color));
            break;

        case 1:
            //writeString("");
            writeString("file1.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file1.pco=" + String(color));
            break;

        case 2:
            //writeString("");
            writeString("file2.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file2.pco=" + String(color));
            break;

        case 3:
            //writeString("");
            writeString("file3.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file3.pco=" + String(color));
            break;

        case 4:
            //writeString("");
            writeString("file4.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file4.pco=" + String(color));
            break;

        case 5:
            //writeString("");
            writeString("file5.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file5.pco=" + String(color));
            break;

        case 6:
            //writeString("");
            writeString("file6.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file6.pco=" + String(color));
            break;

        case 7:
            //writeString("");
            writeString("file7.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file7.pco=" + String(color));
            break;

        case 8:
            //writeString("");
            writeString("file8.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file8.pco=" + String(color));
            break;

        case 9:
            //writeString("");
            writeString("file9.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file9.pco=" + String(color));
            break;

        case 10:
            //writeString("");
            writeString("file10.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file10.pco=" + String(color));
            break;

        case 11:
            //writeString("");
            writeString("file11.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file11.pco=" + String(color));
            break;

        case 12:
            //writeString("");
            writeString("file12.txt=\"" + String(szName) + "\"");
            //writeString("");
            writeString("file12.pco=" + String(color));
            break;

        }  
}

char* getPreviousDirFromPath(char* path)
{
    char* strTmp = (char*)calloc(2,sizeof(char));
    strTmp = "/";
    int lpath = strlen(path);
    
    for (int n=lpath-2;n>1;n--)
    {
        char chrRead = path[n];
        
        if (chrRead == '/')
        {
            //Copiamos ahora el trozo de cadena restante
            //ponemos n-1 para que copie también la "/"
            strTmp = (char*)calloc(n+2,sizeof(char));
            strlcpy(strTmp,path,n+2);
            break;
        }
    }

    return strTmp;
}

void clearFilesInScreen()
{
  String szName = "";
  int color = 65535;
  int pos_in_HMI_file = 0;

  for (int i=0;i<=13;i++)
  {
      printFileRows(pos_in_HMI_file, color, szName);
      pos_in_HMI_file++;
  }

}

void putFilesFoundInScreen()
{

  //Antes de empezar, limpiamos el buffer
  //para evitar que se colapse

  int pos_in_HMI_file = 0;
  String szName = "";
  String type="";

  int color = 65535; //60868
  
  if (FILE_PTR_POS==0)
  {
      // Esto es solo para el parent dir, para que siempre salga arriba
      szName = FILES_FOUND_BUFF[FILE_PTR_POS].path;
      color = 2016;  // Verde
      szName = String("..     ") + szName;
      
      //writeString("");
      writeString("prevDir.txt=\"" + String(szName) + "\"");
      //writeString("");
      writeString("prevDir.pco=" + String(color));
      
      // Descartamos la posición cero del buffer porque es una posición especial
      FILE_PTR_POS=1;
  }

  for (int i=FILE_PTR_POS;i<=FILE_PTR_POS+12;i++)
  {
        szName = FILES_FOUND_BUFF[FILE_PTR_POS + pos_in_HMI_file].path;
        type = FILES_FOUND_BUFF[FILE_PTR_POS + pos_in_HMI_file].type;
        
        // Lo trasladamos a la pantalla
        if (type == "DIR")
        {
            //Directorio
            color = 60868;  // Amarillo
            szName = String("<DIR>  ") + szName;
        }     
        else if (type == "TAP" || type == "TZX")
        {
            //Fichero
            color = 65535;   // Blanco
        }
        else
        {
            color = 44405;  // gris apagado
        }

        printFileRows(pos_in_HMI_file, color, szName);
        pos_in_HMI_file++;
  }
}

void putFilesInScreen()
{

  //Antes de empezar, limpiamos el buffer
  //para evitar que se colapse

  int pos_in_HMI_file = 0;
  String szName = "";
  String type="";

  int color = 65535; //60868
  
  if (FILE_PTR_POS==0)
  {
      // Esto es solo para el parent dir, para que siempre salga arriba
      szName = FILES_BUFF[FILE_PTR_POS].path;
      color = 2016;  // Verde
      szName = String("..     ") + szName;
      
      //writeString("");
      writeString("prevDir.txt=\"" + String(szName) + "\"");
      //writeString("");
      writeString("prevDir.pco=" + String(color));
      
      // Descartamos la posición cero del buffer porque es una posición especial
      FILE_PTR_POS=1;
  }

  for (int i=FILE_PTR_POS;i<=FILE_PTR_POS+12;i++)
  {
        szName = FILES_BUFF[FILE_PTR_POS + pos_in_HMI_file].path;
        type = FILES_BUFF[FILE_PTR_POS + pos_in_HMI_file].type;
        
        // Lo trasladamos a la pantalla
        if (type == "DIR")
        {
            //Directorio
            color = 60868;  // Amarillo
            szName = String("<DIR>  ") + szName;
        }     
        else if (type == "TAP" || type == "TZX")
        {
            //Fichero
            color = 65535;   // Blanco
        }
        else
        {
            color = 44405;  // gris apagado
        }

        printFileRows(pos_in_HMI_file, color, szName);
        pos_in_HMI_file++;
  }

}

void putInHome()
{
    FILE_BROWSER_SEARCHING = false;  
    FILE_LAST_DIR="/";
    FILE_PREVIOUS_DIR="/";
    // Open root directory
    FILE_PTR_POS = 0;
    clearFilesInScreen();
    getFilesFromSD();
    putFilesInScreen();
}

void findTheTextInFiles()
{

    bool filesFound = false;

    if(FILES_FOUND_BUFF!=NULL)
    {
        free(FILES_FOUND_BUFF);
        FILES_FOUND_BUFF=NULL;
    }

    const int maxResults = 500;

    FILES_FOUND_BUFF = (tFileBuffer*)calloc(maxResults+1,sizeof(tFileBuffer));

    // Copiamos el primer registro que no cambia
    // porque es la ruta anterior.
    FILES_FOUND_BUFF[0].path = FILES_BUFF[0].path;
    FILES_FOUND_BUFF[0].type = FILES_BUFF[0].type;
    FILES_FOUND_BUFF[0].isDir = FILES_BUFF[0].isDir;
    
    if (FILE_BROWSER_SEARCHING)
    {
        // Buscamos el texto en el buffer actual.
        String fileRead = "";
        int j = 1;

        Serial.println();
        Serial.println();
        Serial.println(FILE_TXT_TO_SEARCH);

        for (int i=1;i<FILE_TOTAL_FILES;i++)
        {
            fileRead = FILES_BUFF[i].path;
            fileRead.toLowerCase();
            FILE_TXT_TO_SEARCH.toLowerCase();

            if (fileRead.indexOf(FILE_TXT_TO_SEARCH) != -1)
            {
                
                filesFound = true;

                if (j < maxResults)
                {
                    //Fichero localizado. Lo almacenamos
                    FILES_FOUND_BUFF[j].path = FILES_BUFF[i].path;
                    FILES_FOUND_BUFF[j].type = FILES_BUFF[i].type;
                    FILES_FOUND_BUFF[j].isDir = FILES_BUFF[i].isDir;
                    
                    Serial.println(fileRead);
                    
                    j++;
                }
                else
                {
                    // No se muestran mas resultados. Hemos llegado a "maxResults"
                    break;
                }
            }
        }

        FILE_TOTAL_FILES = j;       
        FILE_PTR_POS=0;     

        // Representamos ahora   
        clearFilesInScreen();
        putFilesFoundInScreen();
    }
}

void verifyCommand(String strCmd) 
{
   
  if (strCmd.indexOf("BKX=") != -1) 
  {
      // Con este procedimiento capturamos el bloque seleccionado
      // desde la pantalla.
      byte* buff = (byte*)calloc(8,sizeof(byte));
      strCmd.getBytes(buff, 7);
      String num = String(buff[4]+buff[5]+buff[6]+buff[7]);
      BLOCK_SELECTED = num.toInt();

      updateInformationMainPage();
  }

  if (strCmd.indexOf("INFB") != -1) 
  {
      // Con este comando nos indica la pantalla que 
      // está en modo FILEBROWSER
      FILE_BROWSER_OPEN = true;
  }

  if (strCmd.indexOf("SHR") != -1) 
  {
      // Con este comando nos indica la pantalla que 
      // está en modo FILEBROWSER
      FILE_BROWSER_SEARCHING = true;
      Serial.println("");
      Serial.println("");
      Serial.println(" ---------- Buscando ficheros");
      findTheTextInFiles();
      Serial.println(" ----------------------------");

  }

  if (strCmd.indexOf("OUTFB") != -1) 
  {
      // Con este comando nos indica la pantalla que 
      // está en modo FILEBROWSER
      FILE_BROWSER_OPEN = false;
  }

  if (strCmd.indexOf("GFIL") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero

      writeString("statusFILE.txt=\"GETTING FILES\"");

      getFilesFromSD();

      // if (FILE_BROWSER_SEARCHING)
      // {
      if (!FILE_DIR_OPEN_FAILED)
      {
          putFilesInScreen();
          FILE_STATUS = 1;
          // El GFIL desconecta el filtro de busqueda
          FILE_BROWSER_SEARCHING = false;
      }        
          //putInHome();
      // }
      // else
      // {
      //     if (!FILE_DIR_OPEN_FAILED)
      //     {
      //         putFilesInScreen();
      //         FILE_STATUS = 1;
      //     }        
      // }



      //writeString("");
      writeString("statusFILE.txt=\"\"");

  }

  if (strCmd.indexOf("FPUP") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      FILE_PTR_POS = FILE_PTR_POS - 13;

      if (FILE_PTR_POS < 0)
      {
          FILE_PTR_POS = 0;
      }

      if (!FILE_BROWSER_SEARCHING)
      {
          clearFilesInScreen();
          putFilesInScreen(); 
      }
      else
      {
          clearFilesInScreen();
          putFilesFoundInScreen(); 
      }
  }

  if (strCmd.indexOf("FPDOWN") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      if(FILE_TOTAL_FILES > 13)
      {
          FILE_PTR_POS = FILE_PTR_POS + 13;

          if (FILE_PTR_POS > (FILE_TOTAL_FILES-13))
          {
              FILE_PTR_POS = FILE_TOTAL_FILES-13;
          }

          if (!FILE_BROWSER_SEARCHING)
          {
              clearFilesInScreen();
              putFilesInScreen(); 
          }
          else
          {
              clearFilesInScreen();
              putFilesFoundInScreen(); 
          }
      }
  }

  if (strCmd.indexOf("FPHOME") != -1) 
  {
      // Con este comando nos indica la pantalla que quiere
      // le devolvamos ficheros en la posición actual del puntero
      putInHome();

      writeString("statusFILE.txt=\"GETTING FILES\"");

      getFilesFromSD();

      writeString("statusFILE.txt=\"\"");

  }

  if (strCmd.indexOf("CHD=") != -1) 
  {
      // Con este comando capturamos el directorio a cambiar
      byte* buff = (byte*)calloc(8,sizeof(byte));
      strCmd.getBytes(buff, 7);
      String num = String(buff[4]+buff[5]+buff[6]+buff[7]);

      FILE_IDX_SELECTED = num.toInt();

      //Cogemos el directorio
      FILE_DIR_TO_CHANGE = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED + FILE_PTR_POS].path + "/";    

      char* dir_ch = (char*)calloc(FILE_DIR_TO_CHANGE.length()+1,sizeof(char));
      FILE_DIR_TO_CHANGE.toCharArray(dir_ch,FILE_DIR_TO_CHANGE.length()+1);
      FILE_PREVIOUS_DIR = FILE_LAST_DIR;
      FILE_LAST_DIR = dir_ch;

      FILE_PTR_POS = 0;
      clearFilesInScreen();

      //writeString("");
      writeString("statusFILE.txt=\"GETTING FILES\"");

      getFilesFromSD();
      
      if (!FILE_DIR_OPEN_FAILED)
      {
          //putFilesInScreen();
          putFilesInScreen();
      }
      
      //writeString("");
      writeString("statusFILE.txt=\"\"");
  }
  
    if (strCmd.indexOf("PAR=") != -1) 
  {
      // Con este comando capturamos el directorio padre

      //Cogemos el directorio padre que siempre estará en el prevDir y por tanto
      //no hay que calcular la posición
      FILE_DIR_TO_CHANGE = FILES_BUFF[0].path;    

      char* dir_ch = (char*)calloc(FILE_DIR_TO_CHANGE.length()+1,sizeof(char));
      FILE_DIR_TO_CHANGE.toCharArray(dir_ch,FILE_DIR_TO_CHANGE.length()+1);
      FILE_LAST_DIR = dir_ch;

      if (FILE_DIR_TO_CHANGE.length() == 1)
      {
          //Es el nivel mas alto
          FILE_PREVIOUS_DIR = "/";
      }
      else
      {
          //Cogemos el directorio anterior de la cadena
          FILE_PREVIOUS_DIR = (char*)calloc(FILE_DIR_TO_CHANGE.length(),sizeof(char));
          FILE_PREVIOUS_DIR = getPreviousDirFromPath(FILE_LAST_DIR);
      }
      //
      //
      FILE_PTR_POS = 0;
      clearFilesInScreen();

      writeString("statusFILE.txt=\"GETTING FILES\"");
      
      getFilesFromSD();
      
      if (!FILE_DIR_OPEN_FAILED)
      {
          putFilesInScreen();
      }

      writeString("statusFILE.txt=\"\"");

  }

  if (strCmd.indexOf("LFI=") != -1) 
  {
      // Con este comando
      // devolvamos el fichero que se ha seleccionado en la pantalla
      byte* buff = (byte*)calloc(8,sizeof(byte));
      strCmd.getBytes(buff, 7);
      String num = String(buff[4]+buff[5]+buff[6]+buff[7]);

      FILE_IDX_SELECTED = num.toInt();

      //Cogemos el fichero
      if (!FILE_BROWSER_SEARCHING)
      {
          FILE_TO_LOAD = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED + FILE_PTR_POS].path;     

          Serial.println();
          Serial.println();
          Serial.println("File to load: " + FILE_TO_LOAD);    

          // Ya no me hace falta. Lo libero
          if (FILES_BUFF!=NULL)
          {
              free(FILES_BUFF);
              FILES_BUFF=NULL;
          }
          
      }
      else
      {
          FILE_TO_LOAD = FILE_LAST_DIR + FILES_FOUND_BUFF[FILE_IDX_SELECTED + FILE_PTR_POS].path;     

          Serial.println();
          Serial.println();
          Serial.println("File to load: " + FILE_TO_LOAD); 

          // Ya no me hace falta. Lo libero
          if(FILES_FOUND_BUFF!=NULL)
          {
              free(FILES_FOUND_BUFF);
              FILES_FOUND_BUFF = NULL;
          }

      }


      // Cambiamos el estado de fichero seleccionado
      if (FILE_TO_LOAD != "")
      {
          FILE_SELECTED = true;
      }
      else
      {
          FILE_SELECTED = false;
      }

      FILE_STATUS = 0;
      FILE_NOTIFIED = false;

      FILE_BROWSER_OPEN = false;

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

    LAST_MESSAGE = "Loading in progress. Please wait.";
    updateInformationMainPage();
  }

  if (strCmd.indexOf("PAUSE") != -1) 
  {
    PLAY = false;
    PAUSE = true;
    STOP = false;

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

    LAST_MESSAGE = "Tape stop. Press play to start again.";
    //writeString("");
    writeString("currentBlock.val=1");
    //writeString("");
    writeString("progressTotal.val=0");
    //writeString("");
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

    LAST_MESSAGE = "Tape stop. Press play to start again.";
    //writeString("");
    writeString("currentBlock.val=1");
    //writeString("");
    writeString("progressTotal.val=0");
    //writeString("");
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

  if (strCmd.indexOf("TXTF=") != -1) 
  {
    //Cogemos el valor
    byte buff[50];
    strCmd.getBytes(buff, 50);
    const int lencmd = 5;
    int n = lencmd;
    char phrase[50];
    char str = (char)buff[n];
    while (str != '@') 
    {
      phrase[n - lencmd] += (char)buff[n];
      n++;
      str = (char)buff[n];
    }

    // Ahora localizamos los ficheros
    FILE_TXT_TO_SEARCH = phrase;
    FILE_BROWSER_SEARCHING = true;
    findTheTextInFiles();
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
    //Serial.flush();
  }
}
