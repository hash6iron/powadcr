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
      File32 fFileLST;
      
      private:

      struct tFileLST
      {
          int ID=0;
          char type='F';
          int seek=0;
          String fileName="";
          String fileType="";
      };

      void clearFileBuffer()
      {
        // Borramos todos los registros
        for (int i=0;i<MAX_FILES_TO_LOAD;i++)
        {
          FILES_BUFF[i].isDir = false;
          FILES_BUFF[i].path = "";
          FILES_BUFF[i].type = "";
          FILES_BUFF[i].fileposition = 0;
        }
      }

      void fillWithFiles(File32 &fout, File32 &fstatus, String search_pattern)
      {
          // NOTA:
          // ***************************************************
          // Esta función toma la ruta establecida por sdm.dir
          // del manejador de ficheros.
          // ***************************************************

          // Un fichero SOURCE_FILE_TO_MANAGE es del tipo
          //
          // 
          // PATH=/TAP/F/
          // ...
          // --------------------------------------------------------------------------------------------------
          //  ID    | Type [D / F] | Seek   | Filename
          // ---------------------------------------------------------------------------------------------------
          //  0     , F            , 288    , F-16 Combat Pilot (1991)(Digital Integration)[cr Matasoft][128K].tap;
          //
          // F --> File
          // D --> Directory


          // Declaramos el nombre del fichero
          char szName[512];
          const String separator="|";
          
          // Comprobamos que la ruta actual es un DIRECTORIO
          if (!sdm.dir.isDir()) 
          {
              // Devolvemos que hay error              
              return;
          }
          else
          {
              // Nos ponemos al inicio del directorio
              sdm.dir.rewindDirectory();
              
              // Posición del fichero encontrado en _files.lst
              int lpos = 1;
              // Contadores de ficheros y directorios
              int cdir = 0;
              int cfiles = 0;

              #ifdef DEBUGMODE
                logln("");
                log("Search pattern: " + search_pattern);
                logln("");
              #endif

              // Por si se queda abierto, lo cerramos primero
              // para poder abrirlo después en el WHILE
              if (sdm.file.isOpen())
              {sdm.file.close();}

              // Ahora lo recorremos
              while (sdm.file.openNext(&sdm.dir, O_RDONLY))
              {
                  esp_task_wdt_reset();

                  // Ok. Entonces es un fichero y cogemos su extensión
                  #ifdef DEBUGMODE
                    logln("");
                    log("ID: " + String(lpos));
                  #endif

                  size_t len = sdm.file.getName(szName,254);   

                  #ifdef DEBUGMODE
                    log(" - " + String(szName) + " len: " + String(len));                  
                  #endif

                  // Cuando la longitud es cero el nombre del fichero es erroneo
                  if (len != 0)
                  {
                      char* substr = strlwr(szName + (len - 4));    
                      uint32_t posf = sdm.dir.position();

                      String szNameStr = szName;

                      #ifdef DEBUGMODE
                        logln("");
                        logln("File found: " + szNameStr);
                        log(szNameStr);
                        logln("");
                      #endif

                      szNameStr.toLowerCase();
                      search_pattern.toLowerCase();

                      if (szNameStr.indexOf(search_pattern) != -1 || search_pattern=="")
                      {
                          #ifdef DEBUGMODE
                            logln("");
                            log( "[" + szNameStr + "] matched with: " + search_pattern);
                            logln("");
                          #endif
                          
                          // ¿No es un directorio?
                          if (!sdm.file.isDir())
                          {
                              // ¿No está oculto?
                              if (!sdm.file.isHidden())
                              {
                                  // Ok. Entonces es un fichero y cogemos su extensión                               
                                  // Si tiene una de las extensiones esperadas, se almacena
                                  if (strstr(substr, ".tap") || strstr(substr, ".tzx") || strstr(substr, ".tsx") || strstr(substr, ".cdt") || strstr(substr, ".wav") || strstr(substr, ".mp3") || strstr(substr, ".flac")) 
                                  {
                                      // ***************************
                                      // Cogemos la info del fichero
                                      // ***************************

                                      // numero del fichero
                                      fout.print(String(lpos));
                                      fout.print(separator);
                                      // tipo
                                      fout.print("F");
                                      fout.print(separator);
                                      // seek
                                      fout.print(String(posf));
                                      fout.print(separator);
                                      // nombre del fichero
                                      fout.print(szName);
                                      fout.println(separator);
                                      // Incrementamos el indice
                                      cfiles++;
                                      lpos++;
                                  }
                                  else
                                  {
                                    //No es fichero reconocido
                                  }
                              }
                          }
                          else
                          {
                              // Es un directorio
                              // Lo registramos si no está oculto
                              if(!sdm.file.isHidden())
                              {
                                  // ***************************
                                  // Cogemos la info del directorio
                                  // ***************************
                                  //

                                  // numero del fichero
                                  fout.print(String(lpos));
                                  fout.print(separator);
                                  // tipo
                                  fout.print("D");
                                  fout.print(separator);
                                  // seek
                                  fout.print(String(posf));
                                  fout.print(separator);
                                  // nombre del directorio a MAYUSCULAS
                                  // String szDirNameTmp = szName;
                                  // szDirNameTmp.toUpperCase();
                                  fout.print(szName);                      
                                  fout.println(separator);
                                  // Incrementamos el indice
                                  cdir++;
                                  lpos++;
                              }                   
                          }

                          // Registramos la ruta y el total de ficheros y directorios en _files.inf
                          fstatus.println("CFIL=" + String(cfiles));
                          fstatus.println("CDIR=" + String(cdir));
                          // Cerramos el fichero
                          //sdm.file.close();
                          //
                          FILE_TOTAL_FILES = cdir + cfiles;

                          // Devolvemos el total de ITEMS cargados 
                          writeString("statusFILE.txt=\"ITEMS " + String(FILE_TOTAL_FILES-1) +"\"");

                      }
                  }

                  sdm.file.close();
              }
          }
     }

      void registerFiles(String path, String filename, String filename_inf,String search_pattern)
      {
          // Registramos todos los ficheros encontrados y sus indices en un fichero "SOURCE_FILE_TO_MANAGE"
          String regFile = path + filename;
          String statusFile = path + filename_inf;

          char* file_ch = (char*)malloc(256 * sizeof(char));
          regFile.toCharArray(file_ch, 256);

          char* filest_ch = (char*)malloc(256 * sizeof(char));
          statusFile.toCharArray(filest_ch, 256);

          // Convertimos el path a char*
          char* path_ch = (char*)malloc(256 * sizeof(char));
          path.toCharArray(path_ch, 256);

          // Manejador del fichero _files.lst
          File32 f = sdm.file;
          // Manejador del fichero _files.inf
          File32 fstatus = sdm.file;


          if (sdm.createEmptyFile32(file_ch) && sdm.createEmptyFile32(filest_ch))
          {
              #ifdef DEBUGMODE
                logln("");
                logln("File created: " + String(file_ch));

                logln("");
                logln("File status created: " + String(filest_ch));            
              #endif

              // Abrimos el fichero
              f = sdm.openFile32(file_ch);
              fstatus = sdm.openFile32(filest_ch);

              //f.println("Esto es un prueba");
              // Ahora lo rellenamos con los ficheros incluidos en el path (que hemos convertido a char*)
              #ifdef DEBUGMODE
                logln("");
                logln("Path to search: " + String(path_ch));
              #endif
              
              // Registramos la ruta y los ficheros que lo contienen en el _files.lst
              fstatus.println("PATH=" + String(path_ch));

              // Rellenamos con los ficheros que contiene el directorio
              // y las estadísticas
              fillWithFiles(f,fstatus,search_pattern);
          }
          else
          {
            #ifdef DEBUGMODE
              logln("");
              logln("Error creating " + SOURCE_FILE_TO_MANAGE);
            #endif
          }

          // cerramos
          f.close();
          fstatus.close();

          // liberamos memoria
          free(path_ch);
          free(file_ch);
          free(filest_ch);
      }

      tFileLST getParametersFromLine(char* line)
      {   
          tFileLST lineData;
          int index = 0;
          char* ptr = NULL;
          const char* separator="|";

          ptr = strtok(line,separator);

          while(ptr != NULL)
          {
              // Capturamos los tokens
              switch(index)
              {
                case 0:
                lineData.ID = atoi(ptr);
                break;

                case 1:
                lineData.type = ptr[0];
                break;

                case 2:
                lineData.seek = atoi(ptr);
                break;

                case 3:
                lineData.fileName = String(ptr);
                lineData.fileType = (lineData.fileName).substring(lineData.fileName.length() - 3);
                break;
              }          
              ptr = strtok(NULL, separator);
              index++;
          }

          return lineData;
      }

      int getSeekFileLST(File32 &f, int posFileBrowser)
      {
        int seekFile = 0;
        char line[256];
        //
        int n;
        int i=0;        

        if (f.isOpen())
        {
            // read lines from the file
            while ((n = f.fgets(line, sizeof(line))) > 0) 
            {
                // eliminamos el terminador '\n'
                line[n-1] = 0;

                // Ahora leemos los datos de la linea
                tFileLST ld = getParametersFromLine(line);

                // si coincide con la posición buscada
                if (posFileBrowser == ld.ID);
                {
                    // Devolvemos el seek
                    return ld.seek;
                }
            }
        }

        return seekFile;
      }

      bool getIsDirFileLST(File32 &f, int pos)
      {
          bool isDir = false;

          return isDir;
      }

      tFileLST getNextLineInFileLST(File32& f)
      {
          char line[256];
          int n = 0;

          tFileLST ld;
          ld.fileName= "";
          ld.ID = -1;
          ld.seek = -1;
          ld.type = '\0';

          if (f.isOpen())
          {
              if((n = f.fgets(line, sizeof(line))) > 0)
              {
                  // eliminamos el terminador '\n'
                  line[n-1] = 0;
                  // Ahora leemos los datos de la linea
                  ld = getParametersFromLine(line); 
              }
          }
          else
          {
            #ifdef DEBUGMODE
              logln("Error reading file. " + SOURCE_FILE_TO_MANAGE +" is closed");
            #endif
          }

          return ld;
      }

      void putFilePtrInPosition(File32& f, int pos)
      {
          int n = 0;
          int l = 0;
          char line[256];

          tFileLST ld;
          ld.fileName= "";
          ld.ID = -1;
          ld.seek = -1;
          ld.type = 'F';

          // Comenzamos desde el principio del fichero
          f.rewind();
          #ifdef DEBUGMODE
            logln("ptr to put in position: " + String(pos));
          #endif

          // Ahora buscamos y movemos el puntero
          while(l < pos)
          {
              // Vamos saltando lineas hasta la posición esperada
              n = f.fgets(line,sizeof(line));
              l++;
          }

          #ifdef DEBUGMODE
            logln("Position is " + String(l));
          #endif
      }

      void showFileLST(File32 &f)
      {
          int n = 0;
          int total = 0;
          char line[256];
          
          if (f.isOpen())
          {
              // read lines from the file
              while ((n = f.fgets(line, sizeof(line))) > 0) 
              {
                  // eliminamos el terminador '\n'
                  line[n-1] = 0;

                  // Ahora leemos los datos de la linea
                  tFileLST ld = getParametersFromLine(line);

                  #ifdef DEBUGMODE
                    log("Data: [");
                    log(String(ld.ID));
                    log("] - ");
                    log(String(ld.seek));
                    log(" - ");
                    log(String(ld.type));
                    log(" - ");
                    log(ld.fileName);
                    logln("");
                  #endif

                  // Guardamos el ultimo
                  total = ld.ID;
              }

              FILE_TOTAL_FILES = total;

              #ifdef DEBUGMODE
                logln("Total files: " + String(FILE_TOTAL_FILES-1));
              #endif
          }        
      }

      inline bool exists_file (String name) 
      {
          // Comprueba si existe el fichero en la ruta
          struct stat buffer;   
          return (stat (name.c_str(), &buffer) == 0); 
      }

      bool exist_LST_file(String path, String filename)
      {
          File32 f;
          String fFileList = path + filename;

          if(f.open(fFileList.c_str(), O_RDONLY))
          {
            f.close();
            logln("File.lst existe!!!!!");
            return true;
          }
          else
          {
            logln("File.lst NOOO existe!!!!!");
            return false;
          }
      }

      // void registerFileLST(String path, bool forze_rescan, String filename, String filename_inf, String search_pattern)
      // {
      //     // registramos todos los ficheros
      //     // Si el _files.lst ya existe no se vuelve a crear, a no ser que sea forzado el rescan.
      //     File32 f;
      //     String fFileList = path + filename;

      //     #ifdef DEBUGMODE
      //       logln("");
      //       log("Path to register in LST file: " + fFileList);
      //       logln("");
      //     #endif

      //     if(!f.open(fFileList.c_str(), O_RDONLY))
      //     {
      //         #ifdef DEBUGMODE
      //           logln("Registering files in: " + fFileList);
      //         #endif
      //         registerFiles(path,filename,filename_inf,search_pattern);
      //     }
      //     else
      //     {
      //         f.close();

      //         if(forze_rescan)
      //         {
      //             #ifdef DEBUGMODE
      //               logln("Registering files FORZED");
      //             #endif
      //             registerFiles(path,filename,filename_inf,search_pattern);
      //         }
      //     }
      // }

      void manageFileLST(File32& fFileLST, String path, String sourceFile)
      {
        String fFileList = path + sourceFile;

        // Abrimos ahora el _files.lst, para manejarlo
        if (!LST_FILE_IS_OPEN)
        {
            // Abrimos el fichero
            fFileLST.open(fFileList.c_str(), O_RDONLY);
            fFileLST.rewind();

            // #ifdef DEBUGMODE
              logln("Opening again - rewind");
            // #endif

            IN_THE_SAME_DIR = true;
            // Mostramos el contenido y obtenemos el total de ficheros
            //
            FILE_TOTAL_FILES = 1;
            char line[256];
            int n=0;
            while (n = fFileLST.fgets(line,sizeof(line)) > 0)
            {
                FILE_TOTAL_FILES++;
            }

            LST_FILE_IS_OPEN = true;
        }       
      }

      void getFilesFromSD(bool forze_rescan, String output_file, String output_file_inf, String search_pattern="")
      {
          #ifdef DEBUGMOCE
            logAlert("Getting files from SD");
          #endif

          // Cadena para un filePath
          char szName[255];
          int j = 0;          


          tFileLST fl;

          clearFileBuffer();

          #ifdef DEBUGMODE
            logln("");
            logln("");
            log("--------------------------------------");
            logln("");
            log("Searching files in - " + FILE_LAST_DIR);
          #endif

          if (sdm.dir.isOpen())
          {
              sdm.dir.close();
          }

          if (!sdm.dir.open(FILE_LAST_DIR.c_str())) 
          {
              FILE_DIR_OPEN_FAILED = true;
              // manageFile();
          }
          else
          {
              // El directorio se ha abierto sin problemas
              FILE_DIR_OPEN_FAILED = false;

              String recDirTmp = FILE_LAST_DIR;
              recDirTmp.toUpperCase();
              if (recDirTmp == "/REC/")
              {
                forze_rescan=true;
              }
              else if (recDirTmp == "/FAV/")
              {
                forze_rescan=true;
              }
              else if (recDirTmp == "/WAV/")
              {
                forze_rescan=true;
              }


              // Si el fichero manejador esta abierto no se hacen mas comprobaciones.
              if (!LST_FILE_IS_OPEN)
              {
                  // Si no existe el historico de los ficheros se genera un _file.lst
                  if (forze_rescan || !exist_LST_file(FILE_LAST_DIR,output_file))
                  {
                      logln("Registrando ficheros");
                      registerFiles(FILE_LAST_DIR, output_file, output_file_inf,search_pattern);
                  }
              }
              else
              {
                logln("Manejando los ficheros en buffer");
              }

              // Usamos el fichero que contiene el mapa del directorio actual, _files.lst
              manageFileLST(fFileLST, FILE_LAST_DIR, SOURCE_FILE_TO_MANAGE);
    
              // AÑADIMOS AL INICIO EL PARENT DIR
              FILES_BUFF[0].isDir = true;
              FILES_BUFF[0].type = "PAR";
              FILES_BUFF[0].path = String(FILE_PREVIOUS_DIR);
              FILES_BUFF[0].fileposition = sdm.file.position();

              #ifdef DEBUGMODE
                logln("Showing files from: " + String(FILE_PTR_POS));
              #endif

              // Posicionamos el puntero
              if (FILE_PTR_POS!=0)
              {
                  putFilePtrInPosition(fFileLST,FILE_PTR_POS-1);
              }
              else
              {
                  putFilePtrInPosition(fFileLST,0);
              }

              // Ahora añadimos los ficheros al buffer de presentación (14 lineas)
              int idptr = 1;
              while(idptr < MAX_FILES_TO_LOAD)
              {
                  fl = getNextLineInFileLST(fFileLST);

                  #ifdef DEBUGMODE
                    logln("");
                    log(fl.fileName + " - ");
                    log(fl.fileType + " - ");
                    log(String(fl.ID) + " - ");
                    log(String(fl.seek) + " - ");
                    log(String(fl.type));
                  #endif

                  // Si es un dato de fichero
                  if (fl.ID != -1)
                  {
                      // Es correcto. Lo mostramos
                      FILES_BUFF[idptr].fileposition = fl.seek;
                      FILES_BUFF[idptr].path = fl.fileName;

                      if (fl.type=='D')
                      {
                        FILES_BUFF[idptr].type = "DIR";
                        FILES_BUFF[idptr].isDir = true;
                      }
                      else
                      {
                        FILES_BUFF[idptr].isDir = false;
                        FILES_BUFF[idptr].type = (fl.fileType);
                        FILES_BUFF[idptr].type.toUpperCase();
                      }                   
                  }
                  else
                  {
                      // Salimos
                      logln("EOF");
                      return;
                  }

                  idptr++;
              }

              writeString("statusFILE.txt=\"ITEMS " + String(FILE_TOTAL_FILES-1) +"\"");
          }

          // Cerramos todos los ficheros (añadido el 25/07/2023)
          sdm.dir.close();
          // fFileLST.close();
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

      void printFileRowsBlock(String &serialTxt, int row, int color, String szName)
      {
            // Ponemos los inicios de mensaje 0xFF 0xFF 0xFF
            char t = 255;
            switch(row)
            {
              case 0:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += "file0.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file0.pco=" + String(color);
                  break;
      
              case 1:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file1.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file1.pco=" + String(color);
                  break;
      
              case 2:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file2.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file2.pco=" + String(color);
                  break;
      
              case 3:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file3.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file3.pco=" + String(color);
                  break;
      
              case 4:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file4.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file4.pco=" + String(color);
                  break;
      
              case 5:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file5.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file5.pco=" + String(color);
                  break;
      
              case 6:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file6.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file6.pco=" + String(color);
                  break;
      
              case 7:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file7.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file7.pco=" + String(color);
                  break;
      
              case 8:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file8.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file8.pco=" + String(color);
                  break;
      
              case 9:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file9.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file9.pco=" + String(color);
                  break;
      
              case 10:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file10.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file10.pco=" + String(color);
                  break;
      
              case 11:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file11.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file11.pco=" + String(color);
                  break;
      
              case 12:
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file12.txt=\"" + String(szName) + "\"";
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt += t;
                  serialTxt +="file12.pco=" + String(color);
                  break;
      
            }  

            // Ponemos los finales de mensaje 0xFF 0xFF 0xFF
            serialTxt += t;
            serialTxt += t;
            serialTxt += t;
      }
      
      String getPreviousDirFromPath(String path)
      {
          String strTmp = INITFILEPATH;
          //strncpy(strTmp,&INITFILEPATH[0],2);

          #ifdef DEBUGMODE
            logln("");
            logln("Current dir: " + path);
          #endif

          int lpath = path.length();
          
          for (int n=lpath-2;n>1;n--)
          {
              char chrRead = path[n];
              
              if (chrRead == '/')
              {
                  //Copiamos ahora el trozo de cadena restante
                  strTmp = path.substring(0,n+1);
                  break;
              }
          }

          #ifdef DEBUGMODE
            logln("");
            logln("Next dir: " + strTmp);
          #endif

          return strTmp;
      }
      
      // void clearFilesInScreen()
      // {
      //   String szName = "";
      //   int color = 65535;
      //   int pos_in_HMI_file = 0;
      
      //   #ifdef DEBUGMOCE
      //     logAlert("Cleaning file browser");
      //   #endif

      //   String mens = "";

      //   for (int i=0;i<=TOTAL_FILES_IN_BROWSER_PAGE;i++)
      //   {
      //       printFileRowsBlock(mens, pos_in_HMI_file, color, szName);
      //       //delay(5);
      //       // printFileRows(pos_in_HMI_file, color, szName);
      //       pos_in_HMI_file++;
      //   }

      //   // Ahora vamos a redibujar dos veces
      //   writeStringBlock(mens);
      //   // delay(5);
      //   // writeStringBlock(mens);
      // }

      void showInformationAboutFiles()
      {
          // Limpiamos el path, para evitar cargar un fichero no seleccionado.
          writeString("path.txt=\"\"");  
          // Indicamos el path actual
          writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");
          // Actualizamos el total del ficheros leidos anteriormente.
          writeString("statusFILE.txt=\"ITEMS  " + String(FILE_TOTAL_FILES-1) +"\"");         
          
          // Obtenemos la pagina mostrada del listado de ficheros
          int totalPages = ((FILE_TOTAL_FILES-1) / TOTAL_FILES_IN_BROWSER_PAGE);
          
          if ((FILE_TOTAL_FILES-1) % TOTAL_FILES_IN_BROWSER_PAGE != 0)
          { 
              totalPages+=1;
          }

          int currentPage = (FILE_PTR_POS / TOTAL_FILES_IN_BROWSER_PAGE) + 1;

          // Actualizamos el total del ficheros leidos anteriormente.
          //writeString("tPage.txt=\"" + String(totalPages) +"\"");
          writeString("cPage.txt=\"" + String(currentPage) +" - " + String(totalPages) + "\"");
      }
           
      void putFilesInScreen()
      {
      
        //Antes de empezar, limpiamos el buffer
        //para evitar que se colapse
        #ifdef DEBUGMOCE
          logAlert("Showing files in browser");
        #endif
        
        int pos_in_HMI_file = 0;
        String szName = "";
        String type="";
      
        int color = 65535; //60868
        
        // Esto es solo para el parent dir, para que siempre salga arriba
        szName = FILES_BUFF[0].path;
        color = 2016;  // Verde
        szName = String(".. ") + szName;
        
        writeString("");
        writeString("prevDir.txt=\"" + String(szName) + "\"");
        writeString("");
        writeString("prevDir.pco=" + String(color));
                  
        String mens = "";
        for (int i=1;i < MAX_FILES_TO_LOAD;i++)
        {
              szName = FILES_BUFF[i].path;
              type = FILES_BUFF[i].type;
              
              #ifdef DEBUGMODE
                logln("File type: " + type);
              #endif

              // Convertimos a mayusculas el tipo
              type.toUpperCase();

              // Lo trasladamos a la pantalla
              if (type == "DIR")
              {
                  //Directorio
                  color = 60868;  // Amarillo
                  szName = String("<DIR>  ") + szName;
                  szName.toUpperCase();
              }     
              else if (type == "TAP" || type == "TZX" || type == "TSX" || type == "CDT" || type == "WAV" || type == "MP3")
              {
                  //Fichero
                  if (_sdf.exists("/fav/" + szName))
                  {
                    color = 34815;   // Cyan - Indica que esta en favoritos
                  }
                  else
                  {
                    color = 65535;   // Blanco
                  }
                  
              }
              else
              {
                  color = 44405;  // gris apagado
              }
      
              printFileRowsBlock(mens,i-1, color, szName);
              // printFileRows(i-1, color, szName);
              // Delay necesario para un correcto listado en pantalla.
              // delay(2);

        }

        // Ahora vamos a redibujar dos veces
        writeStringBlock(mens);
        // delay(5);
        // writeStringBlock(mens);

        showInformationAboutFiles();        
      
      }

      void putInHome()
      {
          //FILE_BROWSER_SEARCHING = false;  
          FILE_LAST_DIR_LAST = INITFILEPATH;
          FILE_LAST_DIR = INITFILEPATH;
          FILE_PREVIOUS_DIR = INITFILEPATH;
          // Open root directory
          FILE_PTR_POS = 1;
          IN_THE_SAME_DIR = false;
          // clearFilesInScreen(); // 02/12/2024
          getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
          putFilesInScreen();
      }
      
      void findTheTextInFiles()
      {
          // Hacemos una busqueda
          SOURCE_FILE_TO_MANAGE = "_fsearch.lst";
          SOURCE_FILE_INF_TO_MANAGE = "_fsearch.inf";

          LST_FILE_IS_OPEN = false;
          if (fFileLST.isOpen())
          {
            fFileLST.close();
          }

          FILE_PTR_POS = 1;
          writeString("statusFILE.txt=\"SEARCHING\""); 
          getFilesFromSD(true,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE,FILE_TXT_TO_SEARCH);
          //putFilesInScreen();
          refreshFiles(); //07/11/2024          
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
          #ifdef DEBUGMOCE
            logAlert("Refreshing file browser");
          #endif
          
          // clearFilesInScreen(); // 02/12/2024
          putFilesInScreen();          
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
          writeString("tape.progressBlock.val=0");

          TOTAL_BLOCKS = 0;
          BLOCK_SELECTED = 0;
          BYTES_LOADED = 0;     
          
      }
 
      public:



      void reloadCustomDir(String path)
      {
        FILE_LAST_DIR=path;
        getFilesFromSD(true,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
      }

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

      void activateWifi(bool enable)
      {
          if(enable)
          {
            if(WIFI_ENABLE && !REC)
            {
              WiFi.mode(WIFI_STA);
              WiFi.begin(ssid, password);
              btStart();
              //LAST_MESSAGE = "Connecting wifi radio";
              //delay(125);
              //LAST_MESSAGE = "Wifi enabled";
              //
              writeString("tape.wifiInd.pic=38");
            }
          }
          else
          {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            btStop();          
            //LAST_MESSAGE = "Disconnecting wifi radio";
            //delay(2000);
            //LAST_MESSAGE = "Wifi disabled";
            //
            writeString("tape.wifiInd.pic=37");
          }
      }

      void reloadDir()
      {
          // Recarga el directorio
          FILE_PTR_POS = 1;

          LST_FILE_IS_OPEN = false;
          if (fFileLST.isOpen())
          {
            fFileLST.close();
          }

          getFilesFromSD(true,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);      
          refreshFiles();        
      }

      void firstLoadingFilesFB()
      {
          // Carga por primera vez ficheros en el filebrowser
          getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
          putFilesInScreen();        
      }

      void copyFile(File32 &fSource, File32 &fTarget)
      {
          size_t n;  
          uint8_t buf[64];

          while ((n = fSource.read(buf, sizeof(buf))) > 0) 
          {
            fTarget.write(buf, n);
          }        
      }

      void verifyCommand(String strCmd) 
      {
        
        // Reiniciamos el comando
        LAST_COMMAND = "";

        // Selección de bloque desde keypad - pantalla
        if(strCmd.indexOf("RSET") != -1)
        {
          delay(2000);
          ESP.restart();
        }
        else if (strCmd.indexOf("DSD") != -1)
        {
            DISABLE_SD = true;
        }
        else if (strCmd.indexOf("BKX=") != -1) 
        {
            // Con este procedimiento capturamos el bloque seleccionado
            // desde la pantalla.
            if (TAPESTATE == 10 || TAPESTATE == 3 || TAPESTATE == 2)
            {
              uint8_t buff[8];

              strCmd.getBytes(buff, 7);
              long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
              String num = String(val);
              BLOCK_SELECTED = num.toInt() + 1;
              // updateInformationMainPage(true);
              // Esto lo hacemos para poder actualizar la info del bloque
              UPDATE = true;
            }

        }
        else if (strCmd.indexOf("REL=") != -1)
        {
            // Recarga la pantalla de ficheros
            firstLoadingFilesFB();
        }
        else if (strCmd.indexOf("PAG=") != -1) 
        {
            // Con este procedimiento obtenemos la pàgina del filebrowser
            // que se desea visualizar
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
            int pageSelected = num.toInt();

            // Calculamos el total de páginas
            int totalPages = ((FILE_TOTAL_FILES-1) / TOTAL_FILES_IN_BROWSER_PAGE);
            if ((FILE_TOTAL_FILES-1) % TOTAL_FILES_IN_BROWSER_PAGE != 0)
            {
                totalPages+=1;
            }            

            #ifdef DEBUGMODE
              logln("");
              logln("Page selected: " + String(pageSelected+1) + "/" + String(totalPages));
            #endif

            // Controlamos que no nos salgamos del total de páginas
            if ((pageSelected+1) <= totalPages)
            {
                // Cogemos el primer item y refrescamos
                FILE_PTR_POS = (pageSelected * TOTAL_FILES_IN_BROWSER_PAGE) + 1;
            }
            else
            {
                // Posicionamos entonces en la ultima página
                pageSelected = totalPages-1;
                // Cogemos el primer item y refrescamos
                FILE_PTR_POS = (pageSelected * TOTAL_FILES_IN_BROWSER_PAGE) + 1;
            }
            
            // Actualizamos la lista con la posición nueva del puntero
            getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
            // Refrescamos el listado de ficheros visualizado
            refreshFiles();            
        }      
        else if (strCmd.indexOf("INFB") != -1) 
        {
            // Con este comando nos indica la pantalla que 
            // está en modo FILEBROWSER
            //proccesingEject();            
            FILE_BROWSER_OPEN = true;
            #ifdef DEBUGMODE
              logAlert("INFB output: File browser open");
            #endif

        }
        else if (strCmd.indexOf("SHR") != -1) 
        {
            // Con este comando nos indica la pantalla que 
            // está en modo searching
            //FILE_BROWSER_SEARCHING = true;
            findTheTextInFiles();      
        }
        else if (strCmd.indexOf("OUTFB") != -1) 
        {

            // Con este comando nos indica la pantalla que 
            // está SALE del modo FILEBROWSER
            FILE_BROWSER_OPEN = false;
            #ifdef DEBUGMODE
              logAlert("OUTFB output: File browser closed");
            #endif
        }
        else if (strCmd.indexOf("GFIL") != -1) 
        {
            // Con este comando nos indica la pantalla que quiere
            // le devolvamos ficheros en la posición actual del puntero
      
            #ifdef DEBUGMODE
              logAlert("GFIL output: Getting files");
            #endif

            // Hemos quitado esto para que se ponga en FALSE cuando haga ejecting y no antes
            //FILE_PREPARED = false;
            FILE_SELECTED = false;

            resetIndicators();

            if (FILE_LAST_DIR_LAST != FILE_LAST_DIR)
            {

                if (sdm.dir.isOpen())
                {
                    sdm.dir.close();             
                }              

                getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE,"");
                //refreshFiles(); //07/11/2024 - Ojo!
            }

            if (!FILE_DIR_OPEN_FAILED)
            {
                // clearFilesInScreen(); // 02/12/2024
                putFilesInScreen();
                FILE_STATUS = 1;
                // El GFIL desconecta el filtro de busqueda
                //FILE_BROWSER_SEARCHING = false;                   
            }            
        }
        else if (strCmd.indexOf("RFSH") != -1) 
        {
            reloadDir();
        }
        else if (strCmd.indexOf("FINI") != -1) 
        {
            // Posicionamos entonces en la primera página
            // Cogemos el primer item y refrescamos
            FILE_PTR_POS = 1;
            // Actualizamos la lista con la posición nueva del puntero
            getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
            // Refrescamos el listado de ficheros visualizado
            refreshFiles();  
            delay(125);
            showInformationAboutFiles();                     
        }
        else if (strCmd.indexOf("FEND") != -1) 
        {
            // Posicionamos entonces en la ultima página
            int totalPages = ((FILE_TOTAL_FILES-1) / TOTAL_FILES_IN_BROWSER_PAGE);
            // Cogemos el primer item y refrescamos
            FILE_PTR_POS = (totalPages * TOTAL_FILES_IN_BROWSER_PAGE) + 1;
            // Actualizamos la lista con la posición nueva del puntero
            getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
            // Refrescamos el listado de ficheros visualizado
            refreshFiles(); 
            delay(125);
            showInformationAboutFiles();                       
        }        
        else if (strCmd.indexOf("FPUP") != -1) 
        {
            // Con este comando nos indica la pantalla que quiere
            // le devolvamos ficheros en la posición actual del puntero
            FILE_PTR_POS = FILE_PTR_POS - TOTAL_FILES_IN_BROWSER_PAGE;
      
            if (FILE_PTR_POS < 0)
            {
                FILE_PTR_POS = 1;
            }

            getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
            refreshFiles();
            delay(125);
            showInformationAboutFiles();            
        }
        else if (strCmd.indexOf("FPDOWN") != -1) 
        {
            // Con este comando nos indica la pantalla que quiere
            // le devolvamos ficheros en la posición actual del puntero
            FILE_PTR_POS = FILE_PTR_POS + TOTAL_FILES_IN_BROWSER_PAGE;
  
            #ifdef DEBUGMODE
              logln("");
              logln("files in page: " + String(FILE_TOTAL_FILES-1));
            #endif

            if (FILE_PTR_POS > (FILE_TOTAL_FILES-1))
            {
                // Hacemos esto para permanecer en la misma pagina
                FILE_PTR_POS -= TOTAL_FILES_IN_BROWSER_PAGE;
            }

            getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
            refreshFiles();
            delay(125);
            showInformationAboutFiles();
        }
        else if (strCmd.indexOf("FPHOME") != -1) 
        {
            // Con este comando nos indica la pantalla que quiere
            // le devolvamos ficheros en la posición actual del puntero
            SOURCE_FILE_TO_MANAGE = "_files.lst";
            SOURCE_FILE_INF_TO_MANAGE = "_files.inf"; 

            LST_FILE_IS_OPEN = false;
            if (fFileLST.isOpen())
            {
              fFileLST.close();
            }

            putInHome();
         
            getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
            refreshFiles(); 
        }        
        else if (strCmd.indexOf("FAV=") != -1) 
        {
            
            #ifdef DEBUGMODE
              logAlert("FAV Command");
            #endif                  

            // Con este comando añadimos el fichero seleccionado a la carpeta favoritos si no existe
            // Con este comando
            // devolvamos el fichero que se ha seleccionado en la pantalla
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
      
            FILE_IDX_SELECTED = num.toInt();
            FILE_SELECTED = false;
      
            //Extraemos el fichero
            String fileName  = FILES_BUFF[FILE_IDX_SELECTED+1].path;
            String fToFav = FILE_LAST_DIR + fileName;                         
      
            #ifdef DEBUGMODE
              logAlert("Filename: " + fileName);
              logAlert("Full path: " + fToFav);
            #endif

            // Cambiamos el estado de fichero seleccionado
            if (fToFav != "")
            {
                #ifdef DEBUGMODE
                  logAlert("File was selected to move to favorite dir");
                #endif

                // Ahora lo cargamos en favorito.
                String pSource = fToFav;
                String pTarget = "/fav/" + fileName;

                #ifdef DEBUGMODE
                  logln("");
                  log("Source file: " + pSource);
                  logln("");
                  log("Target file: " + pTarget);
                  logln("");
                #endif
                
                //Esto lo hacemos para ver si el directorio existe
                String favDir = "/FAV";

                if (_sdf.chdir(favDir))
                {
                    #ifdef DEBUGMODE
                      logAlert("FAV ok");
                    #endif

                    if (!_sdf.exists(pTarget))
                    {
                        // Si el fichero no existe en favoritos, lo copio
                        File32 fSource;
                        File32 fTarget;

                        char pSrc[255];
                        char pTgt[255];

                        // Abrimos los ficheros fuente y destino
                        bool fSourceOk = fSource.open(strcpy(pSrc,pSource.c_str()),O_READ);
                        bool fTargetOk = fTarget.open(strcpy(pTgt,pTarget.c_str()),O_WRITE | O_CREAT | O_TRUNC);

                        #ifdef DEBUGMODE
                          logln("");
                          log("* Source file: " + String(pSrc));
                          logln("");
                          log("* Target file: " + String(pTgt));
                          logln("");
                        #endif

                        if (!fSourceOk)
                        {
                            logAlert("Error opening source file");
                        }
                        
                        if (!fTargetOk)
                        {
                            logAlert("Error opening target file");
                        }

                        if (fSourceOk && fTargetOk)
                        {

                          logAlert("Starting copy ...");

                          copyFile(fSource,fTarget);
                          fSource.close();
                          fTarget.close();

                          logAlert("Copy finish.");

                        }
                        else
                        {
                          #ifdef DEBUGMODE
                            logln("");
                            log("Error! Copy failed");
                          #endif
                        }                  
                    }
                    else
                    {
                        #ifdef DEBUGMODE
                          logln("");
                          log("File already in FAV.");
                        #endif
                    }                  
                }
                else
                {
                  #ifdef DEBUGMODE
                    logln("");
                    log("Error! directory /FAV not created.");
                  #endif
                }
            }     
        }              
        else if (strCmd.indexOf("BBOPEN") != -1)
        {
          // Block browser abierto
          BB_OPEN = true;
          BB_PTR_ITEM = 0;
        }
        else if (strCmd.indexOf("BBCL=") != -1)
        {
          // Block browser cerrado con ID seleccionado o -1 para ninguno
          // Con este procedimiento obtenemos la pàgina del filebrowser
          // que se desea visualizar
          int posEq = strCmd.indexOf("=");
          String sbstr = strCmd.substring(posEq+1);

          // Obtenemos el ID seleccionado
          int blsel = sbstr.toInt();
         
          logln("Bloque seleccionado: " + String(blsel));

          if (blsel >= 0 && blsel <= TOTAL_BLOCKS)
          {
            BLOCK_SELECTED = blsel;
            UPDATE_HMI = true;
          }         
          BB_OPEN = false;
        }        
        else if (strCmd.indexOf("BDOWN") != -1)
        {
          // Pagina arriba block browser
          BB_PTR_ITEM += MAX_BLOCKS_IN_BROWSER;

          if (BB_PTR_ITEM > TOTAL_BLOCKS - 1)
          {
            BB_PTR_ITEM -= MAX_BLOCKS_IN_BROWSER;
          }

          BB_UPDATE = true;

        }
        else if (strCmd.indexOf("BUP") != -1)
        {
          // Pagina arriba block browser
          BB_PTR_ITEM -= MAX_BLOCKS_IN_BROWSER;

          if (BB_PTR_ITEM < 0)
          {
            BB_PTR_ITEM = 0;
          }

          BB_UPDATE = true;

        }
        else if (strCmd.indexOf("CHD=") != -1) 
        {
            // Con este comando capturamos el directorio a cambiar
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));

            //String num = String(val);
            FILE_IDX_SELECTED = static_cast<int> (val);
      
            //Cogemos el directorio

            if (FILE_IDX_SELECTED > sizeof(FILES_BUFF))
            {
              // Regeneramos el directorio de manera automática.
              writeString("currentDir.txt=\">> Error in file system. Reloading <<\"");
              reloadDir();
              return;
            }
            
            FILE_DIR_TO_CHANGE = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED+1].path + "/";
            
            #ifdef DEBUGMODE
              logln("Dir to change " + FILE_DIR_TO_CHANGE);
            #endif
            
            IN_THE_SAME_DIR = false;
      
            // Reserva dinamica de memoria
            String dir_ch = FILE_DIR_TO_CHANGE;

            //
            FILE_PREVIOUS_DIR = FILE_LAST_DIR;
            FILE_LAST_DIR = dir_ch;
            FILE_LAST_DIR_LAST = FILE_LAST_DIR;
     
            FILE_PTR_POS = 1;
            // clearFilesInScreen(); // 02/12/2024
            writeString("statusFILE.txt=\"READING\"");  

            // if(!exist_LST_file(FILE_LAST_DIR, SOURCE_FILE_TO_MANAGE))
            // {
            //     logln("Ruta: " + FILE_LAST_DIR);
            //     registerFiles(FILE_LAST_DIR,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE,"");
            // }
            LST_FILE_IS_OPEN = false;
            if (fFileLST.isOpen())
            {
              fFileLST.close();
            }

            getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
            
            if (!FILE_DIR_OPEN_FAILED)
            {
                //putFilesInScreen();
                putFilesInScreen();
            }
            else
            {
              #ifdef DEBUGMODE
                logln("Error to open directory");
              #endif
            }

        }
        else if (strCmd.indexOf("PAR=") != -1) 
        {
            // Con este comando capturamos el directorio padre
            String oldDir = FILE_PREVIOUS_DIR;
            
            LST_FILE_IS_OPEN = false;
            if (fFileLST.isOpen())
            {
              fFileLST.close();
            }

            // Forzamos a leer el repositorio de ficheros del directorio.
            SOURCE_FILE_TO_MANAGE = "_files.lst"; 
            SOURCE_FILE_INF_TO_MANAGE = "_files.inf"; 

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
            FILE_PTR_POS = 1;    
            getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
            refreshFiles();
            delay(125);
            showInformationAboutFiles();            
            
            // 
            if (!FILE_DIR_OPEN_FAILED)
            {
                putFilesInScreen();
            }
            else
            {
                // Error
                writeString("currentDir.txt=\">> Error opening directory. <<\"");
                delay(1500);
                writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");             
            }

            refreshFiles();
      
        }
        else if (strCmd.indexOf("TRS=") != -1) 
        {
            // Con este comando
            // Borramos el fichero que se ha seleccionado en la pantalla
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);

            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
      
            FILE_IDX_SELECTED = num.toInt();
            FILE_SELECTED_DELETE = false;
      
            FILE_TO_DELETE = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED+1].path;      
            FILE_SELECTED_DELETE = true; 


            if (FILE_SELECTED_DELETE)
            {
              // Lo Borramos
                File32 mf = _sdf.open(FILE_TO_DELETE,O_WRONLY);

                if (!mf.isWritable())
                {
                  writeString("currentDir.txt=\">> Error. File not writeable <<\"");
                  delay(1500);
                  writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");                  
                  mf.close();
                  FILE_SELECTED_DELETE = false;
                }
                else if (mf.isReadOnly())
                { 
                  writeString("currentDir.txt=\">> Error. Readonly file <<\"");
                  delay(1500);
                  writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");                  
                  mf.close();
                  FILE_SELECTED_DELETE = false;
                }
                else
                {
                    mf.close();

                    if (!_sdf.remove(FILE_TO_DELETE))
                    {
                      #ifdef DEBUGMODE
                        logln("Error to remove file. " + FILE_TO_DELETE);
                      #endif
                      
                      writeString("currentDir.txt=\">> Error. File not removed <<\"");
                      delay(1500);
                      writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");    
                      FILE_SELECTED_DELETE = false;              
                    }
                    else
                    {
                      FILE_SELECTED_DELETE = false;
                      logln("File remove. " + FILE_TO_DELETE);
                      
                      // Tras borrar hacemos un rescan
                      getFilesFromSD(true,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);      
                      delay(125);
                      refreshFiles();
                      delay(125);
                    }                  
                }
            }
            else
            {
              writeString("currentDir.txt=\">> First, select a file. <<\"");
            }
        }      
        // Load file - Carga en el TAPE el fichero seleccionado en pantalla
        else if (strCmd.indexOf("LFI=") != -1) 
        {
            // Con este comando
            // devolvamos el fichero que se ha seleccionado en la pantalla
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
      
            FILE_IDX_SELECTED = num.toInt();
            FILE_SELECTED = false;
      
            // Path completo del fichero
            PATH_FILE_TO_LOAD = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED+1].path;  
            // Fichero sin path
            FILE_LOAD = FILES_BUFF[FILE_IDX_SELECTED+1].path;                
      
            // Cambiamos el estado de fichero seleccionado
            if (PATH_FILE_TO_LOAD != "")
            {
                #ifdef DEBUGMODE
                  logAlert("File was selected");
                #endif

                FILE_SELECTED = true;
            }

      
            #ifdef DEBUGMODE
              logln("");
              logln("File to load = ");
              log(PATH_FILE_TO_LOAD);
              logln("");
            #endif

            FILE_STATUS = 0;
      
            //FILE_BROWSER_OPEN = false;
      
        }   
        // Configuración de frecuencias de muestreo
        else if (strCmd.indexOf("FRQ1") != -1)
        {
          DfreqCPU = 3250000.0;
        }
        else if (strCmd.indexOf("FRQ2") != -1)
        {
          DfreqCPU = 3300000.0;
        }
        else if (strCmd.indexOf("FRQ3") != -1)
        {
          DfreqCPU = 3330000.0;
        }
        else if (strCmd.indexOf("FRQ4") != -1)
        {
          DfreqCPU = 3350000.0;
        }
        else if (strCmd.indexOf("FRQ5") != -1)
        {
          DfreqCPU = 3400000.0;
        }
        else if (strCmd.indexOf("FRQ6") != -1)
        {
          DfreqCPU = 3430000.0;
        }
        else if (strCmd.indexOf("FRQ7") != -1)
        {
          DfreqCPU = 3450000.0;
        }
        else if (strCmd.indexOf("FRQ8") != -1)
        {
          DfreqCPU = 3500000.0;
        }
        // Indica que la pantalla está activa
        else if (strCmd.indexOf("LCDON") != -1) 
        {
            LCD_ON = true;
        }
        // Control de TAPE
        else if (strCmd.indexOf("FFWD") != -1) 
        {
            logln("FFWD pressed");
            FFWIND = true;
            RWIND = false;
        }
        else if (strCmd.indexOf("RWD") != -1) 
        {
            logln("RWD pressed");
            FFWIND = false;
            RWIND = true;
        }
        else if (strCmd.indexOf("PLAY") != -1) 
        {

          #ifdef DEBUGMODE
            logAlert("PLAY pressed.");
          #endif

          PLAY = true;
          PAUSE = false;
          STOP = false;
          REC = false;
          EJECT = false;
          ABORT = false;

          BTN_PLAY_PRESSED = true;
        }   
        else if (strCmd.indexOf("REC") != -1) 
        {

          #ifdef DEBUGMODE
            logAlert("REC pressed.");
          #endif

          PLAY = false;
          PAUSE = false;
          STOP = false;
          REC = true;
          ABORT = false;
          EJECT = false;
        }
        else if (strCmd.indexOf("PAUSE") != -1) 
        {

          #ifdef DEBUGMODE
            logAlert("PAUSE pressed.");
          #endif

          PLAY = false;
          PAUSE = true;
          STOP = false;
          REC = false;
          ABORT = true;
          EJECT = false;
          //updateInformationMainPage();
        }    
        else if (strCmd.indexOf("STOP") != -1) 
        {

          #ifdef DEBUGMODE
            logAlert("STOP pressed.");
          #endif

          PLAY = false;
          PAUSE = false;
          STOP = true;
          REC = false;
          ABORT = true;
          EJECT = false;

          BLOCK_SELECTED = 0;
          BYTES_LOADED = 0;
        }     
        else if (strCmd.indexOf("EJECT") != -1) 
        {
          #ifdef DEBUGMODE
            logAlert("EJECT pressed.");
          #endif

          PLAY = false;
          PAUSE = false;
          STOP = true;
          REC = false;
          ABORT = false;
          EJECT = true;

          // Esto lo hacemos así porque el EJECT lanza un comando en paralelo
          // al control del tape (tapeControl)
          // no quitar!!
          if (PROGRAM_NAME != "" || TOTAL_BLOCKS !=0)
          {
              LAST_MESSAGE = "Ejecting cassette.";
              writeString("g0.txt=\"" + LAST_MESSAGE + "\"");
              delay(500);
              clearInformationFile();
              delay(250);
          }

          FILE_BROWSER_OPEN = true;
          //
          // Entramos en el file browser
          writeString("page file");          
          delay(250);
          refreshFiles();

        }    
        // Ajuste del volumen
        else if (strCmd.indexOf("VOL=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];
          MAIN_VOL = valVol;
          
          // Ajustamos el volumen
          logln("Main volume value=" + String(MAIN_VOL));
          // Control del Master volume
          MAIN_VOL_L = MAIN_VOL;
          MAIN_VOL_R = MAIN_VOL;

          // ESP32kit.setVolume(MAIN_VOL);
          //ESP32kit.maxAmplitude = 
          //_zxp.set_amplitude(MAIN_VOL * 32767 / 100);
        }
        // Ajuste el vol canal R
        else if (strCmd.indexOf("VRR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          MAIN_VOL_R = valVol;
        }
        // Ajuste el vol canal L
        else if (strCmd.indexOf("VLL=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];
          MAIN_VOL_L = valVol;
          
          // Ajustamos el volumen
          #ifdef DEBUGMODE
            logln("");
            logln("L-Channel volume value=" + String(MAIN_VOL_L));
            logln("");
          #endif
        }
        // Devuelve el % de filtrado aplicado en pantalla. Filtro del recording
        else if (strCmd.indexOf("THR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valThr = (int)buff[4];
          SCHMITT_THR = valThr;
          logln("Threshold value=" + String(SCHMITT_THR));
        }
        // Habilitar terminadores para forzar siguiente pulso a HIGH
        else if (strCmd.indexOf("TER=") != -1) 
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

          // #ifdef DEBUGMODE
            logln("");
            log("Terminadores =" + String(APPLY_END));
          // #endif
        }
        // Polarización de la señal
        else if (strCmd.indexOf("PLZ=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Empieza en DOWN
              POLARIZATION = up;
              LAST_EAR_IS = up;
              INVERSETRAIN = true;
          }
          else
          {
              // Empieza en UP
              POLARIZATION = down;
              LAST_EAR_IS = down;
              INVERSETRAIN = false;
          }

          logln("");
          log("Polarization =" + String(INVERSETRAIN));

        }
        // Nivel LOW a cero
        else if (strCmd.indexOf("ZER=") != -1) 
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
          
          logln("");
          log("Down level is ZERO =" + String(ZEROLEVEL));

        }
        // Enable Schmitt Trigger threshold adjust
        else if (strCmd.indexOf("ESH=") != -1) 
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

          logln("");
          log("Threshold enable=" + String(EN_SCHMITT_CHANGE));

        }
        // Mutea la salida amplificada
        else if (strCmd.indexOf("MAM=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          ACTIVE_AMP = !valEn;

          // Bajamos el volumen del speaker que esta en el canal amplificado DERECHO
          MAIN_VOL_R = 5;
          // Actualizamos el HMI
          writeString("menuAudio.volR.val=" + String(MAIN_VOL_R));
          writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_R));

          // Habilitamos/Deshabilitamos el amplificador
          ESP32kit.setSpeakerActive(ACTIVE_AMP);
          logln("Active amp=" + String(ACTIVE_AMP));
        }        
        // Habilita los dos canales
        else if (strCmd.indexOf("STE=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          EN_STEREO = valEn;

          logln("Mute enable=" + String(EN_STEREO));
        }
        // Enable MIC left channel - Option
        else if (strCmd.indexOf("EMI=") != -1) 
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
          logln("MIC LEFT enable=" + String(SWAP_MIC_CHANNEL));
        }
        // Enable MIC left channel - Option
        else if (strCmd.indexOf("EAR=") != -1) 
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

          // Ahora mutamos el Amplificador en ambos estados del SWAP
          // ya que el usuario decida si lo activa al quitar el swap
          ACTIVE_AMP = false;
          writeString("menuAudio.mutAmp.val=1");
          // Habilitamos/Deshabilitamos el amplificador
          ESP32kit.setSpeakerActive(ACTIVE_AMP);

          logln("EAR LEFT enable=" + String(SWAP_EAR_CHANNEL));
        }
        // Save polarization in ID 0x2B
        else if (strCmd.indexOf("SAV") != -1) 
        {
          //Guardamos la configuracion en un fichero
          String path = FILE_LAST_DIR;
          String tmpPath = PATH_FILE_TO_LOAD;
          char strpath[tmpPath.length() + 5] = {};
          strcpy(strpath,tmpPath.c_str());
          strcat(strpath,".cfg");

          File32 cfg;

          logln("");
          log("Saving configuration in " + String(strpath));

          if (cfg.open(strpath,O_WRITE | O_CREAT))
          {
            // Creamos el fichero de configuracion.
 
            logln("");
            log("file created - ");

            // Ahora escribimos la configuracion
            cfg.println("<freq>"+ String(SAMPLING_RATE) +"</freq>");        
            cfg.println("<zerolevel>" + String(ZEROLEVEL) + "</zerolevel>");
            cfg.println("<blockend>" + String(APPLY_END) + "</blockend>");     

            if (POLARIZATION == up)
            {
               cfg.println("<polarized>1</polarized>");       
            } 
            else
            {
              cfg.println("<polarized>0</polarized>");
            }
          
            cfg.close();

          }
          
          #ifdef DEBUGMODE
            log("Config. saved");
          #endif
        }
        // Sampling rate
        else if (strCmd.indexOf("SAM=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==0)
          {
              SAMPLING_RATE = 48000;
              writeString("tape.lblFreq.txt=\"48KHz\"" );
          }
          else if(valEn==1)
          {
              SAMPLING_RATE = 44100;
              writeString("tape.lblFreq.txt=\"44KHz\"" );
          }
          else if(valEn==2)
          {
              SAMPLING_RATE = 32000;
              writeString("tape.lblFreq.txt=\"32KHz\"" );
          }
          else if(valEn==3)
          {
              SAMPLING_RATE = 22050;
              writeString("tape.lblFreq.txt=\"22KHz\"" );
          }
          
          #ifdef DEBUGMODE
            log("Sampling rate =" + String(SAMPLING_RATE));
          #endif
        }
        // Habilitar recording sobre WAV file
        else if (strCmd.indexOf("WAV=") != -1) 
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

          #ifdef DEBUGMODE
            logln("Modo WAV =" + String(MODEWAV));
          #endif
        }
        // Habilitar Audio output cuando está grabando.
        else if (strCmd.indexOf("LOO=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Activa el loop de audio
              REC_AUDIO_LOOP = true;
          }
          else
          {
              REC_AUDIO_LOOP = false;
          }
        }
        // Habilitar/Des WiFi RADIO.
        else if (strCmd.indexOf("WIF=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Activa el loop de audio
              WIFI_ENABLE = true;
          }
          else
          {
              WIFI_ENABLE = false;
          }

          if (WIFI_ENABLE)
          {
              activateWifi(true);
          }
          else
          {
              activateWifi(false);
          }          
        }
        // Show data debug by serial console
        else if (strCmd.indexOf("SDD=") != -1) 
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
          logln("SHOW_DATA_DEBUG enable=" + String(SHOW_DATA_DEBUG));
        }     
        // Parametrizado del timming maquinas. ROM estandar (no se usa)
        else if (strCmd.indexOf("MP1=") != -1) 
        {
          //minSync1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_SYNC=val;
          logln("MP1=" + String(MIN_SYNC));
        }
        else if (strCmd.indexOf("MP2=") != -1) 
        {
          //maxSync1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_SYNC=val;
          logln("MP2=" + String(MAX_SYNC));
        }
        else if (strCmd.indexOf("MP3=") != -1) 
        {
          //minBit0
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_BIT0=val;
          logln("MP3=" + String(MIN_BIT0));
        }
        else if (strCmd.indexOf("MP4=") != -1) 
        {
          //maxBit0
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_BIT0=val;
          logln("MP4=" + String(MAX_BIT0));
        }
        else if (strCmd.indexOf("MP5=") != -1) 
        {
          //minBit1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_BIT1=val;
          logln("MP5=" + String(MIN_BIT1));
        }
        else if (strCmd.indexOf("MP6=") != -1) 
        {
          //maxBit1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_BIT1=val;
          logln("MP6=" + String(MAX_BIT1));
        }
        else if (strCmd.indexOf("MP7=") != -1) 
        {
          //max pulses lead
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);

          // logln("0: " + String((char)buff[0]));
          // logln("1: " + String((char)buff[1]));
          // logln("2: " + String((char)buff[2]));
          // logln("3: " + String((char)buff[3]));
          // logln("4: " + String(buff[4]));
          // logln("5: " + String(buff[5]));
          // logln("6: " + String(buff[6]));
          // logln("7: " + String(buff[7]));

          long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
          //
          MAX_PULSES_LEAD=val;
          logln("MP7=" + String(MAX_PULSES_LEAD));
        }
        else if (strCmd.indexOf("MP8=") != -1) 
        {
          //minLead
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_LEAD=val;
          logln("MP8=" + String(MIN_LEAD));
        }
        else if (strCmd.indexOf("MP9=") != -1) 
        {
          //maxLead
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = buff[4];
          //
          MAX_LEAD=val;
          logln("MP9=" + String(MAX_LEAD));
        }
        // Control de volumen por botones - MASTER
        else if (strCmd.indexOf("VOLUP") != -1) 
        {
          MAIN_VOL += 1;
          
          if (MAIN_VOL >100)
          {
            MAIN_VOL = 100;

          }
        }        
        else if (strCmd.indexOf("VOLDW") != -1) 
        {
          MAIN_VOL -= 1;
          
          if (MAIN_VOL < 30)
          {
            MAIN_VOL = 30;
          }
          // logln("");
          // logln("VOL DOWN");
          // logln("");
        }
        // Busqueda de ficheros
        else if (strCmd.indexOf("TXTF=") != -1) 
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

          #ifdef DEBUGMODE
            logln("");
            log("Search mode.");
            logln("");
            log("Text to search: ");
            log(phrase);
          #endif

          // Ahora localizamos los ficheros
          String(tmpPhrase) = phrase;
          tmpPhrase.trim();
          FILE_TXT_TO_SEARCH = tmpPhrase;
          //FILE_BROWSER_SEARCHING = true;
          
          findTheTextInFiles();
        }
        else if (strCmd.indexOf("PDEBUG") != -1)
        {
            // Estamos en la pantalla DEBUG
            #ifdef DEBUGMODE
              logAlert("PAGE DEBUG");
            #endif
            CURRENT_PAGE = 3;
        }
        else if (strCmd.indexOf("PMENU1") != -1)
        {
            // Estamos en la pantalla MENU
            #ifdef DEBUGMODE
              logAlert("PAGE MENU");
            #endif
            CURRENT_PAGE = 2;
        }
        else if (strCmd.indexOf("PTAPE") != -1)
        {
            // Estamos en la pantalla TAPE
            #ifdef DEBUGMODE
              logAlert("PAGE TAPE");
            #endif
            CURRENT_PAGE = 1;
            updateInformationMainPage(true);
        }
        else
        {}

      //Fin del procedimiento
      }        

      void writeString(String stringData) 
      {
      
          SerialHW.flush();
      
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
      
      }
      
      void writeStringBlock(String stringData) 
      {
          
          SerialHW.flush();

          for (int i = 0; i < stringData.length(); i++) 
          {
            // Enviamos los datos
            SerialHW.write(stringData[i]);
          }
    
      }      

      void write(String stringData)
      {
          for (int i = 0; i < stringData.length(); i++) 
          {
            // Enviamos los datos
            SerialHW.write(stringData[i]);
          }
      }

      void sendbin(char *data)
      {
          for (int i = 0; i < sizeof(data); i++) 
          {
            // Enviamos los datos
            SerialHW.write(data[i]);
          }
      }

      void setBasicFileInformation(int id, int group, char* name,char* typeName,int size, bool playeable)
      {
          // El ID y el GROUP solo tiene sentido en .TZX version y compatibles pero no en TAP
          if(size < 0)
          {size=0;}

          if (id == 33)
          {
            LAST_GROUP = "[META BLK: " + String(group) + "]";
          }
          else if (id == 34)
          {
            LAST_GROUP = "[END MET.BLK]";
          }
          else
          {
            LAST_GROUP = "";
          }
          
          LAST_SIZE = size;
          strncpy(LAST_NAME,name,14);
          strncpy(LAST_TYPE,typeName,35);


          if (!playeable)
          {
            LAST_SIZE = 0;
          }
          // }
          // else
          // {
          //   LAST_SIZE = 0;
          //   strncpy(LAST_NAME,"...",14);
          //   strncpy(LAST_TYPE,typeName,35);
          // }
      }

      void clearInformationFile()
      {
          PROGRAM_NAME = "";
          lastPrgName = "";
          PROGRAM_NAME_2 = "";
          lastPrgName2 = "";
          LAST_SIZE = 0;
          strncpy(LAST_NAME,"",1);
          strncpy(LAST_TYPE,"",1);
          lastType = "";
          LAST_GROUP = "";
          lastGrp = "";
          lastBl1 = BLOCK_SELECTED = 0;
          lastBl2 = TOTAL_BLOCKS = 0;
          lastPr1 = PROGRESS_BAR_BLOCK_VALUE = 0;
          lastPr2 = PROGRESS_BAR_TOTAL_VALUE = 0;


          // Forzamos un actualizado de la información del tape
          // writeString("name.txt=\"" + PROGRAM_NAME + " : " + PROGRAM_NAME_2 + "\"");
          writeString("name.txt=\"" + PROGRAM_NAME + "\"");
          writeString("tape2.name.txt=\"" + PROGRAM_NAME + " : " + PROGRAM_NAME_2 + "\"");
          writeString("size.txt=\"0 bytes\"");
          writeString("tape2.size.txt=\"0 bytes\"");
          writeString("type.txt=\"" + String(LAST_TYPE) + " " + LAST_GROUP + "\"");
          writeString("tape2.type.txt=\"" + String(LAST_TYPE) + " " + LAST_GROUP + "\"");
          writeString("progression.val=" + String(PROGRESS_BAR_BLOCK_VALUE));   
          writeString("progressTotal.val=" + String(PROGRESS_BAR_TOTAL_VALUE));
          writeString("progressBlock.val=" + String(PROGRESS_BAR_BLOCK_VALUE));
          writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));       
          writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));                              
      }

      void updateInformationMainPage(bool FORZE_REFRESH = false) 
      {            
          // String msgPol = "";
          // String msgEAR = "";

          // if (POLARIZATION == up)
          // {
          //   msgPol = "UP";
          // }
          // else
          // {
          //   msgPol = "DOWN";
          // }

          // if (LAST_EAR_IS == up)
          // {
          //   msgEAR = "UP";
          // }
          // else
          // {
          //   msgEAR = "DOWN";
          // }

          if (PLAY)
          {
            if (CURRENT_PAGE == 3)
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
          }

          if (TOTAL_BLOCKS != 0 || REC || EJECT || FORZE_REFRESH) 
          {
        
            // Enviamos información al HMI
            if (TYPE_FILE_LOAD != "TAP" || REC || FORZE_REFRESH)
            {
                // Para TZX
                if (lastPrgName!=PROGRAM_NAME)// || lastPrgName2!=PROGRAM_NAME_2)
                {
                  // writeString("name.txt=\"" + PROGRAM_NAME + " : " + PROGRAM_NAME_2 + "\"");
                  writeString("name.txt=\"" + PROGRAM_NAME + "\"");
                  writeString("tape2.name.txt=\"" + PROGRAM_NAME + " : " + PROGRAM_NAME_2 + "\"");
                }
                lastPrgName = PROGRAM_NAME;
                lastPrgName2 = PROGRAM_NAME_2;
            }
            else
            {
                // Para TAP
                if (lastPrgName!=PROGRAM_NAME || FORZE_REFRESH)
                {
                  writeString("name.txt=\"" + PROGRAM_NAME + "\"");
                  writeString("tape2.name.txt=\"" + PROGRAM_NAME + "\"");
                }
                lastPrgName = PROGRAM_NAME;
            }
            
            if (lstLastSize!=LAST_SIZE || FORZE_REFRESH)
            {
                if (LAST_SIZE > 9999)
                {
                  writeString("size.txt=\"" + String(LAST_SIZE / 1024) + " Kb\"");
                  writeString("tape2.size.txt=\"" + String(LAST_SIZE / 1024) + " Kb\"");
                }
                else
                {
                  writeString("size.txt=\"" + String(LAST_SIZE) + " bytes\"");
                  writeString("tape2.size.txt=\"" + String(LAST_SIZE) + " bytes\"");
                }
            }

            lstLastSize = LAST_SIZE;

            String cmpTypeStr = String(LAST_NAME);
            cmpTypeStr.trim();


            if (TYPE_FILE_LOAD != "TAP" || REC || FORZE_REFRESH)
            {
                if (lastType!=LAST_TYPE || lastGrp!=LAST_GROUP || FORZE_REFRESH)
                { 
                  writeString("type.txt=\"" + String(LAST_TYPE) + " " + LAST_GROUP + "\"");
                  writeString("tape2.type.txt=\"" + String(LAST_TYPE) + " " + LAST_GROUP + "\"");
                }
                lastType = LAST_TYPE;
                lastGrp = LAST_GROUP;
            }
            else
            {

                if (lastType!=LAST_TYPE || FORZE_REFRESH)
                { 
                  writeString("type.txt=\"" + String(LAST_TYPE) + "\"");
                  writeString("tape2.type.txt=\"" + String(LAST_TYPE) + "\"");
                }
                lastType = LAST_TYPE;  

                // writeString("name.txt=\"" + PROGRAM_NAME + " : " + String(LAST_NAME) + "\"");           
                // writeString("tape2.name.txt=\"" + PROGRAM_NAME + " : " + String(LAST_NAME) + "\"");     

                if (lastPrgName!=PROGRAM_NAME || FORZE_REFRESH)
                {
                  writeString("name.txt=\"" + PROGRAM_NAME + "\"");
                  writeString("tape2.name.txt=\"" + PROGRAM_NAME + "\"");
                }
                lastPrgName = PROGRAM_NAME;                
            }
        
            //writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));
            //writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));
          }
          else if (TOTAL_BLOCKS == 0)
          {
            //writeString("totalBlocks.val=0");
            //writeString("currentBlock.val=0");
          }

          // if (lastfname != HMI_FNAME || FORZE_REFRESH)
          // {writeString("fname.txt=\"" + HMI_FNAME + "\"");}
          // lastfname = HMI_FNAME;


          if (lastMsn != LAST_MESSAGE || FORZE_REFRESH)
          {writeString("g0.txt=\"" + LAST_MESSAGE + "\"");}
          lastMsn = LAST_MESSAGE;
          
          if (lastPr1 != PROGRESS_BAR_BLOCK_VALUE || FORZE_REFRESH)
          {writeString("progressBlock.val=" + String(PROGRESS_BAR_BLOCK_VALUE));}
          lastPr1 = PROGRESS_BAR_BLOCK_VALUE;

          if (lastPr2 != PROGRESS_BAR_TOTAL_VALUE || FORZE_REFRESH)
          {writeString("progressTotal.val=" + String(PROGRESS_BAR_TOTAL_VALUE));}
          lastPr2 = PROGRESS_BAR_TOTAL_VALUE;

          if (lastBl1 != TOTAL_BLOCKS || FORZE_REFRESH)
          {
              if (TOTAL_BLOCKS == 0)
              {
                writeString("totalBlocks.val=" + String(TOTAL_BLOCKS));
              }
              else
              {
                writeString("totalBlocks.val=" + String(TOTAL_BLOCKS-1));                
              }
          }
          lastBl1 = TOTAL_BLOCKS;

          if (lastBl2 != BLOCK_SELECTED || FORZE_REFRESH)
          {writeString("currentBlock.val=" + String(BLOCK_SELECTED));}
          lastBl2 = BLOCK_SELECTED;
                            
          if (CURRENT_PAGE == 2)
          {
            // Solo mostramos la información de memoria si estamos en la pagina de Menú
            updateMem();
          }
          
      }
      
      int getEndCharPosition(String str,int start)
      {
        //logln(">> " + str);
        int pos=-1;
        int stateCr=0;
        for (int i = start; i < str.length(); i++) 
        {
          char cr = str[i];
          //logln(String(cr) + " - " + String(int(cr)));

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

        //logln("Pos: " + String(pos));
        return pos;
      }

      void getSeveralCommands(String strCmd)
      {

          String str = "";
          int cStart=0;
          int lastcStart=0;

          // Buscamos el separador de comandos que es 
          // 0x32 0x32 0x32
          
          #ifdef DEBUGMODE
            logln("");
            logln("Several commands routine");
          #endif

          int cPos = getEndCharPosition(strCmd,cStart);          

          if (cPos==-1)
          {
            // Solo hay un comando
            #ifdef DEBUGMODE
              logln("");
              logln("Only one command");
            #endif

            verifyCommand(strCmd);
          }
          else
          {
            // Hay varios
            while(cPos < strCmd.length() && cPos!=-1)
            {
              #ifdef DEBUGMODE
                logln("");
                logln("Start,pos,last: " + String(cStart) + "," + String(cPos) + "," + String(lastcStart));
              #endif

              str=strCmd.substring(cStart,cPos-2);

              #ifdef DEBUGMODE
                logln("");
                logln("Command: " + str);
              #endif

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
            // #ifdef DEBUGMODE
            //   logln("");
            //   logln("Echo: " + strCmd);
            //   logln("");
            // #endif
            getSeveralCommands(strCmd);
          }
          //SerialHW.flush();
        }
      }

      String readStr() 
      {
        String strCmd = "";

        if (SerialHW.available() >= 1) 
        {
          // get the new uint8_t:
          while (SerialHW.available())
          {
            strCmd = SerialHW.readString();
          }
          return strCmd;
        }
        else
        {
          return "";
        }
      }

      u_int8_t* readByte() 
      {
        uint8_t *b;

        if (SerialHW.available() >= 1) 
        {
          // get the new uint8_t:
          while (SerialHW.available())
          {
            SerialHW.readBytes(b,1);
          }
          return b;
        }
        else
        {
          return 0;
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
          logln("");
          logln("");
          logln("> MEM REPORT");
          logln("------------------------------------------------------------");
          logln("");
          logln("Total heap: " + String(ESP.getHeapSize() / 1024) + "KB");
          logln("Free heap: " + String(ESP.getFreeHeap() / 1024) + "KB");
          logln("Total PSRAM: " + String(ESP.getPsramSize() / 1024) + "KB");
          logln("Free PSRAM: " + String (ESP.getFreePsram() / 1024) + "KB");  
          logln("");
          logln("------------------------------------------------------------");
          //
          updateMem();
      }           

      void updateMem()
      {
          if (lst_stackFreeCore0 != stackFreeCore0 || lst_psram_used != ESP.getPsramSize() || lst_stack_used != ESP.getHeapSize())
          {
            #ifdef DEBUGMODE
              writeString("menu.totalPSRAM.txt=\"Task0: " + String(stackFreeCore0) + " KB | " + String(ESP.getPsramSize() / 1024) + " KB | " + String(ESP.getHeapSize() / 1024) + " KB\"");            
            #else
              writeString("menu.totalPSRAM.txt=\"" + String(ESP.getPsramSize() / 1024) + " KB | " + String(ESP.getHeapSize() / 1024) + " KB\"");            
            #endif
            
          }
          
          lst_stackFreeCore0 = BLOCK_SELECTED;
          lst_psram_used = ESP.getPsramSize();
          lst_stack_used = ESP.getHeapSize();

          if (lst_stackFreeCore1 != stackFreeCore1 || lst_psram_used != ESP.getFreePsram() || lst_stack_used != ESP.getFreeHeap())
          {
            #ifdef DEBUGMODE
              writeString("menu.freePSRAM.txt=\"Task1: "  + String(stackFreeCore1) + " KB | " + String(ESP.getFreePsram() / 1024) + " KB | " + String(ESP.getFreeHeap() / 1024) + " KB\"");
            #else
              writeString("menu.freePSRAM.txt=\""  + String(ESP.getFreePsram() / 1024) + " KB | " + String(ESP.getFreeHeap() / 1024) + " KB\"");
            #endif
          }
          lst_stackFreeCore1 = BLOCK_SELECTED;
          lst_psram_free = ESP.getFreePsram();
          lst_stack_free = ESP.getFreeHeap();
      }

      // Constructor
      HMI()
      {}
};
