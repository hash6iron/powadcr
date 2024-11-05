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

      void fillWithFiles(File32 &fout, File32 &fstatus)
      {
          // NOTA:
          // ***************************************************
          // Esta función toma la ruta establecida por sdm.dir
          // del manejador de ficheros.
          // ***************************************************

          // Un fichero _files.lst es del tipo
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
          char szName[255];
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

            // Ahora lo recorremos
            while (sdm.file.openNext(&sdm.dir, O_RDONLY))
            {

                // Ok. Entonces es un fichero y cogemos su extensión
                sdm.file.getName(szName,254);                       
                int8_t len = strlen(szName);
                char* substr = strlwr(szName + (len - 4));    
                uint32_t posf = sdm.dir.position();

                // ¿No es un directorio?
                if (!sdm.file.isDir())
                {
                    // ¿No está oculto?
                    if (!sdm.file.isHidden())
                    {
                        // // Ok. Entonces es un fichero y cogemos su extensión
                        // sdm.file.getName(szName,254);                       
                        // int8_t len = strlen(szName);
                        // char* substr = strlwr(szName + (len - 4));    
                        // uint32_t posf = sdm.dir.position();
                        
                        // Si tiene una de las extensiones esperadas, se almacena
                        if (strstr(substr, ".tap") || strstr(substr, ".tzx") || strstr(substr, ".tsx") || strstr(substr, ".wav")) 
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
                        // nombre del directorio
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
                sdm.file.close();
                //
                FILE_TOTAL_FILES = cdir + lpos;
                writeString("statusFILE.txt=\"FILES " + String(FILE_TOTAL_FILES-1) +"\""); 
            }
          }
     }

      void registerFiles(String path)
      {
          // Registramos todos los ficheros encontrados y sus indices en un fichero "_files.lst"
          String regFile = path + "_files.lst";
          String statusFile = path + "_files.inf";

          char* file_ch = (char*)malloc(256 * sizeof(char));
          regFile.toCharArray(file_ch, 256);

          char* filest_ch = (char*)malloc(256 * sizeof(char));
          statusFile.toCharArray(filest_ch, 256);

          // Convertimos el path a char*
          char* path_ch = (char*)malloc(256 * sizeof(char));
          path.toCharArray(path_ch, 256);

          // Manejador del fichero _files.lst
          File32 f = sdm.file;
          File32 fstatus = sdm.file;


          if (sdm.createEmptyFile32(file_ch) && sdm.createEmptyFile32(filest_ch))
          {
              logln("");
              logln("File created: " + String(file_ch));

              logln("");
              logln("File status created: " + String(filest_ch));            

              // Abrimos el fichero
              f = sdm.openFile32(file_ch);
              fstatus = sdm.openFile32(filest_ch);

              //f.println("Esto es un prueba");
              // Ahora lo rellenamos con los ficheros incluidos en el path (que hemos convertido a char*)
              logln("");
              logln("Path to search: " + String(path_ch));
              
              // Registramos la ruta y los ficheros que lo contienen en el _files.lst
              fstatus.println("PATH=" + String(path_ch));

              // Rellenamos con los ficheros que contiene el directorioç
              // y las estadísticas
              fillWithFiles(f,fstatus);
          }
          else
          {
            logln("");
            logln("Error creating _files.lst");
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
            logln("Error reading file. _files.lst is closed");
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
          logln("ptr to put in position: " + String(pos));

          // Ahora buscamos y movemos el puntero
          while(l < pos)
          {
              // Vamos saltando lineas hasta la posición esperada
              n = f.fgets(line,sizeof(line));
              l++;
          }

          logln("Position is " + String(l));
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

                  log("Data: [");
                  log(String(ld.ID));
                  log("] - ");
                  log(String(ld.seek));
                  log(" - ");
                  log(String(ld.type));
                  log(" - ");
                  log(ld.fileName);
                  logln("");

                  // Guardamos el ultimo
                  total = ld.ID;
              }

              FILE_TOTAL_FILES = total;
              logln("Total files: " + String(FILE_TOTAL_FILES));
          }        
      }

      inline bool exists_file (String name) 
      {
          // Comprueba si existe el fichero en la ruta
          struct stat buffer;   
          return (stat (name.c_str(), &buffer) == 0); 
      }

      void registerFileLST(String path, bool forze_rescan)
      {
          // registramos todos los ficheros
          // Si el _files.lst ya existe no se vuelve a crear, a no ser que sea forzado el rescan.
          File32 f;
          String fFileList = path + "_files.lst";

          if(!f.open(fFileList.c_str(), O_RDONLY))
          {
              logln("Registering files in: " + fFileList);
              registerFiles(path);
          }
          else
          {
              f.close();

              if(forze_rescan)
              {
                  logln("Registering files FORZED");
                  registerFiles(path);
              }
          }


      }

      void manageFileLST(File32& fFileLST, String path)
      {
        String fFileList = path + "_files.lst";

        // Abrimos ahora el _files.lst, para manejarlo
        // if (!IN_THE_SAME_DIR)
        // {
        if (!fFileLST.isOpen())
        {
            // Abrimos el fichero
            fFileLST.open(fFileList.c_str(), O_RDONLY);
            fFileLST.rewind();

            logln("Opening again - rewind");

            IN_THE_SAME_DIR = true;
            // Mostramos el contenido y obtenemos el total de ficheros
            //showFileLST(fFileLST);
                            //
            FILE_TOTAL_FILES = 1;
            char line[256];
            int n=0;
            while (n = fFileLST.fgets(line,sizeof(line)) > 0)
            {
                FILE_TOTAL_FILES++;
            }
        }
        // }       
      }

      void getFilesFromSD(bool forze_rescan)
      {
       
          // Cadena para un filePath
          char szName[255];
          int j = 0;

          File32 fFileLST;
          tFileLST fl;

          clearFileBuffer();

          logln("--------------------------------------");
          logln("Searching files in - " + FILE_LAST_DIR);

          if (sdm.dir.isOpen())
          {
              sdm.dir.close();
          }

          if (!sdm.dir.open(FILE_LAST_DIR.c_str())) 
          {
              FILE_DIR_OPEN_FAILED = true;
          }
          else
          {
              // El directorio se ha abierto sin problemas
              FILE_DIR_OPEN_FAILED = false;

              if (FILE_LAST_DIR == "/REC/")
              {
                forze_rescan=true;
              }

              // Si no existe el historico de los ficheros se genera un _file.lst
              registerFileLST(FILE_LAST_DIR, forze_rescan);

              // Usamos el fichero que contiene el mapa del directorio actual, _files.lst
              manageFileLST(fFileLST, FILE_LAST_DIR);
    
              // AÑADIMOS AL INICIO EL PARENT DIR
              FILES_BUFF[0].isDir = true;
              FILES_BUFF[0].type = "PAR";
              FILES_BUFF[0].path = String(FILE_PREVIOUS_DIR);
              FILES_BUFF[0].fileposition = sdm.file.position();

              logln("Showing files from: " + String(FILE_PTR_POS));

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
          }

          writeString("statusFILE.txt=\"FILES " + String(FILE_TOTAL_FILES-1) +"\"");

          // Cerramos todos los ficheros (añadido el 25/07/2023)
          sdm.dir.close();
          fFileLST.close();

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
            //delay(5);
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
              //delay(5);
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
        
        // Esto es solo para el parent dir, para que siempre salga arriba
        szName = FILES_BUFF[0].path;
        color = 2016;  // Verde
        szName = String(".. ") + szName;
        
        writeString("");
        writeString("prevDir.txt=\"" + String(szName) + "\"");
        writeString("");
        writeString("prevDir.pco=" + String(color));
                  
        for (int i=1;i < MAX_FILES_TO_LOAD;i++)
        {
              szName = FILES_BUFF[i].path;
              type = FILES_BUFF[i].type;
              //logln("File type: " + type);

              // Convertimos a mayusculas el tipo
              type.toUpperCase();

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
      
              printFileRows(i-1, color, szName);
              //delay(5);
              //printFileRows(pos_in_HMI_file, color, szName);
              //pos_in_HMI_file++;
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
          IN_THE_SAME_DIR = false;
          clearFilesInScreen();
          getFilesFromSD(false);
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
          FILE_PTR_POS = 0;
          getFilesFromSD(true);       
          refreshFiles();        
      }

      void verifyCommand(String strCmd) 
      {
        
        // Reiniciamos el comando
        LAST_COMMAND = "";

        // Selección de bloque desde keypad - pantalla
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
            
        }

        if (strCmd.indexOf("PAG=") != -1) 
        {
            // Con este procedimiento obtenemos la pàgina del filebrowser
            // que se desea visualizar
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
            int pageSelected = num.toInt();

            // Calculamos el total de páginas
            int totalPages = (FILE_TOTAL_FILES / TOTAL_FILES_IN_BROWSER_PAGE);
            if (FILE_TOTAL_FILES % TOTAL_FILES_IN_BROWSER_PAGE != 0)
            {
                totalPages+=1;
            }            

            logln("");
            logln("Page selected: " + String(pageSelected+1) + "/" + String(totalPages));

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
            getFilesFromSD(false);
            // Refrescamos el listado de ficheros visualizado
            refreshFiles();            
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
      
            FILE_PREPARED = false;
            FILE_SELECTED = false;

            resetIndicators();

            if (FILE_LAST_DIR_LAST != FILE_LAST_DIR)
            {

                if (sdm.dir.isOpen())
                {
                    sdm.dir.close();             
                }              

                getFilesFromSD(false);
            }

            if (!FILE_DIR_OPEN_FAILED)
            {

                clearFilesInScreen();
                putFilesInScreen();
                FILE_STATUS = 1;
                // El GFIL desconecta el filtro de busqueda
                FILE_BROWSER_SEARCHING = false;                   
            }            
        }

        if (strCmd.indexOf("RFSH") != -1) 
        {
            reloadDir();
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

            getFilesFromSD(false);
            refreshFiles();
        }
      
        if (strCmd.indexOf("FPDOWN") != -1) 
        {
            // Con este comando nos indica la pantalla que quiere
            // le devolvamos ficheros en la posición actual del puntero
            FILE_PTR_POS = FILE_PTR_POS + TOTAL_FILES_IN_BROWSER_PAGE;
  
            if (FILE_PTR_POS > FILE_TOTAL_FILES)
            {
                // Hacemos esto para permanecer en la misma pagina
                FILE_PTR_POS -= TOTAL_FILES_IN_BROWSER_PAGE;
            }

            getFilesFromSD(false);
            refreshFiles();
        }
      
        if (strCmd.indexOf("FPHOME") != -1) 
        {
            // Con este comando nos indica la pantalla que quiere
            // le devolvamos ficheros en la posición actual del puntero
            putInHome();
      
            //writeString("statusFILE.txt=\"GETTING FILES\"");
      
            getFilesFromSD(false);
      
            //writeString("statusFILE.txt=\"\"");
      
        }
      
        if (strCmd.indexOf("CHD=") != -1) 
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
            
            logln("Dir to change " + FILE_DIR_TO_CHANGE);
            
            IN_THE_SAME_DIR = false;
      
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
      
            getFilesFromSD(false);
            
            if (!FILE_DIR_OPEN_FAILED)
            {
                //putFilesInScreen();
                putFilesInScreen();
            }
            else
            {
              logln("Error to open directory");
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
            getFilesFromSD(false);
            
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
                FILE_TO_DELETE = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED+1].path;     
      
                SerialHW.println();
                SerialHW.println();
                SerialHW.println("File to delete: " + FILE_TO_DELETE);  
                FILE_SELECTED_DELETE = true; 
            }
            else
            {
                FILE_TO_DELETE = FILE_LAST_DIR + FILES_FOUND_BUFF[FILE_IDX_SELECTED+1].path;     
      
                SerialHW.println();
                SerialHW.println();
                SerialHW.println("File to delete in buffer: " + FILE_TO_DELETE); 
                FILE_SELECTED_DELETE = true;  
            }

            if (FILE_SELECTED_DELETE)
            {
              // Lo Borramos
                File32 mf = _sdf.open(FILE_TO_DELETE,O_WRONLY);

                if (!mf.isWritable())
                {
                  writeString("currentDir.txt=\">> Error. File not writeable <<\"");
                  mf.close();
                }
                else if (mf.isLFN())
                { 
                  writeString("currentDir.txt=\">> Error. Long filename <<\"");
                  mf.close();
                }
                else if (mf.isReadOnly())
                { 
                  writeString("currentDir.txt=\">> Error. Readonly file <<\"");
                  mf.close();
                }
                else if (mf.isSystem())
                { 
                  writeString("currentDir.txt=\">> Error. System file <<\"");
                  mf.close();
                }
                else
                {
                    mf.close();

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
                      
                      // Tras borrar hacemos un rescan
                      getFilesFromSD(true);       
                      refreshFiles();
                    }                  
                }


            }
        }      

        // Load file - Carga en el TAPE el fichero seleccionado en pantalla
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
                FILE_TO_LOAD = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED+1].path;                    
            }
            else
            {
                FILE_TO_LOAD = FILE_LAST_DIR + FILES_FOUND_BUFF[FILE_IDX_SELECTED+1].path;           
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
      
        // Configuración de frecuencias de muestreo
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

        // Indica que la pantalla está activa
        if (strCmd.indexOf("LCDON") != -1) 
        {
            LCD_ON = true;
        }
      
        // Control de TAPE
        if (strCmd.indexOf("FFWD") != -1) 
        {
          if (LOADING_STATE == 2 || LOADING_STATE == 3 || LOADING_STATE == 0) 
          {
            BLOCK_SELECTED++;
      
            if (BLOCK_SELECTED > TOTAL_BLOCKS - 1) 
            {
              BLOCK_SELECTED = 0;
            }
            FFWIND = true;
            RWIND = false;
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
        
        // Ajuste del volumen
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

        // Ajuste el vol canal R
        if (strCmd.indexOf("VRR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          MAIN_VOL_R = valVol;
          
        }

        // Ajuste el vol canal L
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

        // Devuelve el % de filtrado aplicado en pantalla. Filtro del recording
        if (strCmd.indexOf("THR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valThr = (int)buff[4];
          SCHMITT_THR = valThr;
          SerialHW.println("Threshold value=" + String(SCHMITT_THR));
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

        // Habilitar Audio output cuando está grabando.
        if (strCmd.indexOf("LOO=") != -1) 
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
        if (strCmd.indexOf("WIF=") != -1) 
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

        // Parametrizado del timming maquinas. ROM estandar (no se usa)
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

        // Control de volumen por botones - MASTER
        if (strCmd.indexOf("VOLUP") != -1) 
        {
          MAIN_VOL += 1;
          
          if (MAIN_VOL >100)
          {
            MAIN_VOL = 100;

          }
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
      
        // Busqueda de ficheros
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

      //Fin del procedimiento
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
          if (TYPE_FILE_LOAD != "TAP" || REC)
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


          if (TYPE_FILE_LOAD != "TAP" || REC)
          {
              writeString("type.txt=\"" + String(LAST_TYPE) + " " + LAST_GROUP + "\"");
              writeString("tape2.type.txt=\"" + String(LAST_TYPE) + " " + LAST_GROUP + "\"");
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
