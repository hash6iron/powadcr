/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: HMI.h
    
    Creado por:
      Copyright (c) Antonio Tamairón. 2023  / https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/
    
    Descripción:
    Conjunto de recursos para la gestión de la pantalla TJC

    Version: 0.2

    Historico de versiones
    v.0.1 - Version inicial
    v.0.2 - Convertida en class

    Derechos de autor y distribución
    --------------------------------
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.
    
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    
    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
    
    To Contact the dev team you can write to hash6iron@gmail.com
 +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

#pragma once

class HMI
{

      private:
      
      int countFiles(char* path)
      {
          int fileC = -1;
      
          sdm.dir.rewindDirectory();

          if (sdm.dir != 0)
          {
              sdm.dir.close();
          }

          if (sdm.dir.open(path)) 
          {
              // Si se ha abierto. Partimos de que el directorio
              // esta vacio.
              fileC = 0;

              if (sdm.file != 0)
              {
                  sdm.file.close();
              }

              while (sdm.file.openNext(&sdm.dir, O_READ)) //O_RDONLY
              {

                  char* szName = (char*)calloc(255,sizeof(char));

                  if (sdm.file.isDir())
                  {
                      sdm.file.getName(szName,254);
                      sdm.sdf.chdir(szName);
                      sdm.file.close();
                  }
                  else
                  {
                      // Es un fichero
                      sdm.file.getName(szName,254);
                      sdm.file.close();
                  }
                  fileC++;

                  if (szName != NULL)
                  {
                    free(szName);
                    szName = NULL;
                  }
              }
          }
      
          sdm.dir.close();
          sdm.file.close();
          
          return fileC;
      }      

      void getFilesFromSD()
      {
          char* szName = (char*)calloc(255,sizeof(char));
          //szName = "\0";
      
          int j = 0;
      
          // Dimensionamos el array con calloc
          if (FILES_BUFF != NULL)
          {
              // Liberamos primero el espacio anterior
              free(FILES_BUFF);
              FILES_BUFF = NULL;
          }
      
          // FILE_TOTAL_FILES = countFiles(FILE_LAST_DIR);
          // Serial.println("");
          // Serial.println("Files to load - " + String(FILE_TOTAL_FILES));

          FILE_TOTAL_FILES = MAX_FILES_TO_LOAD;
          
          FILES_BUFF = (tFileBuffer*)calloc(FILE_TOTAL_FILES+1,sizeof(tFileBuffer));
            
          FILE_DIR_OPEN_FAILED = false;
          
          if (!sdm.dir.open(FILE_LAST_DIR)) 
          {
              Serial.println("");
              Serial.println("dir.open failed - " + String(FILE_LAST_DIR));
              FILE_DIR_OPEN_FAILED = true;
          }
          else
          {
              sdm.dir.rewindDirectory();
              sdm.file.rewind();
      
              Serial.println("");
              Serial.println("dir.open OK");
      
              FILE_DIR_OPEN_FAILED = false;
      
              //  AÑADIMOS AL INICIO EL PARENT DIR
              FILES_BUFF[0].isDir = true;
              FILES_BUFF[0].type = "PAR";
              FILES_BUFF[0].path = String(FILE_PREVIOUS_DIR);
      
              j++;
              
              while (sdm.file.openNext(&sdm.dir, O_RDONLY) && j < FILE_TOTAL_FILES)
              {
                  if (sdm.file.isDir())
                  {
                      if (!sdm.file.isHidden())
                      {
                          sdm.file.getName(szName,254);     
                          //Cambiamos 
                          //sdm.sdf.chdir(szName);
      
                          FILES_BUFF[j].isDir = true;
                          FILES_BUFF[j].type = "DIR";
                          FILES_BUFF[j].path = String(szName);      
                          
                          j++;
                      }
                      else
                      {
                        // No se muestra el dir porque está oculto
                      }
      
                  }
                  else
                  {
                      if (!sdm.file.isHidden())
                      {
                          // Es un fichero
                          sdm.file.getName(szName,254);
                          int8_t len = strlen(szName);
                          char* substr = strlwr(szName + (len - 4));    

                          FILES_BUFF[j].isDir = false;      

                          if (strstr(substr, ".tap")) 
                          {
                              FILES_BUFF[j].type = "TAP";
                              FILES_BUFF[j].path = String(szName);
                              j++;
                          }
                          else if (strstr(substr, ".tzx")) 
                          {
                              FILES_BUFF[j].type = "TZX";
                              FILES_BUFF[j].path = String(szName);
                              j++;
                          }
                          else if (strstr(substr, ".sna")) 
                          {
                              FILES_BUFF[j].type = "SNA";
                              FILES_BUFF[j].path = String(szName);
                              j++;
                          }                    
                          else if (strstr(substr, ".z80")) 
                          {
                              FILES_BUFF[j].type = "Z80";
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
      
                  sdm.file.close();

                  if ((j % EACH_FILES_REFRESH) == 0)
                  {
                      writeString("statusFILE.txt=\"FILES " + String(j) +"\"");

                  }
                        
              }
      
              FILE_TOTAL_FILES = j;
              sdm.dir.close();

              getMemFree();
          }

          writeString("statusFILE.txt=\"FILES " + String(FILE_TOTAL_FILES-1) +"\"");
          //delay(1000);

          // Cerramos todos los ficheros (añadido el 25/07/2023)
          sdm.dir.close();
          sdm.file.close();

          Serial.println("");
          Serial.println("");
          Serial.println("TOTAL FILES READ: " + String(FILE_TOTAL_FILES-1));
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
          strTmp = &INITFILEPATH[0];
          
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
            delay(5);
            //printFileRows(pos_in_HMI_file, color, szName);
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

              //Realizamos dos pasadas para evitar temas de perdida de información
              printFileRows(pos_in_HMI_file, color, szName);
              delay(5);
              //printFileRows(pos_in_HMI_file, color, szName);

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
              delay(5);
              //printFileRows(pos_in_HMI_file, color, szName);
              pos_in_HMI_file++;
        }
      
      }
      
      void putInHome()
      {
          FILE_BROWSER_SEARCHING = false;  
          FILE_LAST_DIR=&INITFILEPATH[0];
          FILE_PREVIOUS_DIR=&INITFILEPATH[0];
          // Open root directory
          FILE_PTR_POS = 0;
          clearFilesInScreen();
          getFilesFromSD();
          putFilesInScreen();
      }
      
      void findTheTextInFiles()
      {
      
          //bool filesFound = false;
      
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
          
              writeString("statusFILE.txt=\"SEARCHING FILES\"");
              delay(125);
      
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
                      
                      //filesFound = true;
      
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
      
              if (j<=1)
              {
                  // No hay resultados
                  writeString("statusFILE.txt=\"NO FILES FOUND\"");
                  delay(125);
              }
              else
              {
                  // Limpiamos el mensaje
                  writeString("statusFILE.txt=\"\"");
                  delay(125);
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
      
            //writeString("statusFILE.txt=\"GETTING FILES\"");
            //delay(125);
      
            if (sdm.dir.isOpen())
            {
                sdm.dir.close();
                Serial.println("");
                Serial.println("");
                Serial.println(" Closing file before new browsing.");                
            }

            getFilesFromSD();
      
            if (!FILE_DIR_OPEN_FAILED)
            {
                clearFilesInScreen();
                putFilesInScreen();
                FILE_STATUS = 1;
                // El GFIL desconecta el filtro de busqueda
                FILE_BROWSER_SEARCHING = false;
            }        
          
            //writeString("statusFILE.txt=\"\"");
            delay(125);
      
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
      
            //writeString("statusFILE.txt=\"GETTING FILES\"");
      
            getFilesFromSD();
      
            //writeString("statusFILE.txt=\"\"");
      
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
            //writeString("statusFILE.txt=\"GETTING FILES\"");
      
            getFilesFromSD();
            
            if (!FILE_DIR_OPEN_FAILED)
            {
                //putFilesInScreen();
                putFilesInScreen();
            }
            
            //writeString("");
            //writeString("statusFILE.txt=\"\"");
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
                FILE_PREVIOUS_DIR = &INITFILEPATH[0];
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
      
            //writeString("statusFILE.txt=\"GETTING FILES\"");
            
            getFilesFromSD();
            
            if (!FILE_DIR_OPEN_FAILED)
            {
                putFilesInScreen();
            }
      
            //writeString("statusFILE.txt=\"\"");
      
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
      
      
        if (strCmd.indexOf("VOL=") != -1) 
        {
          //Cogemos el valor
          byte buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];
          MAIN_VOL = valVol;
          
          // Ajustamos el volumen
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

      public:
        
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
              //delay(10);       
          }
      }
      
      void setBasicFileInformation(char* name,char* typeName,int size)
      {
          LAST_NAME = name;
          LAST_SIZE = size;
          LAST_TYPE = typeName;
      }

      void updateInformationMainPage() 
      {

        // Serial.println ();
        // Serial.println ();
        // Serial.println ("Updating Main Page");
        
        if (TOTAL_BLOCKS != 0) 
        {
      
          // Cargamos la información
          // LAST_NAME = globalTAP.descriptor[BLOCK_SELECTED].name;
          // LAST_SIZE = globalTAP.descriptor[BLOCK_SELECTED].size;
          // LAST_TYPE = globalTAP.descriptor[BLOCK_SELECTED].typeName;

          // Enviamos información al HMI
          writeString("name.txt=\"" + PROGRAM_NAME + "\"");

          //writeString("screen2.name.txt=\"" + PROGRAM_NAME + "\"");

          writeString("size.txt=\"" + String(LAST_SIZE - 2) + " bytes\"");

          //writeString("screen2.size.txt=\"" + String(LAST_SIZE - 2) + " bytes\"");

          String cmpTypeStr = String(LAST_NAME);
          cmpTypeStr.trim();

          // Serial.println("");
          // Serial.println("");
          // Serial.println(">" + cmpTypeStr + "<");

          if (cmpTypeStr != "")
          {
              writeString("type.txt=\"" + String(LAST_TYPE) + ": " + String(LAST_NAME) + "\"");            
          }
          else
          {
              writeString("type.txt=\"" + String(LAST_TYPE) + " " + String(LAST_NAME) + "\"");            
          }


          //writeString("screen2.type.txt=\"" + String(LAST_TYPE) + ": " + String(LAST_NAME) + "\"");
      
          writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));

          //writeString("screen2.totalBlocks.val=" + String(TOTAL_BLOCKS));

          writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));

          //writeString("screen2.currentBlock.val=" + String(BLOCK_SELECTED + 1));
        }
      
        //writeString("");
        writeString("g0.txt=\"" + LAST_MESSAGE + "\"");
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

      void set_SDM(SDmanager sdm)
      {
        //_sdm = sdm;
      }
      
      // Constructor
      HMI()
      {}
};
