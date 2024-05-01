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

      SDmanager _sdm;
      SdFat32 _sdf;
      
      private:

      void clearFileBuffer()
      {
        // Borramos todos los registros
        for (int i;i<MAX_FILES_TO_LOAD;i++)
        {
          FILES_BUFF[i].isDir = false;
          FILES_BUFF[i].path = "";
          FILES_BUFF[i].type = "";
        }
      }

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

                  char szName[255];

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
                  //
                  fileC++;
              }
          }
      
          sdm.dir.close();
          sdm.file.close();
          
          return fileC;
      }      

      void getFilesFromSD()
      {
       
          // Cadena para un filePath
          char szName[255];
          int j = 0;

          clearFileBuffer();

          FILE_TOTAL_FILES = MAX_FILES_TO_LOAD;
          FILE_DIR_OPEN_FAILED = false;
          
          if (!sdm.dir.open(FILE_LAST_DIR.c_str())) 
          {
              SerialHW.println("");
              SerialHW.println("dir.open failed - " + String(FILE_LAST_DIR));
              FILE_DIR_OPEN_FAILED = true;
          }
          else
          {
              sdm.dir.rewindDirectory();
              sdm.file.rewind();
      
              // SerialHW.println("");
              // SerialHW.println("dir.open OK");
      
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
                          
                          //Pasamos todo a minusculas
                          //strlwr(szName);

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
                          else if (strstr(substr, ".tsx")) 
                          {
                              FILES_BUFF[j].type = "TSX";
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
                          else if (strstr(substr, ".wav")) 
                          {
                              FILES_BUFF[j].type = "WAV";
                              FILES_BUFF[j].path = String(szName);
                              j++;
                          }                          
                          else
                          {
                              // No se guarda
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

              // Sorting del array
              //

              FILE_TOTAL_FILES = j;
              sdm.dir.close();

              //getMemFree();
              //updateMem();
          }

          writeString("statusFILE.txt=\"FILES " + String(FILE_TOTAL_FILES-1) +"\"");
          //delay(1000);

          // Cerramos todos los ficheros (añadido el 25/07/2023)
          sdm.dir.close();
          sdm.file.close();
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
      
      String getPreviousDirFromPath(String path)
      {
          String strTmp = INITFILEPATH;
          //strncpy(strTmp,&INITFILEPATH[0],2);
          
          int lpath = path.length();
          
          for (int n=lpath-2;n>1;n--)
          {
              char chrRead = path[n];
              
              if (chrRead == '/')
              {
                  //Copiamos ahora el trozo de cadena restante
                  //ponemos n-1 para que copie también la "/"
                  //strlcpy(strTmp,path,n+2);
                  strTmp = path.substring(0,n+2);
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
      
        for (int i=0;i<=TOTAL_FILES_IN_BROWSER_PAGE;i++)
        {
            printFileRows(pos_in_HMI_file, color, szName);
            delay(5);
            //printFileRows(pos_in_HMI_file, color, szName);
            pos_in_HMI_file++;
        }
      
      }

      void showInformationAboutFiles()
      {
          // Limpiamos el path, para evitar cargar un fichero no seleccionado.
          writeString("path.txt=\"\"");  
          // Indicamos el path actual
          writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");
          // Actualizamos el total del ficheros leidos anteriormente.
          writeString("statusFILE.txt=\"FILES " + String(FILE_TOTAL_FILES - 1) +"\"");         
          
          // Obtenemos la pagina mostrada del listado de ficheros
          int totalPages = (FILE_TOTAL_FILES / TOTAL_FILES_IN_BROWSER_PAGE);
          
          if (FILE_TOTAL_FILES % TOTAL_FILES_IN_BROWSER_PAGE != 0)
          { 
              totalPages+=1;
          }

          int currentPage = (FILE_PTR_POS / TOTAL_FILES_IN_BROWSER_PAGE) + 1;

          // Actualizamos el total del ficheros leidos anteriormente.
          //writeString("tPage.txt=\"" + String(totalPages) +"\"");
          writeString("cPage.txt=\"" + String(currentPage) +" - " + String(totalPages) + "\"");
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
              else if (type == "TAP" || type == "TZX" || type == "TSX")
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

        showInformationAboutFiles(); 
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
            
            writeString("");
            writeString("prevDir.txt=\"" + String(szName) + "\"");
            writeString("");
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
              else if (type == "TAP" || type == "TZX" || type == "TSX")
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

        showInformationAboutFiles();        
      
      }

      void putInHome()
      {
          FILE_BROWSER_SEARCHING = false;  
          FILE_LAST_DIR_LAST = INITFILEPATH;
          FILE_LAST_DIR = INITFILEPATH;
          FILE_PREVIOUS_DIR = INITFILEPATH;
          // Open root directory
          FILE_PTR_POS = 0;
          clearFilesInScreen();
          getFilesFromSD();
          putFilesInScreen();
      }
      
      void findTheTextInFiles()
      {
      
          //bool filesFound = false;     
          const int maxResults = MAX_FILES_FOUND_BUFF;
      
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
      
              // SerialHW.println();
              // SerialHW.println();
              // SerialHW.println(FILE_TXT_TO_SEARCH);
      
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
                          
                          //SerialHW.println(fileRead);
                          
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
                  // Indicamos que ya no estamos en searching para que se pueda
                  // hacer refresco sin problemas
                  FILE_BROWSER_SEARCHING = false;
                  FILE_BROWSER_OPEN = true;
                  // Borramos todo 
                  clearFilesInScreen();                  
              }
              else
              {
                  // Limpiamos el mensaje
                  writeString("statusFILE.txt=\"FILES " + String(j) + "\"");
                  delay(125);

                  FILE_TOTAL_FILES = j;       
                  FILE_PTR_POS=0;     
          
                  // Representamos ahora   
                  clearFilesInScreen();
                  putFilesFoundInScreen();                  
              }
      

          }
      }
      
      void SerialHWSendData(String data) 
      {
      
        if (SerialHW.available() + data.length() < 64) 
        {
          // Lo envio
          SerialHW.println(data);
        } 
        else 
        {
          // Lo descarto
          //SerialHW.flush();
        }
      }            

      void refreshFiles()
      {
          // Refrescamos el listado de ficheros visualizado
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

      void resetBlockIndicators()
      {
          PROGRAM_NAME = "";
          PROGRAM_NAME_2 = "";
          strcpy(LAST_NAME,"              ");
          strcpy(LAST_TYPE,"                                   ");
          LAST_SIZE = 0;

          writeString("tape.totalBlocks.val=0");
          writeString("tape.currentBlock.val=0");
          writeString("tape.progressTotal.val=0");
          writeString("tape.progression.val=0");

          TOTAL_BLOCKS = 0;
          BLOCK_SELECTED = 0;
          BYTES_LOADED = 0;     
      }

      void proccesingEject()
      {
          PLAY = false;
          PAUSE = false;
          STOP = true;
          ABORT = false;
          REC = false;

          if (FILE_PREPARED)
          {
            SerialHW.println("EJECT command");
            EJECT = true;
          }
          else
          {
            EJECT = false;
          }
          
          resetBlockIndicators();
      }

      public:

      void resetIndicators()
      {
        resetBlockIndicators();
      }

      void saveID2B(int valEn)
      {
        // Almacena la polarizacion en el fichero abierto en un bloque ID 0x2B
        if(FILE_SELECTED)
        {
          // 
        }
      }

      void verifyCommand(String strCmd) 
      {
        
        // Reiniciamos
        LAST_COMMAND = "";

        if (strCmd.indexOf("BKX=") != -1) 
        {
            // Con este procedimiento capturamos el bloque seleccionado
            // desde la pantalla.
            uint8_t buff[8];

            strCmd.getBytes(buff, 7);
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
            BLOCK_SELECTED = num.toInt();
            
            // Esto lo hacemos para poder actualizar la info del TAPE
            FFWIND=true;
            RWIND=true;
            
            //updateInformationMainPage();
        }

        if (strCmd.indexOf("PAG=") != -1) 
        {
            // Con este procedimiento capturamos el bloque seleccionado
            // desde la pantalla.
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            
            String num = String(val);

            int pageSelected = num.toInt();
            int totalPages = (FILE_TOTAL_FILES / TOTAL_FILES_IN_BROWSER_PAGE);
            if (FILE_TOTAL_FILES % TOTAL_FILES_IN_BROWSER_PAGE != 0)
            {
                totalPages+=1;
            }            

            // SerialHW.println("");
            // SerialHW.println("Pagina seleccionada");
            // SerialHW.print(String(pageSelected));
            // SerialHW.print("/");
            // SerialHW.print(String(totalPages));

            if (pageSelected <= totalPages)
            {
                // Cogemos el primer item y refrescamos
                FILE_PTR_POS = (pageSelected * TOTAL_FILES_IN_BROWSER_PAGE) + 1;

                // Refrescamos el listado de ficheros visualizado
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

        if (strCmd.indexOf("INFB") != -1) 
        {
            // Con este comando nos indica la pantalla que 
            // está en modo FILEBROWSER
            proccesingEject();            
            FILE_BROWSER_OPEN = true;
        }
      
        if (strCmd.indexOf("SHR") != -1) 
        {
            // Con este comando nos indica la pantalla que 
            // está en modo FILEBROWSER
            FILE_BROWSER_SEARCHING = true;
            // SerialHW.println("");
            // SerialHW.println("");
            // SerialHW.println(" ---------- Buscando ficheros");
            findTheTextInFiles();
            // SerialHW.println(" ----------------------------");
      
        }
      
        if (strCmd.indexOf("OUTFB") != -1) 
        {
            // Con este comando nos indica la pantalla que 
            // está SALE del modo FILEBROWSER
            if (!FILE_SELECTED)
            {
                //FILE_LAST_DIR = &INITFILEPATH[0];
                //FILE_LAST_DIR_LAST =  &INITCHAR[0];
                FILE_BROWSER_OPEN = false;

                // SerialHW.println("");
                // SerialHW.println("");
                // SerialHW.println("No file was SELECTED: ");
            }
        }
      
        if (strCmd.indexOf("GFIL") != -1) 
        {
            // Con este comando nos indica la pantalla que quiere
            // le devolvamos ficheros en la posición actual del puntero
      
            //writeString("statusFILE.txt=\"GETTING FILES\"");
            //delay(125);
            FILE_PREPARED = false;
            FILE_SELECTED = false;

            resetIndicators();

            if (FILE_LAST_DIR_LAST != FILE_LAST_DIR)
            {

                if (sdm.dir.isOpen())
                {
                    sdm.dir.close();
                    // SerialHW.println("");
                    // SerialHW.println("");
                    // SerialHW.println(" Closing file before new browsing.");                
                }              

                getFilesFromSD();
            }

            if (!FILE_DIR_OPEN_FAILED)
            {

                // SerialHW.println("");
                // SerialHW.println("");
                // SerialHW.println("I'm here");

                clearFilesInScreen();
                putFilesInScreen();
                FILE_STATUS = 1;
                // El GFIL desconecta el filtro de busqueda
                FILE_BROWSER_SEARCHING = false;                   
            }      
       
          
            //writeString("statusFILE.txt=\"\"");
            //delay(125);
      
        }

        if (strCmd.indexOf("RFSH") != -1) 
        {
            // Recarga el directorio
            getFilesFromSD();       
            refreshFiles();
        }

      
        if (strCmd.indexOf("FPUP") != -1) 
        {
            // Con este comando nos indica la pantalla que quiere
            // le devolvamos ficheros en la posición actual del puntero
            FILE_PTR_POS = FILE_PTR_POS - TOTAL_FILES_IN_BROWSER_PAGE;
      
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
            if(FILE_TOTAL_FILES > TOTAL_FILES_IN_BROWSER_PAGE)
            {
                FILE_PTR_POS = FILE_PTR_POS + TOTAL_FILES_IN_BROWSER_PAGE;
      
                if (FILE_PTR_POS > FILE_TOTAL_FILES)
                {
                    FILE_PTR_POS -= TOTAL_FILES_IN_BROWSER_PAGE;
                }
      
                if (!FILE_BROWSER_SEARCHING)
                {
                    clearFilesInScreen();
                    putFilesInScreen(); 

                    // SerialHW.println("");
                    // SerialHW.println("");
                    // SerialHW.println("I'm in BROWSER mode yet.");
                }
                else
                {
                    clearFilesInScreen();
                    putFilesFoundInScreen(); 

                    // SerialHW.println("");
                    // SerialHW.println("");
                    // SerialHW.println("I'm in SEARCH mode yet.");
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
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));

            String num = String(val);
      
            FILE_IDX_SELECTED = num.toInt();
      
            //Cogemos el directorio

            FILE_DIR_TO_CHANGE = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED + FILE_PTR_POS].path + "/";    
      
            // Reserva dinamica de memoria
            String dir_ch = FILE_DIR_TO_CHANGE;

            //
            //FILE_DIR_TO_CHANGE.toCharArray(dir_ch,FILE_DIR_TO_CHANGE.length()+1);
            FILE_PREVIOUS_DIR = FILE_LAST_DIR;
            FILE_LAST_DIR = dir_ch;
            FILE_LAST_DIR_LAST = FILE_LAST_DIR;
     
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
            FILE_LAST_DIR = FILE_DIR_TO_CHANGE;
            FILE_LAST_DIR_LAST = FILE_LAST_DIR;

            if (FILE_DIR_TO_CHANGE.length() == 1)
            {
                //Es el nivel mas alto
                FILE_PREVIOUS_DIR = INITFILEPATH;
            }
            else
            {
                //Cogemos el directorio anterior de la cadena
                FILE_PREVIOUS_DIR = getPreviousDirFromPath(FILE_LAST_DIR);
            }
            //
            //
            FILE_PTR_POS = 0;
            clearFilesInScreen();            
            getFilesFromSD();
            
            if (!FILE_DIR_OPEN_FAILED)
            {
                putFilesInScreen();
            }
      
        }
      
        if (strCmd.indexOf("TRS=") != -1) 
        {
            // Con este comando
            // Borramos el fichero que se ha seleccionado en la pantalla
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);

            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
      
            FILE_IDX_SELECTED = num.toInt();
            FILE_SELECTED_DELETE = false;
      
            //Cogemos el fichero
            if (!FILE_BROWSER_SEARCHING)
            {
                FILE_TO_DELETE = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED + FILE_PTR_POS].path;     
      
                SerialHW.println();
                SerialHW.println();
                SerialHW.println("File to delete: " + FILE_TO_DELETE);  
                FILE_SELECTED_DELETE = true; 
            }
            else
            {
                FILE_TO_DELETE = FILE_LAST_DIR + FILES_FOUND_BUFF[FILE_IDX_SELECTED + FILE_PTR_POS].path;     
      
                SerialHW.println();
                SerialHW.println();
                SerialHW.println("File to delete in buffer: " + FILE_TO_DELETE); 
                FILE_SELECTED_DELETE = true;  
            }

            if (FILE_SELECTED_DELETE)
            {
              // Lo Borramos
                if (!_sdf.remove(FILE_TO_DELETE))
                {
                  SerialHW.println("Error to remove file. " + FILE_TO_DELETE);
                  writeString("currentDir.txt=\">> Error. File not removed <<\"");
                  delay(1500);
                  writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");                  
                }
                else
                {
                  FILE_SELECTED_DELETE = false;
                  SerialHW.println("File remove. " + FILE_TO_DELETE);
                  getFilesFromSD();       
                  refreshFiles();
                }
            }
        }      

        if (strCmd.indexOf("LFI=") != -1) 
        {
            // Con este comando
            // devolvamos el fichero que se ha seleccionado en la pantalla
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
      
            FILE_IDX_SELECTED = num.toInt();
            FILE_SELECTED = false;
      
            //Cogemos el fichero
            if (!FILE_BROWSER_SEARCHING)
            {
                FILE_TO_LOAD = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED + FILE_PTR_POS].path;     
      
                // SerialHW.println();
                // SerialHW.println();
                // SerialHW.println("File to load: " + FILE_TO_LOAD);    
      
                // Ya no me hace falta. Lo libero
                // if (FILES_BUFF!=NULL)
                // {
                //     free(FILES_BUFF);
                //     FILES_BUFF=NULL;
                // }
                
            }
            else
            {
                FILE_TO_LOAD = FILE_LAST_DIR + FILES_FOUND_BUFF[FILE_IDX_SELECTED + FILE_PTR_POS].path;           
            }
      
      
            // Cambiamos el estado de fichero seleccionado
            if (FILE_TO_LOAD != "")
            {
                FILE_SELECTED = true;
            }

      
            FILE_STATUS = 0;
            FILE_NOTIFIED = false;
      
            FILE_BROWSER_OPEN = false;
      
        }
      
        if (strCmd.indexOf("FRQ1") != -1)
        {
          DfreqCPU = 3250000.0;
        }
        if (strCmd.indexOf("FRQ2") != -1)
        {
          DfreqCPU = 3300000.0;
        }
        if (strCmd.indexOf("FRQ3") != -1)
        {
          DfreqCPU = 3330000.0;
        }
        if (strCmd.indexOf("FRQ4") != -1)
        {
          DfreqCPU = 3350000.0;
        }
        if (strCmd.indexOf("FRQ5") != -1)
        {
          DfreqCPU = 3400000.0;
        }
        if (strCmd.indexOf("FRQ6") != -1)
        {
          DfreqCPU = 3430000.0;
        }
        if (strCmd.indexOf("FRQ7") != -1)
        {
          DfreqCPU = 3450000.0;
        }
        if (strCmd.indexOf("FRQ8") != -1)
        {
          DfreqCPU = 3500000.0;
        }

        if (strCmd.indexOf("LCDON") != -1) 
        {
            LCD_ON = true;
        }
      
        if (strCmd.indexOf("FFWD") != -1) 
        {
          if (LOADING_STATE == 2 || LOADING_STATE == 3 || LOADING_STATE == 0) 
          {

            SerialHW.println("Ahora muestro el bloque seleccionado");
            SerialHW.println(String(BLOCK_SELECTED));

            BLOCK_SELECTED++;
      
            if (BLOCK_SELECTED > TOTAL_BLOCKS - 1) 
            {
              BLOCK_SELECTED = 0;
            }

            //updateInformationMainPage();

            FFWIND = true;
            RWIND = false;

            SerialHW.println("Ahora muestro el bloque seleccionado");
            SerialHW.println(String(BLOCK_SELECTED));

          }
        }
      
        if (strCmd.indexOf("RWD") != -1) 
        {
          if (LOADING_STATE == 2 || LOADING_STATE == 3 || LOADING_STATE == 0) 
          {
      
            BLOCK_SELECTED--;
      
            if (BLOCK_SELECTED < 0) 
            {
              BLOCK_SELECTED = TOTAL_BLOCKS - 1;
            }
      
            //updateInformationMainPage();

            FFWIND = false;
            RWIND = true;

          }
        }
      
        if (strCmd.indexOf("PLAY") != -1) 
        {
          PLAY = true;
          PAUSE = false;
          STOP = false;
          ABORT = false;
        }
      
        if (strCmd.indexOf("REC") != -1) 
        {
          PLAY = false;
          PAUSE = false;
          STOP = false;
          REC = true;
          ABORT = false;
        }

        if (strCmd.indexOf("PAUSE") != -1) 
        {
          PLAY = false;
          PAUSE = true;
          STOP = false;
          ABORT = true;
      
          LAST_MESSAGE = "Tape paused. Press play to continue load or select block.";
          //updateInformationMainPage();
        }
      
        if (strCmd.indexOf("STOP") != -1) 
        {
          PLAY = false;
          PAUSE = false;
          STOP = true;
          ABORT = true;

          BLOCK_SELECTED = 0;
          BYTES_LOADED = 0;
        }
      
        if (strCmd.indexOf("EJECT") != -1) 
        {
            PLAY = false;
            PAUSE = false;
            STOP = true;
            ABORT = true;

            proccesingEject();            
        }
      
        if (strCmd.indexOf("ABORT") != -1) 
        {
            ABORT=true;
        }

        if (strCmd.indexOf("VOL=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];
          MAIN_VOL = valVol;
          
          // Ajustamos el volumen
          SerialHW.println("Main volume value=" + String(MAIN_VOL));

          //ESP32kit.setVolume(MAIN_VOL);
          //ESP32kit.maxAmplitude = 
          //_zxp.set_amplitude(MAIN_VOL * 32767 / 100);
        }

        if (strCmd.indexOf("VRR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          MAIN_VOL_R = valVol;
          
          // Ajustamos el volumen
          #ifdef DEBUGMODE
            SerialHW.println("");
            SerialHW.println("R-Channel volume value=" + String(MAIN_VOL_R));
            SerialHW.println("");
          #endif
        }

        if (strCmd.indexOf("VLL=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];
          MAIN_VOL_L = valVol;
          
          // Ajustamos el volumen
          #ifdef DEBUGMODE
            SerialHW.println("");
            SerialHW.println("L-Channel volume value=" + String(MAIN_VOL_L));
            SerialHW.println("");
          #endif
        }

        if (strCmd.indexOf("THR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valThr = (int)buff[4];
          SCHMITT_THR = valThr;
          SerialHW.println("Threshold value=" + String(SCHMITT_THR));
          
          // value = byte1+byte2*256+byte3*65536+byte4*16777216

          // for (int i = 0;i<totalSamplesForTh;i++)
          // {
          //   size_t len = _kit.read(buffer, BUFFER_SIZE);
          //   analyzeSamplesForThreshold(len);
          // }
        }

        // Habilitar terminadores para forzar siguiente pulso a HIGH
        if (strCmd.indexOf("TER=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Habilita terminadores
              APPLY_END = true;
          }
          else
          {
              // Deshabilita terminadores
              APPLY_END = false;
          }

          log("Terminadores =" + String(APPLY_END));
        }

        // Polarización de la señal
        if (strCmd.indexOf("PLZ=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Empieza en UP
              POLARIZATION = down;
              LAST_EAR_IS = down;
              INVERSETRAIN = true;
          }
          else
          {
              // Empieza en DOWN
              POLARIZATION = up;
              LAST_EAR_IS = up;
              INVERSETRAIN = false;
          }
          SerialHW.println("Polarization =" + String(INVERSETRAIN));
        }

        // Nivel LOW a cero
        if (strCmd.indexOf("ZER=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // El nivel bajo es 0
              ZEROLEVEL = true;
          }
          else
          {
              // El nivel bajo es -32768
              ZEROLEVEL = false;
          }
          SerialHW.println("Down level is ZERO =" + String(ZEROLEVEL));
        }

        // Enable Schmitt Trigger threshold adjust
        if (strCmd.indexOf("ESH=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              EN_SCHMITT_CHANGE = true;
          }
          else
          {
              EN_SCHMITT_CHANGE = false;
          }
          SerialHW.println("Threshold enable=" + String(EN_SCHMITT_CHANGE));
        }

        // Habilita los dos canales
        if (strCmd.indexOf("STE=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          EN_STEREO = valEn;

          SerialHW.println("Mute enable=" + String(EN_STEREO));
        }

        // Enable MIC left channel - Option
        if (strCmd.indexOf("EMI=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              SWAP_MIC_CHANNEL = true;
          }
          else
          {
              SWAP_MIC_CHANNEL = false;
          }
          SerialHW.println("MIC LEFT enable=" + String(SWAP_MIC_CHANNEL));
        }

        // Enable MIC left channel - Option
        if (strCmd.indexOf("EAR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              SWAP_EAR_CHANNEL = true;
          }
          else
          {
              SWAP_EAR_CHANNEL = false;
          }
          SerialHW.println("EAR LEFT enable=" + String(SWAP_EAR_CHANNEL));
        }

        // Save polarization in ID 0x2B
        if (strCmd.indexOf("I2B=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          saveID2B(valEn);

          log("ID 0x2B = " + String(valEn));
        }

        // Sampling rate
        if (strCmd.indexOf("SAM=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==0)
          {
              SAMPLING_RATE = 48000;
          }
          else if(valEn==1)
          {
              SAMPLING_RATE = 44100;
          }
          else if(valEn==2)
          {
              SAMPLING_RATE = 32000;
          }
          else if(valEn==3)
          {
              SAMPLING_RATE = 22050;
          }
          log("Sampling rate =" + String(SAMPLING_RATE));
        }

        // Habilitar recording sobre WAV file
        if (strCmd.indexOf("WAV=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Habilita terminadores
              MODEWAV = true;
          }
          else
          {
              // Deshabilita terminadores
              MODEWAV = false;
          }

          logln("Modo WAV =" + String(MODEWAV));
        }

        // Show data debug by serial console
        if (strCmd.indexOf("SDD=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              SHOW_DATA_DEBUG = true;
          }
          else
          {
              SHOW_DATA_DEBUG = false;
          }
          SerialHW.println("SHOW_DATA_DEBUG enable=" + String(SHOW_DATA_DEBUG));
        }

        // // Pulses width
        // int MIN_SYNC = 13;
        // int MAX_SYNC = 19;
        // int MIN_BIT0 = 1;
        // int MAX_BIT0 = 39;
        // int MIN_BIT1 = 40;
        // int MAX_BIT1 = 65;
        // int MIN_LEAD = 50;
        // int MAX_LEAD = 56;
        // int MAX_PULSES_LEAD = 800;        

        if (strCmd.indexOf("MP1=") != -1) 
        {
          //minSync1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_SYNC=val;
          SerialHW.println("MP1=" + String(MIN_SYNC));
        }

        if (strCmd.indexOf("MP2=") != -1) 
        {
          //maxSync1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_SYNC=val;
          SerialHW.println("MP2=" + String(MAX_SYNC));
        }

        if (strCmd.indexOf("MP3=") != -1) 
        {
          //minBit0
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_BIT0=val;
          SerialHW.println("MP3=" + String(MIN_BIT0));
        }

        if (strCmd.indexOf("MP4=") != -1) 
        {
          //maxBit0
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_BIT0=val;
          SerialHW.println("MP4=" + String(MAX_BIT0));
        }

        if (strCmd.indexOf("MP5=") != -1) 
        {
          //minBit1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_BIT1=val;
          SerialHW.println("MP5=" + String(MIN_BIT1));
        }

        if (strCmd.indexOf("MP6=") != -1) 
        {
          //maxBit1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_BIT1=val;
          SerialHW.println("MP6=" + String(MAX_BIT1));
        }

        if (strCmd.indexOf("MP7=") != -1) 
        {
          //max pulses lead
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);

          // SerialHW.println("0: " + String((char)buff[0]));
          // SerialHW.println("1: " + String((char)buff[1]));
          // SerialHW.println("2: " + String((char)buff[2]));
          // SerialHW.println("3: " + String((char)buff[3]));
          // SerialHW.println("4: " + String(buff[4]));
          // SerialHW.println("5: " + String(buff[5]));
          // SerialHW.println("6: " + String(buff[6]));
          // SerialHW.println("7: " + String(buff[7]));

          long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
          //
          MAX_PULSES_LEAD=val;
          SerialHW.println("MP7=" + String(MAX_PULSES_LEAD));
        }

        if (strCmd.indexOf("MP8=") != -1) 
        {
          //minLead
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_LEAD=val;
          SerialHW.println("MP8=" + String(MIN_LEAD));
        }

        if (strCmd.indexOf("MP9=") != -1) 
        {
          //maxLead
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = buff[4];
          //
          MAX_LEAD=val;
          SerialHW.println("MP9=" + String(MAX_LEAD));
        }

        if (strCmd.indexOf("VOLUP") != -1) 
        {
          MAIN_VOL += 1;
          
          if (MAIN_VOL >100)
          {
            MAIN_VOL = 100;

          }
          // SerialHW.println("");
          // SerialHW.println("VOL UP");
          // SerialHW.println("");
        }        

        if (strCmd.indexOf("VOLDW") != -1) 
        {
          MAIN_VOL -= 1;
          
          if (MAIN_VOL < 30)
          {
            MAIN_VOL = 30;
          }
          // SerialHW.println("");
          // SerialHW.println("VOL DOWN");
          // SerialHW.println("");
        }
      
        if (strCmd.indexOf("TXTF=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[50];
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

      void writeString(String stringData) 
      {
      
          //SerialHW.flush();
      
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
      
          for (int i = 0; i < stringData.length(); i++) 
          {
            // Enviamos los datos
            SerialHW.write(stringData[i]);
          }
      
          // Indicamos a la pantalla que ya hemos enviado todos
          // los datos, con este triple uint8_t 0xFF
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
      
          if(FILE_BROWSER_OPEN)
          {
              // Metemos un delay de 10ms
              // menos da problemas.
              //delay(10);       
          }
      }
      
      void setBasicFileInformation(char* name,char* typeName,int size)
      {
          LAST_SIZE = size;
          strncpy(LAST_NAME,name,14);
          strncpy(LAST_TYPE,typeName,35);
      }

      void clearInformationFile()
      {
          PROGRAM_NAME = "";
          LAST_SIZE = 0;
          strncpy(LAST_NAME,"",1);
          strncpy(LAST_TYPE,"",1);

          //updateInformationMainPage();
      }

      void updateInformationMainPage() 
      {            

        if (PLAY)
        {
          writeString("debug.dataOffset1.txt=\"" + dataOffset1 +"\"");
          writeString("debug.dataOffset2.txt=\"" + dataOffset2 +"\"");
          writeString("debug.dataOffset3.txt=\"" + dataOffset3 +"\"");
          writeString("debug.dataOffset4.txt=\"" + dataOffset4 +"\"");

          writeString("debug.offset1.txt=\"" + Offset1 +"\"");
          writeString("debug.offset2.txt=\"" + Offset2 +"\"");
          writeString("debug.offset3.txt=\"" + Offset3 +"\"");
          writeString("debug.offset4.txt=\"" + Offset4 +"\"");

          // DEBUG Information
          writeString("debug.blockLoading.txt=\"" + String(BLOCK_SELECTED) +"\"");
          // Esto falla
          writeString("debug.partLoading.txt=\"" + String(PARTITION_BLOCK) +"\"");
          writeString("debug.totalParts.txt=\"" + String(TOTAL_PARTS) +"\"");

          writeString("debug.dbgBlkInfo.txt=\"" + dbgBlkInfo +"\"");
          writeString("debug.dbgPauseAB.txt=\"" + dbgPauseAB +"\"");
          writeString("debug.dbgSync1.txt=\"" + dbgSync1 +"\"");
          writeString("debug.dbgSync2.txt=\"" + dbgSync2 +"\"");
          writeString("debug.dbgBit1.txt=\"" + dbgBit1 +"\"");
          writeString("debug.dbgBit0.txt=\"" + dbgBit0 +"\"");
          writeString("debug.dbgTState.txt=\"" + dbgTState +"\"");
          writeString("debug.dbgRep.txt=\"" + dbgRep +"\"");
        }

        if (TOTAL_BLOCKS != 0 || REC || EJECT) 
        {
      
          // Enviamos información al HMI
          if (TYPE_FILE_LOAD == "TZX" || REC)
          {
              writeString("name.txt=\"" + PROGRAM_NAME + " : " + PROGRAM_NAME_2 + "\"");
              writeString("tape2.name.txt=\"" + PROGRAM_NAME + " : " + PROGRAM_NAME_2 + "\"");
          }
          else
          {
              writeString("name.txt=\"" + PROGRAM_NAME + "\"");
              writeString("tape2.name.txt=\"" + PROGRAM_NAME + "\"");
          }
          
          writeString("size.txt=\"" + String(LAST_SIZE) + " bytes\"");
          writeString("tape2.size.txt=\"" + String(LAST_SIZE) + " bytes\"");

          String cmpTypeStr = String(LAST_NAME);
          cmpTypeStr.trim();


          if (TYPE_FILE_LOAD == "TZX" || REC)
          {
              writeString("type.txt=\"" + String(LAST_TYPE) + " " + LAST_TZX_GROUP + "\"");
              writeString("tape2.type.txt=\"" + String(LAST_TYPE) + " " + LAST_TZX_GROUP + "\"");
          }
          else
          {
              writeString("type.txt=\"" + String(LAST_TYPE) + "\"");
              writeString("tape2.type.txt=\"" + String(LAST_TYPE) + "\"");
                   
              // writeString("name.txt=\"" + PROGRAM_NAME + " : " + String(LAST_NAME) + "\"");           
              // writeString("tape2.name.txt=\"" + PROGRAM_NAME + " : " + String(LAST_NAME) + "\"");     

              writeString("name.txt=\"" + PROGRAM_NAME + "\"");           
              writeString("tape2.name.txt=\"" + PROGRAM_NAME + "\"");
          }
      
          //writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));
          //writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));
        }
        else if (TOTAL_BLOCKS == 0)
        {
          //writeString("totalBlocks.val=0");
          //writeString("currentBlock.val=0");
        }
      
        writeString("g0.txt=\"" + LAST_MESSAGE + "\"");
        writeString("progression.val=" + String(PROGRESS_BAR_BLOCK_VALUE));
        writeString("progressTotal.val=" + String(PROGRESS_BAR_TOTAL_VALUE)); 
        writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));
        writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));         

        updateMem();
      }
      
      int getEndCharPosition(String str,int start)
      {
        //SerialHW.println(">> " + str);
        int pos=-1;
        int stateCr=0;
        for (int i = start; i < str.length(); i++) 
        {
          char cr = str[i];
          //SerialHW.println(String(cr) + " - " + String(int(cr)));

          // Buscamos 0xFF 0xFF 0xFF
          switch(stateCr)
          {
            case 0:
              // Es el primero
              if(int(cr)==32)
              {
                stateCr=1;
              }              
              else
              {
                stateCr=0;
                pos = -1;
              }
              break;

            case 1:
              // Es el segundo
              if(int(cr)==32)
              {
                stateCr=2;
              }              
              else
              {
                stateCr=0;
                pos = -1;
              }
              break;
            
            case 2:
              // Es el separador
              if(int(cr)==32)
              {
                pos = i;
                i=str.length();
              }
              else
              {
                stateCr=0;
                pos = -1;
              }
              break;
          }

        }

        //SerialHW.println("Pos: " + String(pos));
        return pos;
      }

      void getSeveralCommands(String strCmd)
      {

          // por cada comando lanza. Los comandos están
          // separados por 0x0A y 0x0D
          String str = "";
          int cStart=0;
          int lastcStart=0;

          // Buscamos el separador de comandos que es 
          // 0xFF 0xFF 0xFF
          int cPos = getEndCharPosition(strCmd,cStart);          

          if (cPos==-1)
          {
            // Solo hay un comando
            //SerialHW.println("Only one command");
            verifyCommand(strCmd);
          }
          else
          {
            // Hay varios
            while(cPos < strCmd.length() && cPos!=-1)
            {
              //SerialHW.println("Start,pos,last: " + String(cStart) + "," + String(cPos) + "," + String(lastcStart));

              str=strCmd.substring(cStart,cPos-3);
              //SerialHW.println("Command: " + str);
              // Verificamos el comando
              verifyCommand(str);
              // Adelantamos el puntero
              cStart = cPos+1;
              // Buscamos el siguiente
              cPos = getEndCharPosition(strCmd,cStart);              
            }
          }
      }

      void readUART() 
      {
        if (SerialHW.available() >= 1) 
        {
          // get the new uint8_t:
          while (SerialHW.available())
          {
            String strCmd = SerialHW.readString();

            //ECHO
            //SerialHW.println(strCmd);
            getSeveralCommands(strCmd);
          }
          //SerialHW.flush();
        }
      }

      void set_SDM(SDmanager sdm)
      {
        _sdm = sdm;
      }
      
      void set_sdf(SdFat32 sdf)
      {
        _sdf = sdf;
      } 

      void getMemFree()
      {
          SerialHW.println("");
          SerialHW.println("");
          SerialHW.println("> MEM REPORT");
          SerialHW.println("------------------------------------------------------------");
          SerialHW.println("");
          SerialHW.println("Total heap: " + String(ESP.getHeapSize() / 1024) + "KB");
          SerialHW.println("Free heap: " + String(ESP.getFreeHeap() / 1024) + "KB");
          SerialHW.println("Total PSRAM: " + String(ESP.getPsramSize() / 1024) + "KB");
          SerialHW.println("Free PSRAM: " + String (ESP.getFreePsram() / 1024) + "KB");  
          SerialHW.println("");
          SerialHW.println("------------------------------------------------------------");
          //
          updateMem();
      }           

      void updateMem()
      {
          writeString("menu.totalPSRAM.txt=\"StTsk0: " + String(stackFreeCore0) + "KB | " + String(ESP.getPsramSize() / 1024) + " | " + String(ESP.getHeapSize() / 1024) + " KB\"");
          writeString("menu.freePSRAM.txt=\"StTsk1: "  + String(stackFreeCore1) + "KB | " + String(ESP.getFreePsram() / 1024) + " | " + String(ESP.getFreeHeap() / 1024) + " KB\"");
      }

      // Constructor
      HMI()
      {}
};
