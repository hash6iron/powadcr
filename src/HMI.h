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

#include <vector>
// OTA SD Update
// -----------------------------------------------------------------------
#include <Update.h>

#ifdef AUTO_UPDATE
  #include "HTTPClient.h"
  #include "WiFiClientSecure.h"
  #include "ArduinoJson.h"  
#endif

#pragma once

class HMI
{

      //SDMMC _sdm;
      //FS _sdf;
      File fFileLST;
      
      private:

      


      struct tFileLST
      {
          int ID=0;
          char type='F';
          int seek=0;
          String fileName="";
          String fileType="";
      };

      bool copyFileFS(const char* srcPath, const char* dstPath) {
          File srcFile = SD_MMC.open(srcPath, FILE_READ);
          if (!srcFile) return false;

          File dstFile = SD_MMC.open(dstPath, FILE_WRITE);
          if (!dstFile) {
              srcFile.close();
              return false;
          }

          uint8_t buffer[512];
          size_t bytesRead;
          int bytesCopied = 0;
          while ((bytesRead = srcFile.read(buffer, sizeof(buffer))) > 0) {
              dstFile.write(buffer, bytesRead);
              bytesCopied += bytesRead;
              writeString("statusFILE.txt=\"COPY " + String((bytesCopied * 100) / srcFile.size()) + "%\"");

          }

          srcFile.close();
          dstFile.close();
          return true;
      }

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

      const char* getFileExtension(const char* szName) {
          const char* dot = strrchr(szName, '.');
          if (dot && *(dot + 1) != '\0') {
              return dot + 1; // Devuelve un puntero a la extensión (sin el punto)
          }
          return ""; // No hay extensión
      }      

      bool clearFile(const char* path) 
      {
        File file = SD_MMC.open(path, FILE_WRITE);
        if (file) 
        {
            file.close(); // El archivo ahora está vacío (0 bytes)
            return true;
        }
        file.close();
        return false; // Error al abrir el archivo
      }

      // Añade esta función helper para reintentos de lectura SD
      bool safeSDRead(File &file, char* buffer, size_t length, int maxRetries = 3) {
          for(int retry = 0; retry < maxRetries; retry++) {
              if(file.read((uint8_t*)buffer, length) == length) {
                  return true;
              }
              // Pequeña pausa entre reintentos
              delay(10); 
          }
          return false;
      }

      void sortFile(File &file, bool firstDir = true) 
      {
          if (!file) return;

          // Pre-reservar memoria en el heap en lugar de la pila
          const size_t BUFFER_SIZE = 1024;
          char* buffer = (char*)malloc(BUFFER_SIZE);
          if (!buffer) {
              writeString("statusFILE.txt=\"MEM ERROR\"");
              return;
          }

          std::vector<std::tuple<String, String, String, String>> linesWithKeys;
          // linesWithKeys.reserve(1024);
          linesWithKeys.reserve(FILE_TOTAL_FILES < 256 ? FILE_TOTAL_FILES : 256); // Limitar reserva
          
          file.seek(0);
          writeString("statusFILE.txt=\"READING 0%\"");
          
          int totalLines = 0;
          int currentLine = 0;
          int readErrors = 0;
          int lastProgress = -1;

          // Primer conteo de líneas
          while (file.available()) {
              yield();
              if(!safeSDRead(file, buffer, 1)) {
                  readErrors++;
                  if(readErrors > 5) {
                      free(buffer);
                      writeString("statusFILE.txt=\"SD READ ERROR\"");
                      delay(1000);
                      return;
                  }
                  continue;
              }
              if(buffer[0] == '\n') totalLines++;
          }

          // Volver al inicio
          file.seek(0);
          readErrors = 0;

          // Leer y procesar líneas
          String accumulatedLine;
          while (file.available()) {
              yield();
              
              size_t bytesToRead = min((size_t)file.available(), (size_t)(BUFFER_SIZE-1));
              
              if(!safeSDRead(file, buffer, bytesToRead)) {
                  readErrors++;
                  if(readErrors > 5) {
                      free(buffer);
                      writeString("statusFILE.txt=\"SD READ ERROR\"");
                      delay(1000);
                      return;
                  }
                  continue;
              }

              buffer[bytesToRead] = '\0';
              accumulatedLine += String(buffer);
              
              int lineEnd;
              while ((lineEnd = accumulatedLine.indexOf('\n')) != -1) {
                  yield();
                  
                  String line = accumulatedLine.substring(0, lineEnd);
                  line.trim();
                  
                  // Procesar la línea
                  int idx1 = line.indexOf('|');
                  if (idx1 == -1) continue;

                  int idx2 = line.indexOf('|', idx1 + 1);
                  if (idx2 == -1) continue;

                  int idx3 = line.indexOf('|', idx2 + 1);
                  if (idx3 == -1) continue;

                  int idx4 = line.indexOf('|', idx3 + 1);
                  if (idx4 == -1) continue;

                  String tipo = line.substring(idx1 + 1, idx2);
                  String nombre = line.substring(idx3 + 1, idx4);
                  String indice = line.substring(0, idx1);

                  tipo.trim();
                  nombre.trim();
                  
                  linesWithKeys.emplace_back(std::move(tipo), std::move(nombre), 
                                          std::move(indice), std::move(line));

                  currentLine++;
                  int progress = (currentLine * 100) / totalLines;
                  if (progress % 5 == 0 && progress != lastProgress) {
                      writeString("statusFILE.txt=\"READING " + String(progress) + "%\"");
                      lastProgress = progress;
                      yield();
                  }

                  accumulatedLine = accumulatedLine.substring(lineEnd + 1);
              }
          }

          free(buffer);

          // Ordenar usando comparador que depende de firstDir
          writeString("statusFILE.txt=\"SORTING\"");
          yield();

          std::sort(linesWithKeys.begin(), linesWithKeys.end(),
              [firstDir](const std::tuple<String,String,String,String> &a, 
                        const std::tuple<String,String,String,String> &b) {
                  const String &tipoA = std::get<0>(a);
                  const String &nombreA = std::get<1>(a);
                  const String &tipoB = std::get<0>(b);
                  const String &nombreB = std::get<1>(b);

                  bool aIsDir = (tipoA == "D");
                  bool bIsDir = (tipoB == "D");
                  
                  // Detectar archivos especiales .lst y .inf
                  bool aIsSpecial = nombreA.endsWith(".lst") || nombreA.endsWith(".inf") || nombreA.endsWith(".txt");
                  bool bIsSpecial = nombreB.endsWith(".lst") || nombreB.endsWith(".inf") || nombreB.endsWith(".txt");

                  // Los archivos especiales (.lst .inf) siempre van al final
                  if (aIsSpecial != bIsSpecial) {
                      return !aIsSpecial; // Los especiales van al final
                  }

                  // Si ambos son archivos especiales, ordenar por nombre
                  if (aIsSpecial && bIsSpecial) {
                      return nombreA.compareTo(nombreB) < 0;
                  }

                  // Lógica principal según firstDir
                  if (firstDir) {
                      // firstDir = true: Directorios primero, luego archivos
                      if (aIsDir != bIsDir) {
                          return aIsDir; // Directorios primero
                      }
                  } else {
                      // firstDir = false: Archivos primero, luego directorios  
                      if (aIsDir != bIsDir) {
                          return !aIsDir; // Archivos primero
                      }
                  }

                  // Si son del mismo tipo, ordenar por nombre
                  return nombreA.compareTo(nombreB) < 0;
              }
          );

          // Escribir archivo ordenado
          writeString("statusFILE.txt=\"SAVING\"");
          yield();

          if (file) {
              String filepath = file.path();
              file.close();
              file = SD_MMC.open(filepath.c_str(), FILE_WRITE);
              
              lastProgress = -1;
              const size_t totalItems = linesWithKeys.size();
              
              for (size_t i = 0; i < totalItems; i++) {
                  yield();
                  file.println(std::get<3>(linesWithKeys[i]));
                  
                  int progress = (i * 100) / totalItems;
                  if (progress % 5 == 0 && progress != lastProgress) {
                      writeString("statusFILE.txt=\"SAVING " + String(progress) + "%\"");
                      lastProgress = progress;
                      yield();
                  }
              }
              
              file.flush();
          }

          writeString("statusFILE.txt=\"SORT COMPLTE\"");
          linesWithKeys.clear();
          linesWithKeys.shrink_to_fit();
      }

      void sortFile_old(File &file, bool firstDir = true) 
      {
          if (!file) return;
          
          file.seek(0);
          std::vector<std::tuple<String, String, String, String>> linesWithKeys;
          linesWithKeys.reserve(FILE_TOTAL_FILES);
          String lineStr;

          // 1. Leemos las líneas y las almacenamos
          while (file.available()) {
              lineStr = file.readStringUntil('\n');
              if (lineStr.length() == 0) continue;
              lineStr.trim();

              int idx1 = lineStr.indexOf('|');
              if (idx1 == -1) continue;

              int idx2 = lineStr.indexOf('|', idx1 + 1);
              if (idx2 == -1) continue;

              int idx3 = lineStr.indexOf('|', idx2 + 1);
              if (idx3 == -1) continue;

              int idx4 = lineStr.indexOf('|', idx3 + 1);
              if (idx4 == -1) continue;

              String tipo = lineStr.substring(idx1 + 1, idx2);
              String nombre = lineStr.substring(idx3 + 1, idx4);
              String indice = lineStr.substring(0, idx1);

              tipo.trim();
              nombre.trim();

              linesWithKeys.push_back(std::make_tuple(tipo, nombre, indice, lineStr));
          }

          // 2. Ordenamos usando un comparador compatible con C++11
          std::sort(linesWithKeys.begin(), linesWithKeys.end(),
              [firstDir](const std::tuple<String,String,String,String> &a, 
                        const std::tuple<String,String,String,String> &b) {
                  const String &tipoA = std::get<0>(a);
                  const String &nombreA = std::get<1>(a);
                  const String &tipoB = std::get<0>(b);
                  const String &nombreB = std::get<1>(b);

                  bool aIsDir = (tipoA == "D");
                  bool bIsDir = (tipoB == "D");

                  if (aIsDir != bIsDir) {
                      return firstDir ? aIsDir : !aIsDir;
                  }

                  bool aIsUnderscore = nombreA.startsWith("_");
                  bool bIsUnderscore = nombreB.startsWith("_");
                  if (aIsUnderscore != bIsUnderscore) {
                      return !aIsUnderscore;
                  }

                  return nombreA.compareTo(nombreB) < 0;
              }
          );

          // 3. Escribimos el archivo ordenado
          if (file) {
              String filepath = file.path();
              file.close();
              file = SD_MMC.open(filepath.c_str(), FILE_WRITE);
              file.seek(0);
              
              // En lugar de truncate, usamos write y close
              file.close();
              file = SD_MMC.open(filepath.c_str(), FILE_WRITE);

              for (const auto& tup : linesWithKeys) {
                  file.println(std::get<3>(tup));
              }
              file.flush();
          }

          // 4. Liberamos memoria
          linesWithKeys.clear();
          linesWithKeys.shrink_to_fit();
      }

      inline bool hasInvalidASCII(const String& str) {
          const char* data = str.c_str();
          
          // Solo los caracteres de control (0-31) y el DEL (127) son inválidos
          // para nombres de archivo en tu sistema
          static const char invalid_chars[] = 
              "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0A\x0B\x0C\x0D\x0E\x0F"
              "\x10\x11\x12\x13\x14\x15\x16\x17\x18\x19\x1A\x1B\x1C\x1D\x1E\x1F"
              "\x7F"; // Solo el carácter DEL (127)
          
          // strpbrk busca el PRIMER carácter que SÍ está en invalid_chars
          return strpbrk(data, invalid_chars) != NULL;
      }

      void fillWithFilesFromFile(File &fout, File &fstatus, const String &search_pattern, const String &path) 
      {
          // Ruta del archivo _files.lst
          String path_file_lst = path + "_files.lst";
      
          // Verificamos si el archivo _files.lst existe
          if (!SD_MMC.exists(path_file_lst.c_str())) {
              #ifdef DEBUGMODE
                  logln("File _files.lst does not exist: " + path_file_lst);
              #endif
              return;
          }

          #ifdef DEBUGMODE
              logln("Reading file: " + path_file_lst);
          #endif

          // Abrimos el archivo _files.lst
          File lstFile;
          lstFile = SD_MMC.open(path_file_lst.c_str(), FILE_READ);
          if (!lstFile) {
              #ifdef DEBUGMODE
                  logln("Failed to open _files.lst: " + path_file_lst);
              #endif
              return;
          }
      
          lstFile.seek(0);
      
          // Variables para procesar el archivo
          char line[256];
          String strLine;
          int n = 0;
          //int itemsCount = 0;
          int itemsToShow = 0;
          FILE_TOTAL_FILES = 0;
      
          String searchPatternUC = search_pattern;
          searchPatternUC.toUpperCase(); // Convertimos el patrón de búsqueda a mayúsculas
      
          // Recorremos el archivo línea por línea
          while (lstFile.available()) {
              strLine = lstFile.readStringUntil('\n');
              strLine.toCharArray(line, sizeof(line));
              n = strlen(line);
              line[n - 1] = 0; // Eliminamos el terminador '\n'              line[n - 1] = 0; // Eliminamos el terminador '\n'
      
              // Extraemos los parámetros de la línea
              tFileLST fl = getParametersFromLine(line);
      
              // Convertimos el nombre del archivo a mayúsculas para la comparación
              String filenameUC = fl.fileName;
              filenameUC.toUpperCase();
      
              // Si el archivo cumple con el patrón de búsqueda, lo escribimos en fout
              if (filenameUC.indexOf(searchPatternUC) != -1) {
                  fout.printf("%d|%s|%d|%s|\n", fl.ID, fl.fileType.c_str(), fl.seek, fl.fileName.c_str());
                  FILE_TOTAL_FILES++;
              }
      
              // Actualizamos el estado en HMI
              //itemsCount++;
              itemsToShow++;
              if (itemsToShow >= EACH_FILES_REFRESH) {
                  writeString("statusFILE.txt=\"FOUND " + String(FILE_TOTAL_FILES) + "\"");
                  itemsToShow = 0; // Reiniciamos el contador
              }

          }
      
          // Cerramos el archivo
          lstFile.close();
      
          #ifdef DEBUGMODE
              logln("Total files found: " + String(FILE_TOTAL_FILES));
          #endif
      }

    
      void fillWithFiles(File &fout, File &fstatus, String search_pattern)
      {
          // *****************************************************************************
          //
          // Con este metodo, leemos el directorio actual y volcamos los ficheros en fout
          //
          // *****************************************************************************

          //char szName[256];
          const String separator="|";
          if (!global_dir.isDirectory()) return;
          global_dir.seek(0);
      
          int lpos = 1, cdir = 0, cfiles = 0;
          int itemsCount = 0, itemsToShow = 0;
          FILE_TOTAL_FILES = 0;
      
          // Convierte el patrón de búsqueda a minúsculas una sola vez
          search_pattern.toLowerCase();
      
          #ifdef DEBUGMODE
              logln("Registering files with pattern: " + search_pattern);
          #endif

          String entry;
          while (entry = global_dir.getNextFileName())
          {
              
              if (entry == "") break;

              #ifdef DEBUGMODE
                  logln("Reading file: " + entry);
              #endif

              //esp_task_wdt_reset();
              String fname = entry;
              String fnameToLower = "";
              size_t len = fname.length();
      
              if (len != 0)
              {
                  fname = String(entry).substring(String(entry).lastIndexOf('/') + 1);
                  fnameToLower = fname;
                  fnameToLower.toLowerCase();

                  // Obtenemos la extensión del fichero
                  const char *ext = getFileExtension(fname.c_str());

                  #ifdef DEBUGMODE
                      logln("File: " + fname);
                      if (ext) logln("Extension: " + String(ext));
                      else logln("No extension found.");
                  #endif

                  if (fnameToLower.indexOf(search_pattern) != -1 || search_pattern == "")
                  {
                      bool isDir = isDirectoryPath(entry.c_str()); // entry.isDirectory();
                      bool isHidden = entry[0] == '.';

                      //if (ext == nullptr) ext = ""; // Si no hay extensión, asignamos cadena vacía

                      if (!isDir && !isHidden)
                      {
                          // Compara extensión directamente (más rápido que strstr)
                          if (strcmp(ext, "tap") == 0 || strcmp(ext, "tzx") == 0 ||
                              strcmp(ext, "tsx") == 0 || strcmp(ext, "cdt") == 0 ||
                              strcmp(ext, "wav") == 0 || strcmp(ext, "mp3") == 0 ||
                              strcmp(ext, "flac") == 0 || strcmp(ext, "lst") == 0 ||
                              strcmp(ext, "dsc") == 0 || strcmp(ext, "inf") == 0 ||
                              strcmp(ext, "txt") == 0 || strcmp(ext, "radio") == 0)
                          {
                              fout.print(String(lpos)); fout.print(separator);
                              fout.print("F"); fout.print(separator);
                              fout.print(String(global_dir.position())); fout.print(separator);
                              fout.print(fname); fout.println(separator);
                              cfiles++; lpos++;
                          }
                      }
                      else if (isDir && !isHidden)
                      {
                          fout.print(String(lpos)); fout.print(separator);
                          fout.print("D"); fout.print(separator);
                          fout.print(String(global_dir.position())); fout.print(separator);
                          fout.print(fname); fout.println(separator);
                          cdir++; lpos++;
                      }
                      FILE_TOTAL_FILES = cdir + cfiles;
                      if (itemsToShow >= EACH_FILES_REFRESH)
                      {
                          writeString("statusFILE.txt=\"ITEMS " + String(itemsCount) + "\"");
                          itemsToShow = 0;
                      }
                  }
              }
              //entry.close();
              itemsCount++;
              itemsToShow++;
          }
          //
          fstatus.println("CFIL=" + String(cfiles));
          fstatus.println("CDIR=" + String(cdir));
      }     

      bool createEmptyFile32(char* path)
      {
        File fFile;
        fFile = SD_MMC.open(path, FILE_WRITE);
        if (fFile)
        {
            fFile.close();
            return true;
        }
        else
        {
            fFile.close();
            return false;
        }
      }

      bool removeFileFromLst(const String& lstPath, const String& fileNameToRemove) 
      {
          String tmpPath = lstPath + ".tmp";
          logln("Removing file from list: " + fileNameToRemove + " in " + lstPath);
          logln("Temporary file: " + tmpPath);
          int itemsCount = 0;

          // Abrimos el archivo original para lectura
          File lstFile = SD_MMC.open((lstPath).c_str(), FILE_READ);
          if (!lstFile) 
          {
              logln("Error opening list file.");
              return false;
          }
          lstFile.seek(0);

          // Creamos un archivo temporal para escribir las líneas que no se van a eliminar
          File tmpFile = SD_MMC.open(tmpPath.c_str(), FILE_WRITE);
          if (!tmpFile) {
              lstFile.close();
              logln("Error creating temporary file.");
              return false;
          }

          logln("Processing list file...");

          while (lstFile.available()) 
          {
              String line = lstFile.readStringUntil('\n');
              logln("Processing line: " + line);
              // Extrae el nombre del archivo de la línea
              int idx1 = line.indexOf('|');
              int idx2 = line.indexOf('|', idx1 + 1);
              int idx3 = line.indexOf('|', idx2 + 1);
              int idx4 = line.indexOf('|', idx3 + 1);
            
              String nombre = "";
            
              if (idx1 != -1 && idx2 != -1 && idx3 != -1 && idx4 != -1) {
                  nombre = line.substring(idx3 + 1, idx4);
                  nombre.trim();
              }
              
              // Compara los nombres ignorando mayúsculas/minúsculas
              nombre.toLowerCase();
              String fileNameToDel = fileNameToRemove;
              fileNameToDel.toLowerCase();
              
              // Si no coincide, la copiamos al temporal
              if (nombre != fileNameToDel) {
                  tmpFile.println(line);
              }
              itemsCount++;
          }
          lstFile.close();
          tmpFile.close();

          // Borra el original y renombra el temporal
          if (itemsCount != 0)
          {
            SD_MMC.remove(lstPath);
            SD_MMC.rename(tmpPath, lstPath);
            logln("End removing file from list.");
          }
          else
          {
            SD_MMC.remove(tmpPath);
            logln("No items found in list.");
          }

          return true;
      }

      void registerFiles(const String &path, const String &filename, const String &filename_inf, const String &search_pattern, bool rescan) 
      {
          // Construimos las rutas completas
          String regFile = path + "/" + filename;
          String statusFile = path + "/" + filename_inf;
      
          // Buffers para las rutas
          char file_ch[256];
          char filest_ch[256];
          char path_ch[256];
      
          regFile.toCharArray(file_ch, sizeof(file_ch));
          statusFile.toCharArray(filest_ch, sizeof(filest_ch));
          path.toCharArray(path_ch, sizeof(path_ch));
      
          // Manejadores de archivos
          File f, fstatus;
      
          #ifdef DEBUGMODE
              logln("File created: " + regFile);
              logln("File status created: " + statusFile);
          #endif
  
          // Abrimos los archivos
          f = SD_MMC.open(file_ch, FILE_WRITE);
          fstatus = SD_MMC.open(filest_ch, FILE_WRITE);
  
          if (!f || !fstatus) 
          {
              #ifdef DEBUGMODE
                  logln("Error opening files for writing.");
              #endif

              writeString("statusFILE.txt=\"ERROR\"");
              return;
          }
          else
          {
              // Escribimos la ruta en el archivo de estado
              fstatus.println("PATH=" + path);

              #ifdef DEBUGMODE
                  logln("Registering files in path: " + path);   
              #endif
      
              // Si es una búsqueda y no es un rescan, usamos el archivo existente
              if (!search_pattern.isEmpty() && !rescan && f.size() > 0 && fstatus.size() > 0)
              {
                  fillWithFilesFromFile(f, fstatus, search_pattern, path);
              } 
              else 
              {
                  // Registro normal
                  fillWithFiles(f, fstatus, search_pattern);

                  if (f)
                  {
                    f.close();
                    f = SD_MMC.open(file_ch, FILE_READ);
                    // Ordenamos el archivo _files.lst
                    #ifdef DEBUGMODE
                        logln("Sorting file...");
                    #endif
                    
                    writeString("statusFILE.txt=\"SORTING\"");
                    sortFile(f,SORT_FILES_FIRST_DIR);
                  }
              }
      
              // Cerramos los archivos
              f.close();
              fstatus.close();
        }
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
                // Cogemos la extensión del fichero
                lineData.fileName = String(ptr);
                int dotIdx = lineData.fileName.lastIndexOf('.');
                lineData.fileType = (dotIdx != -1) ? lineData.fileName.substring(dotIdx) : "";
                //logln("File type: " + lineData.fileType);
                break;
              }          
              ptr = strtok(NULL, separator);
              index++;
          }

          return lineData;
      }

      int getSeekFileLST(File &f, int posFileBrowser)
      {
        int seekFile = 0;
        char line[256];
        String sline;
        //
        int n;
        int i=0;        

        if (f)
        {
            // read lines from the file
            while (f.available()) 
            {
                sline = f.readStringUntil('\n');
                sline.toCharArray(line, sizeof(line));
                n = strlen(line);
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

      tFileLST getNextLineInFileLST(File& f)
      {
          char line[256];
          String sline;
          int n = 0;

          tFileLST ld;
          ld.fileName= "";
          ld.ID = -1;
          ld.seek = -1;
          ld.type = '\0';

          if (f)
          {
              if(f.available())
              {
                  sline = f.readStringUntil('\n');
                  sline.toCharArray(line, sizeof(line));
                  n = strlen(line);
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

      void putFilePtrInPosition(File &f, int pos) 
      {
          if (!f) {
              #ifdef DEBUGMODE
                  logln("File is not open.");
              #endif
              return;
          }

          f.seek(0); // Comenzamos desde el principio del fichero

          char line[256];
          String sline;
          int currentLine = 0;

          #ifdef DEBUGMODE
              logln("Moving pointer to position: " + String(pos));
          #endif

          // Saltamos líneas hasta alcanzar la posición deseada
          while (currentLine < pos && f.available()) {
              sline = f.readStringUntil('\n');
              sline.toCharArray(line, sizeof(line));
              currentLine++;
          }

          #ifdef DEBUGMODE
              logln("Pointer moved to position: " + String(currentLine));
          #endif
      }

      void countTotalLinesInLST(const String &path, const String &sourceFile) {
          File fFileINF;
          String fFile = path + "/" +sourceFile;

          // Abrimos el fichero _files.inf
          fFileINF = SD_MMC.open(fFile.c_str(), FILE_READ);
          if (!fFileINF) {
              #ifdef DEBUGMODE
                  logln("Failed to open file: " + fFile);
              #endif
              return;
          }

          fFileINF.seek(0);

          FILE_TOTAL_FILES = 0;
          char line[256];
          String sline;
          int cdir = 0, cfil = 0;

          // Leemos línea por línea
          while (fFileINF.available()) {
              sline = fFileINF.readStringUntil('\n');
              sline.toCharArray(line, sizeof(line));

              if (strncmp(line, "CDIR=", 5) == 0) {
                  cdir = atoi(line + 5); // Extraemos el número después de "CDIR="
              } else if (strncmp(line, "CFIL=", 5) == 0) {
                  cfil = atoi(line + 5); // Extraemos el número después de "CFIL="
              }
          }

          FILE_TOTAL_FILES = cdir + cfil;

          #ifdef DEBUGMODE
              logln("Total directories: " + String(cdir));
              logln("Total files: " + String(cfil));
              logln("Total items: " + String(FILE_TOTAL_FILES));
          #endif

          fFileINF.close();
      }

      void putFilesInList() 
      {
          tFileLST fl;
          int idptr = 1; // Comenzamos desde el índice 1 (el índice 0 es para el directorio padre)

          // Añadimos los archivos al buffer de presentación
          while (idptr < MAX_FILES_TO_LOAD) {
              fl = getNextLineInFileLST(fFileLST);

              // Si llegamos al final del archivo, salimos
              if (fl.ID == -1) {
                  #ifdef DEBUGMODE
                      logln("EOF reached.");
                  #endif
                  break;
              }

              // Asignamos los datos al buffer
              FILES_BUFF[idptr].fileposition = fl.seek;
              FILES_BUFF[idptr].path = fl.fileName;

              if (fl.type == 'D') {
                  FILES_BUFF[idptr].type = "DIR";
                  FILES_BUFF[idptr].isDir = true;
              } else {
                  FILES_BUFF[idptr].isDir = false;
                  FILES_BUFF[idptr].type = fl.fileType;
                  FILES_BUFF[idptr].type.toUpperCase(); // Convertimos el tipo a mayúsculas
              }

              #ifdef DEBUGMODE
                  log("File added: ");
                  log(fl.fileName + " - ");
                  log(fl.fileType + " - ");
                  log(String(fl.ID) + " - ");
                  log(String(fl.seek) + " - ");
                  log(String(fl.type));
                  logln("");
              #endif

              idptr++;
          }
      }

      void getFilesFromSD(bool force_rescan, const String &output_file, const String &output_file_inf, const String &search_pattern = "") 
      {
          // Abrimos el archivo _files.lst
          writeString("statusFILE.txt=\"GET FILES\"");

          // Abrimos ahora el _files.lst, para manejarlo
          String fFileList = FILE_LAST_DIR + "/" + SOURCE_FILE_TO_MANAGE;

          FB_READING_FILES = true;
      
          clearFileBuffer();
      
          #ifdef DEBUGMODE
              logln("Trying to use LST file: " + fFileList);
              logln("Searching files in dir: " + FILE_LAST_DIR);
              // File root = SD_MMC.open("/MP3");
              // while (File entry = root.openNextFile()) {
              //     logln(entry.name());
              //     entry.close();
              // }
              // root.close();              
          #endif
      
          // Aseguramos que el directorio está abierto
          if (global_dir) 
          {
              global_dir.close();
          }

          // Abrimos el directorio
          #ifdef DEBUGMODE
              logln("Trying to open dir: " + FILE_LAST_DIR);
          #endif

          global_dir = SD_MMC.open(FILE_LAST_DIR.c_str(), FILE_READ);

          if (!global_dir || !global_dir.isDirectory())
          {
              #ifdef DEBUGMODE
                  logln("Failed to open directory: " + FILE_LAST_DIR);  
              #endif

              FILE_DIR_OPEN_FAILED = true;
              return;
          }
      
          #ifdef DEBUGMODE
              logln("Directory: " + FILE_LAST_DIR + " - opened successfully.");
          #endif

          FILE_DIR_OPEN_FAILED = false;
      
          // Si el fichero manejador no está abierto, generamos el _files.lst si es necesario
          if (!LST_FILE_IS_OPEN) 
          {

              String pathTmp = "";
              if (FILE_LAST_DIR != "/") {pathTmp = FILE_LAST_DIR + "/" + output_file;}
              
              if (force_rescan || !SD_MMC.exists(pathTmp.c_str())) 
              {
                registerFiles(FILE_LAST_DIR, output_file, output_file_inf, search_pattern, force_rescan);
              }

              if (fFileLST) 
              {
                  fFileLST.close();
              }
              
              fFileLST = SD_MMC.open(fFileList.c_str(), FILE_READ);
              fFileLST.seek(0);
              LST_FILE_IS_OPEN = true;
          }
      
          // Si no estamos buscando, contamos las líneas en el archivo _files.inf
          if (search_pattern == "") {
              countTotalLinesInLST(FILE_LAST_DIR, SOURCE_FILE_INF_TO_MANAGE);
          }
      
          // Añadimos el directorio padre al buffer
          FILES_BUFF[0].isDir = true;
          FILES_BUFF[0].type = "PAR";
          FILES_BUFF[0].path = FILE_PREVIOUS_DIR;
          FILES_BUFF[0].fileposition = global_dir.position();

          // Posicionamos el puntero en el archivo
          if (FILE_PTR_POS != 0) {
              putFilePtrInPosition(fFileLST, FILE_PTR_POS - 1);
          } else {
              putFilePtrInPosition(fFileLST, 0);
          }
      
          // Cargamos los archivos en el buffer
          putFilesInList();
      
          // Cerramos el directorio
          global_dir.close();
          FB_READING_FILES = false;
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
      
      void showInformationAboutFiles()
      {
          // Limpiamos el path, para evitar cargar un fichero no seleccionado.
          writeString("path.txt=\"\"");  
          // Indicamos el path actual
          writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");
          // Actualizamos el total del ficheros leidos anteriormente.
          writeString("statusFILE.txt=\"ITEMS  " + String(FILE_TOTAL_FILES) +"\"");         
          
          // Obtenemos la pagina mostrada del listado de ficheros
          int totalPages = ((FILE_TOTAL_FILES) / TOTAL_FILES_IN_BROWSER_PAGE);
          
          if ((FILE_TOTAL_FILES) % TOTAL_FILES_IN_BROWSER_PAGE != 0)
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
                logln("File [" + String(i) + "] - " + szName + " - type: " + type);
              #endif

              // Convertimos a mayusculas el tipo
              type.toUpperCase();

              // Lo trasladamos a la pantalla
              if (type == "DIR")
              {
                  //Directorio
                  color = DIR_COLOR;  // Amarillo
                  szName = String("<DIR>  ") + szName;
                  szName.toUpperCase();
              }     
              else if (type == ".DSC")
              {
                color = DSC_FILE_COLOR;  // Negro - Indica que es un fichero DSC
              }
              else if (type == ".TAP" || type == ".TZX" || type == ".TSX" || type == ".CDT" || type == ".WAV" || type == ".MP3" || type == ".FLAC" || type == ".RADIO")
              {
                  //Ficheros
                  if (SD_MMC.exists("/fav/" + szName))
                  {
                    // Si estan dentro de /FAV
                    color = FAVORITE_FILE_COLOR;   // Cyan - Indica que esta en favoritos
                  }
                  else
                  {
                    // En general
                    color = DEFAULT_COLOR;   // Blanco
                  }
                  
              }
              else
              {
                  // Resto de ficheros
                  color = OTHER_FILES_COLOR;  // gris apagado
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

      void jumpToDir(String newDir)
      {
          // Cambia al nuevo directorio
          //FILE_BROWSER_SEARCHING = false;  
          FILE_LAST_DIR_LAST = INITFILEPATH;
          FILE_LAST_DIR = newDir;
          FILE_PREVIOUS_DIR = INITFILEPATH;
          // Open root directory
          FILE_PTR_POS = 1;
          IN_THE_SAME_DIR = false;
          getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
          putFilesInScreen();
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
          if (fFileLST)
          {
            fFileLST.close();
          }

          FILE_PTR_POS = 1;
          writeString("statusFILE.txt=\"SEARCHING\""); 
          getFilesFromSD(true,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE,FILE_TXT_TO_SEARCH);
          //putFilesInScreen();
          refreshFiles(); //07/11/2024          
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
              //btStart();
              //LAST_MESSAGE = "Connecting wifi radio";
              //delay(125);
              //LAST_MESSAGE = "Wifi enabled";
              //
              writeString("tape.wifiInd.pic=38");
              writeString("tape0.wifiInd.pic=38");
            }
          }
          else
          {
            WiFi.disconnect(true);
            WiFi.mode(WIFI_OFF);
            //btStop();          
            //LAST_MESSAGE = "Disconnecting wifi radio";
            //delay(2000);
            //LAST_MESSAGE = "Wifi disabled";
            //
            writeString("tape.wifiInd.pic=37");
            writeString("tape0.wifiInd.pic=37");
          }
      }

      void reloadDir(bool force_rescan = true)
      {
          // Recarga el directorio
          FILE_PTR_POS = 1;

          LST_FILE_IS_OPEN = false;
          if (fFileLST)
          {
            fFileLST.close();
          }
          LAST_MESSAGE = "Scanning files ...";
          if (force_rescan) {
              getFilesFromSD(force_rescan,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
          }
          refreshFiles();
      }

      void firstLoadingFilesFB()
      {
          // Carga por primera vez ficheros en el filebrowser
          getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
          putFilesInScreen();        
      }

      void copyFile(File &fSource, File &fTarget)
      {
          size_t n;  
          uint8_t buf[1024];
          int prgCopy = 0;
          int bytesRead = 0; 

          while ((n = fSource.read(buf, sizeof(buf))) > 0) 
          {
            // Escribimos
            fTarget.write(buf, n);
            // Actualizamos indicacion
            bytesRead +=n;
            prgCopy = (bytesRead * 100) / fSource.size();
            // Mostramos
            if (prgCopy % 100 == 0) {
                writeString("statusFILE.txt=\"COPY " + String(prgCopy) + "%\"");
            }

          }        
      }

      void verifyCommand(String strCmd) 
      {
        
        // Reiniciamos el comando
        LAST_COMMAND = "";

        // Selección de bloque desde keypad - pantalla
        if(strCmd.indexOf("RSET") != -1)
        {
          delay(1000);
          ESP.restart();
        }
        else if (strCmd.indexOf("YES") != -1)
        {
            YES = true;
            NO = false;
        }
        else if (strCmd.indexOf("NO") != -1)
        {
            YES = false;
            NO = true;
        }
        else if (strCmd.indexOf("DSD") != -1)
        {
            DISABLE_SD = true;
        }
        // Enable Powerled oscilation
        else if (strCmd.indexOf("PLE=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
            PWM_POWER_LED = true;
          }
          else
          {
            PWM_POWER_LED = false;
          }

          saveHMIcfg("PLEopt");
        }    
        else if (strCmd.indexOf("SFF=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
            SORT_FILES_FIRST_DIR = false;
          }
          else
          {
            SORT_FILES_FIRST_DIR = true;
          }

          saveHMIcfg("SFFopt");
        }             
        else if (strCmd.indexOf("RBUF=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[5];
          //
          if (valEn==1)
          {
            RADIO_BUFFERED = true;
          }
          else
          {
            RADIO_BUFFERED = false;
          }

          logln("Radio buffered: " + String(RADIO_BUFFERED));
          saveHMIcfg("RBUFopt");
        }   
        else if (strCmd.indexOf("DHCP=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[5];
          //
          if (valEn==1)
          {
            DHCP_ENABLE = true;
          }
          else
          {
            DHCP_ENABLE = false;
          }

          logln("DHCP enabled: " + String(DHCP_ENABLE));
          saveHMIcfg("DHCPFopt");
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

              logln("Block selected: " + num);
              logln("Block total: " + String(TOTAL_BLOCKS));

              BLOCK_SELECTED = num.toInt();

              // Esto lo hacemos para poder actualizar la info del bloque
              if (!IRADIO_EN)
              {
                  if (BLOCK_SELECTED > (TOTAL_BLOCKS-2))
                  {
                    BLOCK_SELECTED = TOTAL_BLOCKS - 1;
                  }
                  else if (BLOCK_SELECTED < 1)
                  {
                    BLOCK_SELECTED = 1;
                  }  

                  if (TYPE_FILE_LOAD == "TAP")
                  {
                    BLOCK_SELECTED -= 1;
                  }
              }
              // Espermos unos ms para que de tiempo a la pantalla
              delay(250);

              UPDATE = true;
              START_FFWD_ANIMATION = true;
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


            // Vemos que pagina tenemos abierta
            if (FILE_BROWSER_OPEN)
            {
                // Calculamos el total de páginas
                int totalPages = (FILE_TOTAL_FILES / TOTAL_FILES_IN_BROWSER_PAGE);
                if (FILE_TOTAL_FILES % TOTAL_FILES_IN_BROWSER_PAGE != 0)
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
            else if (BLOCK_BROWSER_OPEN)
            {
                // Calculamos el total de páginas
                int totalPages = ((TOTAL_BLOCKS-1) / MAX_BLOCKS_IN_BROWSER);
                if ((TOTAL_BLOCKS-1) % MAX_BLOCKS_IN_BROWSER != 0)
                {
                    totalPages+=1;
                } 

                // Controlamos que no nos salgamos del total de páginas
                if ((pageSelected+1) <= totalPages)
                {
                    // Cogemos el primer item y refrescamos
                    BB_PTR_ITEM = (pageSelected * MAX_BLOCKS_IN_BROWSER);
                }
                else
                {
                    // Posicionamos entonces en la ultima página
                    pageSelected = totalPages-1;
                    // Cogemos el primer item y refrescamos
                    BB_PTR_ITEM = (pageSelected * MAX_BLOCKS_IN_BROWSER);
                }


                BB_PAGE_SELECTED = pageSelected+1;
                BB_UPDATE = true;
            }
       
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
            ENABLE_ROTATE_FILEBROWSER = false;
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

                // if (global_dir)
                // {
                //     global_dir.close();
                // }

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
            if (!FB_READING_FILES)
            {
              reloadDir();
              REGENERATE_IDX = true;
            }
            else
            {
              // Detenemos la lectura
              FB_CANCEL_READING_FILES = true;
            }
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
            int totalPages = ((FILE_TOTAL_FILES) / TOTAL_FILES_IN_BROWSER_PAGE);
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

            if (FILE_PTR_POS > (FILE_TOTAL_FILES))
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
            if (fFileLST)
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
            String fToFav = FILE_LAST_DIR + "/" + fileName;                         
      
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

                if (SD_MMC.open(favDir))
                {
                    #ifdef DEBUGMODE
                      logAlert("FAV ok");
                    #endif
                    // Si el fichero no existe en favoritos, lo copio
                    logln("Checking if file exists in FAV ..." + pTarget);
                    if (!SD_MMC.exists(pTarget))
                    {
                        if (copyFileFS(pSource.c_str(), pTarget.c_str()))
                        {
                          writeString("statusFILE.txt=\"COPY 100%\"");
                          logAlert("File copied to FAV.");
                        }
                        else
                        {
                          writeString("statusFILE.txt=\"COPY ERROR\"");
                          logAlert("Error! Copy failed");
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
          //
          // *************************************************************************************************
          //
          // NOTA: El tipo de BLOCK BROWSER sea DATA o AUDIO depende del logo visualizado
          //       esto se controla en el HMI. El HMI decide si visualizar el panel para AUDIO o para TAP/TZX
          //
          // *************************************************************************************************
          //
          BB_OPEN = true;
          BLOCK_BROWSER_OPEN = true;
        }
        else if (strCmd.indexOf("BBCL=") != -1)
        {
          // Block browser cerrado con ID seleccionado o -1 para ninguno
          // Con este procedimiento obtenemos la pàgina del filebrowser
          // que se desea visualizar
          int posEq = strCmd.indexOf("=");
          String sbstr = strCmd.substring(posEq+1);

          // Obtenemos el ID seleccionado
          int idsel = sbstr.toInt();
          int blsel = (MAX_BLOCKS_IN_BROWSER * BB_PAGE_SELECTED) - (MAX_BLOCKS_IN_BROWSER - idsel);

          //logln("Bloque seleccionado: " + String(blsel));

          if (blsel >= 0 && blsel <= TOTAL_BLOCKS)
          {
            if (TYPE_FILE_LOAD != "TAP")
            {
              BLOCK_SELECTED = blsel;
            }
            else
            {
              BLOCK_SELECTED = blsel - 1;
            }
            writeString("tape.currentBlock.val=" + String(BLOCK_SELECTED));
          }         
          // Delay para esperar a que el browser se cierre
          delay(250);
          BB_OPEN = false;
          BLOCK_BROWSER_OPEN = false;
          UPDATE_HMI = true;
        }        
        else if (strCmd.indexOf("BDOWN") != -1)
        {
          // Pagina arriba block browser
          BB_PTR_ITEM += MAX_BLOCKS_IN_BROWSER;

          if (BB_PTR_ITEM > TOTAL_BLOCKS - 1)
          {
            BB_PTR_ITEM -= MAX_BLOCKS_IN_BROWSER;
          }

          BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;
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

          BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;
          BB_UPDATE = true;

        }
        else if (strCmd.indexOf("BHOME") != -1)
        {
          // Pagina arriba block browser
          BB_PTR_ITEM = 0;
          BB_PAGE_SELECTED = 1;
          BB_UPDATE = true;
        }        
        else if (strCmd.indexOf("BPDOWN") != -1)
        {
          // Pagina arriba block browser

          // // Pagina arriba block browser
          // BB_PTR_ITEM += MAX_BLOCKS_IN_BROWSER;

          // if (BB_PTR_ITEM > TOTAL_BLOCKS - 1)
          // {
          //   BB_PTR_ITEM -= MAX_BLOCKS_IN_BROWSER;
          // }

          // BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;
          // BB_UPDATE = true;
          
          if (TOTAL_BLOCKS > MAX_BLOCKS_IN_BROWSER)
          {
            // Podemos bajar paginas
            BB_PTR_ITEM += (MAX_BLOCKS_IN_BROWSER * 10);
            // Si sobrepasamos el final, mostramos la ultima pagina
            if (BB_PTR_ITEM > TOTAL_BLOCKS - 1)
            {
              // Ultima pagina.
              int n = TOTAL_BLOCKS / MAX_BLOCKS_IN_BROWSER;
              BB_PTR_ITEM = (TOTAL_BLOCKS - (TOTAL_BLOCKS - (MAX_BLOCKS_IN_BROWSER * n)));
            }
            
            // Info de la pagina vista
            BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;
            BB_UPDATE = true;            
          }
        }
        else if (strCmd.indexOf("BPUP") != -1)
        {
          // Pagina arriba block browser
          BB_PTR_ITEM -= (MAX_BLOCKS_IN_BROWSER * 10);

          if (BB_PTR_ITEM < 0)
          {
            BB_PTR_ITEM = 0;
          }

          BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;
          BB_UPDATE = true;

        }        
        else if (strCmd.indexOf("CHD=") != -1) 
        {
            uint32_t val=0;
            // Con este comando capturamos el directorio a cambiar
            // uint8_t buff[8];
            // strCmd.getBytes(buff, 7);

            // #ifdef DEBUGMODE
            //   for (int i=0;i<8;i++)
            //   {
            //     logln("Byte " + String(i) + ": " + String(buff[i]));
            //   }
            // #endif
            
            // long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));

            // Con este comando capturamos el directorio a cambiar
            uint8_t buff[8] = {0}; // Inicializamos el buffer a 0
            strCmd.getBytes(buff, 7);  // getBytes no devuelve size_t
            
            // Verificación de seguridad - verificamos directamente los bytes del comando
            if (buff[0] == 'C' && buff[1] == 'H' && buff[2] == 'D' && buff[3] == '=') {
                // Construimos el valor usando máscaras y shifts para evitar desbordamientos
                val = ((uint32_t)buff[4]) | 
                              ((uint32_t)buff[5] << 8) | 
                              ((uint32_t)buff[6] << 16);
                
                // Verificación adicional del rango
                if (val < MAX_FILES_TO_LOAD) {
                    int indexFileSelected = static_cast<int>(val);
                    FILE_IDX_SELECTED = indexFileSelected;
                    // ... resto del código existente ...
                } else {
                    logln("Error: Invalid file index value: " + String(val));
                    writeString("currentDir.txt=\">> Error: Invalid index value <<\"");
                    return;
                }
            } else {
                logln("Error: Invalid CHD command format");
                writeString("currentDir.txt=\">> Error: Invalid command format <<\"");
                return;
            }

            //String num = String(val);
            int indexFileSelected = static_cast<int> (val);
      
            //Cogemos el directorio
            if (indexFileSelected > sizeof(FILES_BUFF))
            {
              // Regeneramos el directorio de manera automática.
              logln("File index out of range: " + String(indexFileSelected));
              //
              writeString("currentDir.txt=\">> HMI error. Idx out of range <<\"");
              //FILE_IDX_SELECTED = 13;
              //reloadDir();
            }
            else
            {
              // Actuamos sobre la seleccion en HMI
              FILE_IDX_SELECTED = indexFileSelected;   
              //      
              
              if (FILE_LAST_DIR == "/")
              {
                FILE_DIR_TO_CHANGE = FILE_LAST_DIR + FILES_BUFF[FILE_IDX_SELECTED+1].path;
              }
              else
              {
                FILE_DIR_TO_CHANGE = FILE_LAST_DIR + "/" + FILES_BUFF[FILE_IDX_SELECTED+1].path;
              }

              #ifdef DEBUGMODE
                logln("Dir to change " + FILE_DIR_TO_CHANGE);
              #endif
              
              IN_THE_SAME_DIR = false;
        
              // Guardamos para poder volver atrás
              String dir_ch = FILE_DIR_TO_CHANGE;
              if (dir_ch.endsWith("/") && dir_ch.length() > 1) dir_ch.remove(dir_ch.length() - 1);

              //
              logln("Changing to dir: " + dir_ch);

              //
              FILE_PREVIOUS_DIR = FILE_LAST_DIR;
              FILE_LAST_DIR = dir_ch;
              FILE_LAST_DIR_LAST = FILE_LAST_DIR;
        
              FILE_PTR_POS = 1;
              // clearFilesInScreen(); // 02/12/2024
              writeString("statusFILE.txt=\"READING\"");  

              //logln("Paso 1");
              if (fFileLST)
              {
                fFileLST.close();
                LST_FILE_IS_OPEN = false;
              }

              //logln("Paso 2");

              String recDirTmp = FILE_LAST_DIR;
              recDirTmp.toUpperCase();
              if (recDirTmp == "/FAV/" || recDirTmp == "/REC/" || recDirTmp == "/WAV")
              {
                getFilesFromSD(true,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);

              }
              else
              {
                getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
              }

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
     
            //clearRotate();

        }
        else if (strCmd.indexOf("PAR=") != -1) 
        {
            // Con este comando capturamos el directorio padre
            String oldDir = FILE_PREVIOUS_DIR;
            
            LST_FILE_IS_OPEN = false;
            if (fFileLST)
            {
              fFileLST.close();
            }

            // Forzamos a leer el repositorio de ficheros del directorio.
            SOURCE_FILE_TO_MANAGE = "_files.lst"; 
            SOURCE_FILE_INF_TO_MANAGE = "_files.inf"; 

            //Cogemos el directorio padre que siempre estará en el prevDir y por tanto
            //no hay que calcular la posición
            FILE_DIR_TO_CHANGE = FILES_BUFF[0].path;    
            if (FILE_DIR_TO_CHANGE.endsWith("/") && FILE_DIR_TO_CHANGE.length() > 1) FILE_DIR_TO_CHANGE.remove(FILE_DIR_TO_CHANGE.length() - 1);

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

            //clearRotate();

      
        }
        else if (strCmd.indexOf("ROT=") != -1) 
        {
            // Con este comando capturamos el fichero a rotar
            uint8_t buff[8];
            strCmd.getBytes(buff, 7);
            long val = (long)((int)buff[4] + (256*(int)buff[5]) + (65536*(int)buff[6]));
            String num = String(val);
      
            int idx = num.toInt();  
            logln("Rotating index: " + String(idx));

            if (idx >=0 && idx < 14)
            {
              ROTATE_FILENAME = FILES_BUFF[idx+1].path;
              ENABLE_ROTATE_FILEBROWSER = true;  
            }
            else
            {
              ROTATE_FILENAME = "";
              ENABLE_ROTATE_FILEBROWSER = false;
            }

            logln("Rotating filename: " + ROTATE_FILENAME);
            logln("Rotating enable: " + String(ENABLE_ROTATE_FILEBROWSER));

            
            // Esto lo hacemos para que no se quede pillado en el ultimo cuando ha hecho un rotate
            //writeString("file.lastPressed.val=15");
           
        }
        else if (strCmd.indexOf("ROC=") != -1)
        {
          ROTATE_FILENAME = "";
          ENABLE_ROTATE_FILEBROWSER = false;          
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
      
            FILE_TO_DELETE = FILE_LAST_DIR + "/" + FILES_BUFF[FILE_IDX_SELECTED+1].path;      
            FILE_SELECTED_DELETE = true; 


            if (FILE_SELECTED_DELETE)
            {
              // Lo Borramos
                File mf = SD_MMC.open(FILE_TO_DELETE,FILE_WRITE);

                if (!mf)
                {
                  writeString("currentDir.txt=\">> Error. File not writeable <<\"");
                  delay(1500);
                  writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");                  
                  mf.close();
                  FILE_SELECTED_DELETE = false;
                }
                else
                {
                    mf.close();

                    if (!SD_MMC.remove(FILE_TO_DELETE))
                    {
                      #ifdef DEBUGMODE
                        logln("Error to remove file. " + FILE_TO_DELETE);
                      #endif
                      
                      logln("File not removed. " + FILE_TO_DELETE);

                      writeString("currentDir.txt=\">> Error. File not removed <<\"");
                      delay(1500);
                      writeString("currentDir.txt=\"" + String(FILE_LAST_DIR_LAST) + "\"");    
                      FILE_SELECTED_DELETE = false;              
                    }
                    else
                    {
                      FILE_SELECTED_DELETE = false;
                      logln("File remove. " + FILE_TO_DELETE);
                    }                  
                }

                // Tras borrar hacemos un rescan
                logln("Rescanning files ... in dir: " + FILE_LAST_DIR_LAST + "/_files.lst");
                //removeFileFromLst(FILE_LAST_DIR_LAST + "/_files.lst", FILES_BUFF[FILE_IDX_SELECTED+1].path);
                reloadDir();
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

            logln("File selected: " + String(FILE_IDX_SELECTED));

            // Path completo del fichero
            if (FILE_IDX_SELECTED >=0 && FILE_IDX_SELECTED < 14)
            {
              PATH_FILE_TO_LOAD = FILE_LAST_DIR + "/" + FILES_BUFF[FILE_IDX_SELECTED+1].path;  
              // Fichero sin path
              FILE_LOAD = FILES_BUFF[FILE_IDX_SELECTED+1].path;                

              logln("File path to load: " + PATH_FILE_TO_LOAD);
              logln("File name only   : " + FILE_LOAD);

              // Comprobamos que la ruta es existente
              if (!SD_MMC.exists(PATH_FILE_TO_LOAD))
              {
                #ifdef DEBUGMODE
                  logAlert("File not exists: " + PATH_FILE_TO_LOAD);
                #endif
                PATH_FILE_TO_LOAD = "";
                FILE_LOAD = "";
                FILE_SELECTED = false;
              }
              else
              {
                #ifdef DEBUGMODE
                  logAlert("File exists: " + PATH_FILE_TO_LOAD);
                #endif
                FILE_SELECTED = true;
              }
            }
            else
            {
              PATH_FILE_TO_LOAD = "";
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
            KEEP_FFWIND = false;
        }
        else if (strCmd.indexOf("RWD") != -1) 
        {
            logln("RWD pressed");
            FFWIND = false;
            RWIND = true;
            KEEP_RWIND = false;
        }
        else if (strCmd.indexOf("SFWD") != -1) 
        {
            logln("S_FFWD pressed");
            FFWIND = false;
            RWIND = false;
            KEEP_FFWIND = true;
        }
        else if (strCmd.indexOf("TTWD") != -1) 
        {
            logln("S_RWD pressed");
            FFWIND = false;
            RWIND = false;
            KEEP_RWIND = true;
        }        
        else if (strCmd.indexOf("PLAY") != -1) 
        {

          #ifdef DEBUGMODE
            logAlert("PLAY pressed.");
          #endif

          if(!WF_UPLOAD_TO_SD)
          {
            PLAY = true;
            PAUSE = false;
            STOP = false;
            REC = false;
            EJECT = false;
            ABORT = false;
  
            BTN_PLAY_PRESSED = true;  
          }
          else
          {
            LAST_MESSAGE = "Wait to finish the uploading process.";
          }

          // *********************************************************************
          //
          // Solo esta de prueba. ELIMINAR!!!!
          //
          writeString("BBOK.val=1");
          //
          // *********************************************************************


        }   
        else if (strCmd.indexOf("REC") != -1) 
        {

          #ifdef DEBUGMODE
            logAlert("REC pressed.");
          #endif

          if (IRADIO_EN)
          {
            REC = true;
          }
          else
          {
            PLAY = false;
            PAUSE = false;
            STOP = false;
            REC = true;
            ABORT = false;
            EJECT = false;
          }
          
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

          // Si no se esta subiendo nada a la SD desde WiFi podemos abrir
          if (!WF_UPLOAD_TO_SD && !STOP_OR_PAUSE_REQUEST)
          {
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
                  // writeXSTR(66,247,342,16,2,65535,0,1,1,50,LAST_MESSAGE);
                  delay(125);
                  clearInformationFile();
                  delay(125);
              }

              FILE_BROWSER_OPEN = true;
              //
              if (myNex.readNumber("screen.source.val") == 1)
              {
                // Si estamos en el tape pasamos a tape0
                delay(300);
                writeString("page tape0");
              }
              delay(500);          
              // Entramos en el file browser
              writeString("page file");          
              delay(125);
              refreshFiles();
          }
          else
          {
            if (WF_UPLOAD_TO_SD)
            {
              LAST_MESSAGE = "Wait to finish the uploading process.";
            }
            else if (STOP_OR_PAUSE_REQUEST)
            {
              LAST_MESSAGE = "Wait for tape stop or pause before ejecting.";
            }
          }
        }    
        else if (strCmd.indexOf("VLI=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          if (valVol == 1)
          {
              VOL_LIMIT_HEADPHONE = true;

              if (MAIN_VOL > MAX_VOL_FOR_HEADPHONE_LIMIT)
              {
                MAIN_VOL = MAX_VOL_FOR_HEADPHONE_LIMIT;
              }

              if (MAIN_VOL_R > MAX_VOL_FOR_HEADPHONE_LIMIT)
              {
                MAIN_VOL_R = MAX_VOL_FOR_HEADPHONE_LIMIT;
              }     

              if (MAIN_VOL_L > MAX_VOL_FOR_HEADPHONE_LIMIT)
              {
                MAIN_VOL_L = MAX_VOL_FOR_HEADPHONE_LIMIT;
              }
          }
          else
          {
              // 
              VOL_LIMIT_HEADPHONE = false;
              // 
              MAIN_VOL = myNex.readNumber("menuAudio.volM.val");
              MAIN_VOL_R = myNex.readNumber("menuAudio.volR.val");
              MAIN_VOL_L = myNex.readNumber("menuAudio.volL.val");
          }
          
          // Actualizamos el master slide
          myNex.writeNum("menuAudio.volM.val", int(MAIN_VOL));
          myNex.writeNum("menuAudio.volLevelM.val", int(MAIN_VOL));
          myNex.writeStr("tape.tapeVol.txt",String(int(MAIN_VOL)) + "%");
          
          // Almacenamos en NVS
          saveHMIcfg("VLIopt");
          saveVolSliders();

          kitStream.setVolume(MAIN_VOL / 100);

        }
        // Ajuste del volumen
        else if (strCmd.indexOf("VOL=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];
          MAIN_VOL = valVol;

          if (VOL_LIMIT_HEADPHONE)
          {
              if (MAIN_VOL > MAX_VOL_FOR_HEADPHONE_LIMIT)
              {
                MAIN_VOL = MAX_VOL_FOR_HEADPHONE_LIMIT;
              }                 
          }
          MASTER_VOL = valVol;
          saveHMIcfg("VOLMopt");
          myNex.writeStr("tape.tapeVol.txt",String(int(MAIN_VOL)) + "%");

          kitStream.setVolume(MAIN_VOL / 100);
          
        }
        // Ajuste el vol canal R
        else if (strCmd.indexOf("VRR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          MAIN_VOL_R = valVol;
          if (VOL_LIMIT_HEADPHONE)
          {
            if (MAIN_VOL_R > MAX_VOL_FOR_HEADPHONE_LIMIT)
            {
              MAIN_VOL_R = MAX_VOL_FOR_HEADPHONE_LIMIT;
            }
          }          

          saveVolSliders();

        }
        // Ajuste el vol canal L
        else if (strCmd.indexOf("VLL=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];
          MAIN_VOL_L = valVol;
          
          if (VOL_LIMIT_HEADPHONE)
          {
            if (MAIN_VOL_L > MAX_VOL_FOR_HEADPHONE_LIMIT)
            {
              MAIN_VOL_L = MAX_VOL_FOR_HEADPHONE_LIMIT;
            }
          }


          // Ajustamos el volumen
          #ifdef DEBUGMODE
            logln("");
            logln("L-Channel volume value=" + String(MAIN_VOL_L));
            logln("");
          #endif

          saveVolSliders();

        }
        else if (strCmd.indexOf("EQH=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          EQ_HIGH = valVol/100.0;      
          EQ_CHANGE = true;    
          //logln("EQ HIGH: " + String(EQ_HIGH)); 
          saveHMIcfg("EQHopt");
        } 
        else if (strCmd.indexOf("EQM=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          EQ_MID = valVol/100.0;          
          EQ_CHANGE = true;    
          //logln("EQ MID: " + String(EQ_MID)); 
          saveHMIcfg("EQMopt");
        }   
        else if (strCmd.indexOf("EQL=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          EQ_LOW = valVol/100.0;          
          EQ_CHANGE = true;   
          //logln("EQ LOW: " + String(EQ_LOW)); 
          saveHMIcfg("EQLopt");
        }      
        else if (strCmd.indexOf("TON=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valVol = (int)buff[4];

          // Le cambiamos el signo para que cuando seleccionemos -1 sea bajar tono en vez de quitar una muestra que es todo lo contrario (aumentar frecuencia)
          // y lo mismo con el +1
          AZIMUT = valVol;
          TONE_ADJUST = (-210)*(TONE_ADJUSTMENT_ZX_SPECTRUM + (valVol-TONE_ADJUSTMENT_ZX_SPECTRUM_LIMIT));         
          logln("TONE: " + String(TONE_ADJUST) + " Hz"); 
          //saveHMIcfg("EQLopt");
        }    
        else if (strCmd.indexOf("WWW") != -1) 
        {
          // Activa audio internet.
          // logln("Audio internet enabled.");
          // IRADIO_EN = true;
        }
        else if (strCmd.indexOf("RADI") != -1) 
        {
          // Salta al directorio de RADIO internet.
          logln("Jumping to internet radio dir.");
          IRADIO_EN = true;
          // Con este comando nos indica la pantalla que quiere
          // le devolvamos ficheros en la posición actual del puntero
          SOURCE_FILE_TO_MANAGE = "_files.lst";
          SOURCE_FILE_INF_TO_MANAGE = "_files.inf"; 

          LST_FILE_IS_OPEN = false;
          if (fFileLST)
          {
            fFileLST.close();
          }

          jumpToDir("/RADIO");
        
          getFilesFromSD(false,SOURCE_FILE_TO_MANAGE,SOURCE_FILE_INF_TO_MANAGE);
          refreshFiles();            
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
        else if (strCmd.indexOf("AMP=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valAmp = (int)buff[4];
          SCHMITT_AMP = valAmp;
          logln("Amplification value=" + String(SCHMITT_AMP));
        } 
        else if (strCmd.indexOf("IVO=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valInVol = (int)buff[4];
          IN_REC_VOL = valInVol/100.0; // Valor en %
          logln("Input rec gain=" + String(IN_REC_VOL));
          // Ajustamos el input gain
          kitStream.setInputVolume(IN_REC_VOL); // Ajustamos el volumen de entrada al 50%          
        } 
        //IN_REC_VOL
        // else if (strCmd.indexOf("OFS=") != -1) 
        // {
        //   //Cogemos el valor
        //   PULSE_OFFSET = myNex.readNumber("menuAudio3.offset.val");;
        //   logln("Offset value=" + String(PULSE_OFFSET));
        // }                        
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
            //logln("");
            //logln("Terminadores =" + String(APPLY_END));
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
          INVERSETRAIN = valEn;
          // if (valEn==0)
          // {
          //     // Empieza en DOWN
          //     POLARIZATION = up;
          //     EDGE_EAR_IS = up;
          //     //INVERSETRAIN = false;
          // }
          // else
          // {
          //     // Empieza en UP
          //     POLARIZATION = down;
          //     EDGE_EAR_IS = down;
          //     //INVERSETRAIN = true;
          // }

          //logln("");
          //logln("Polarization =" + String(INVERSETRAIN));

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
          
          //logln("");
          //logln("Down level is ZERO =" + String(ZEROLEVEL));

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
          logln("Threshold enable=" + String(EN_SCHMITT_CHANGE));

        }
        // Enable MIC inversion
        else if (strCmd.indexOf("EIN=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              EN_EAR_INVERSION = true;
          }
          else
          {
            EN_EAR_INVERSION = false;
          }

          logln("");
          logln("Enable EAR inversion=" + String(EN_SCHMITT_CHANGE));

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

          // Almacenamos en NVS
          saveHMIcfg("MAMopt");

          if (ACTIVE_AMP)
          {
            // Bajamos el volumen del speaker que esta en el canal amplificado IZQ
            MAIN_VOL_L = 5;
            // Actualizamos el HMI
            writeString("menuAudio.volL.val=" + String(MAIN_VOL_L));
            writeString("menuAudio.volLevel.val=" + String(MAIN_VOL_L));
          }

          // Habilitamos el amplificador de salida
          kitStream.setPAPower(ACTIVE_AMP);
          logln("Active amp=" + String(ACTIVE_AMP));
          AMP_CHANGE = true;
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

          // Almacenamos en NVS
          saveHMIcfg("STEopt");
          // Almacenamos en NVS tambien el mute
          saveHMIcfg("MAMopt");
          //logln("Mute enable=" + String(EN_STEREO));
        }
        // Habilita el Speaker
        else if (strCmd.indexOf("SPK=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              EN_SPEAKER = true;
              // Activamos el amplificador
              kitStream.setPAPower(true);
              writeString("menuAudio.muteAmp.val=0");
              writeString("menuAudio.muteAmp.val=0");
          }
          else
          {
              EN_SPEAKER = false;
              //kitStream.setSpeakerActive(false);
          }

          // Almacenamos en NVS
          saveHMIcfg("SPKopt");
          SPK_CHANGE = true;
          //logln("Speaker enable=" + String(EN_SPEAKER));
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
          //logln("MIC LEFT enable=" + String(SWAP_MIC_CHANNEL));
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
          kitStream.setPAPower(ACTIVE_AMP);
;
          //logln("EAR LEFT enable=" + String(SWAP_EAR_CHANNEL));
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

          File cfg;

          //logln("");
          //logln("Saving configuration in " + String(strpath));
          cfg = SD_MMC.open(strpath, FILE_WRITE);

          if (cfg)
          {
            // Creamos el fichero de configuracion.
 
            //logln("");
            //logln("file created - ");

            // Ahora escribimos la configuracion
            cfg.println("<freq>"+ String(SAMPLING_RATE) +"</freq>");        
            cfg.println("<zerolevel>" + String(ZEROLEVEL) + "</zerolevel>");
            cfg.println("<blockend>" + String(APPLY_END) + "</blockend>");   

            if (INVERSETRAIN)
            {
               cfg.println("<polarized>1</polarized>");       
            } 
            else
            {
              cfg.println("<polarized>0</polarized>");
            }
            cfg.println("<azimut>" + String(AZIMUT) + "</azimut>");

          }
          
          cfg.close();

          #ifdef DEBUGMODE
            log("Config. saved");
          #endif
        }
        // Sampling rate TZX
        else if (strCmd.indexOf("SAM=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==0)
          {
              BASE_SR = 48000;
              //writeString("tape.lblFreq.txt=\"48KHz\"" );
          }
          else if(valEn==1)
          {
              BASE_SR = 44100;
              //writeString("tape.lblFreq.txt=\"44KHz\"" );
          }
          else if(valEn==2)
          {
              BASE_SR = 32000;
              //writeString("tape.lblFreq.txt=\"32KHz\"" );
          }
          else if(valEn==3)
          {
              BASE_SR = STANDARD_SR_8_BIT_MACHINE;
              //writeString("tape.lblFreq.txt=\"22KHz\"" );
          }
          //
          writeString("tape.lblFreq.txt=\"" + String(int(BASE_SR/1000)) + "KHz\"" );
          writeString("tape0.lblFreq.txt=\"" + String(int(BASE_SR/1000)) + "KHz\"" );
         
          // Cambiamos el sampling rate
          // CAmbiamos el sampling rate del hardware de salida
          // auto cfg = akit.defaultConfig();
          // cfg.sample_rate = SAMPLING_RATE;
          // akit.setAudioInfo(cfg);

          auto cfg2 = kitStream.audioInfo();
          cfg2.sample_rate = SAMPLING_RATE;
          kitStream.setAudioInfo(cfg2);

          #ifdef DEBUGMODE
            log("Sampling rate =" + String(SAMPLING_RATE));
          #endif
        }
        else if (strCmd.indexOf("CFR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              WAV_SAMPLING_RATE = myNex.readNumber("menuAudio4.va0.val");
              //logln("Custom sampling rate: " + String(WAV_SAMPLING_RATE));
          }

          WAV_UPDATE_SR = true;                         
          
          #ifdef DEBUGMODE
            log("Customo WAV Sampling rate =" + String(WAV_SAMPLING_RATE));
          #endif
        }         
        else if (strCmd.indexOf("WSR=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==0)
          {
              WAV_SAMPLING_RATE = 48000;
              // writeString("tape.lblFreq.txt=\"48KHz\"" );
          }
          else if(valEn==1)
          {
              WAV_SAMPLING_RATE = 44100;
              // writeString("tape.lblFreq.txt=\"44KHz\"" );
          }
          else if(valEn==2)
          {
              WAV_SAMPLING_RATE = 32000;
              // writeString("tape.lblFreq.txt=\"32KHz\"" );
          }
          else if(valEn==3)
          {
              WAV_SAMPLING_RATE = 22050;
              // writeString("tape.lblFreq.txt=\"22KHz\"" );
          }
          else if(valEn==4)
          {
              WAV_SAMPLING_RATE = 11025;
              // writeString("tape.lblFreq.txt=\"22KHz\"" );
          }           
          
          WAV_UPDATE_SR = true;
          
          #ifdef DEBUGMODE
            log("WAV Sampling rate =" + String(WAV_SAMPLING_RATE));
          #endif
        }  
        else if (strCmd.indexOf("WMO=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Habilitar MONO
              WAV_CHAN = 1;
          }
          else
          {
              // Habilitar STEREO
              WAV_CHAN = 2;
          }
          
          WAV_UPDATE_CH = true;

          #ifdef DEBUGMODE
            logln("WAV chanels =" + String(WAV_CHAN));
          #endif

        }     
        else if (strCmd.indexOf("W8B=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Habilita terminadores
              WAV_BITS_PER_SAMPLE = 8;
          }
          else
          {
              // Deshabilita terminadores
              WAV_BITS_PER_SAMPLE = 16;
          }

          WAV_UPDATE_BS = true;

          #ifdef DEBUGMODE
            logln("WAV bits =" + String(WAV_BITS_PER_SAMPLE));
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
              // Habilita
              MODEWAV = true;
              // Ambas no pueden estar habilitadas
              PLAY_TO_WAV_FILE = false;
          }
          else
          {
              // Deshabilita
              MODEWAV = false;
          }

          #ifdef DEBUGMODE
            logln("Modo WAV =" + String(MODEWAV));
          #endif
        }
        // Habilitar play to WAV file file
        else if (strCmd.indexOf("PTW=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Habilita
              OUT_TO_WAV = true;
              PLAY_TO_WAV_FILE = true;
          }
          else
          {
              // Deshabilita
              OUT_TO_WAV = false;
              PLAY_TO_WAV_FILE = false;
          }

          #ifdef DEBUGMODE
            logln("Modo Out to WAV =" + String(OUT_TO_WAV));
          #endif
        }
        // Habilitar play to WAV file file
        else if (strCmd.indexOf("WA8=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          if (valEn==1)
          {
              // Habilita
              WAV_8BIT_MONO = true;
          }
          else
          {
              // Deshabilita
              WAV_8BIT_MONO = false;
          }

          #ifdef DEBUGMODE
            logln("Modo WAV mono 8-bits =" + String(WAV_8BIT_MONO));
          #endif
        }
        // Deshabilitar Auto Stop en WAV and MP3 player.
        else if (strCmd.indexOf("DPS=") != -1) 
        {
          //Cogemos el valor
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int valEn = (int)buff[4];
          //
          disable_auto_media_stop = !disable_auto_media_stop;
          
          logln("DPS: " + String(disable_auto_media_stop));

          if (!disable_auto_media_stop)
          {
            // Play auto-stop
            writeString("tape.playType.txt=\"*\"");
            writeString("tape0.playType.txt=\"*\"");
            writeString("tape.playType.y=3");
            writeString("tape0.playType.y=3");
            writeString("doevents");
          }
          else
          {
            // Play continuo
            writeString("tape.playType.txt=\"C\"");
            writeString("tape0.playType.txt=\"C\"");
            writeString("tape.playType.y=6");
            writeString("tape0.playType.y=6");
            writeString("doevents");
          }          

          #ifdef DEBUGMODE
            logln("Modo Auto STOP player =" + String(OUT_TO_WAV));
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
          //logln("SHOW_DATA_DEBUG enable=" + String(SHOW_DATA_DEBUG));
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
          //logln("MP1=" + String(MIN_SYNC));
        }
        else if (strCmd.indexOf("MP2=") != -1) 
        {
          //maxSync1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_SYNC=val;
          //logln("MP2=" + String(MAX_SYNC));
        }
        else if (strCmd.indexOf("MP3=") != -1) 
        {
          //minBit0
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_BIT0=val;
          //logln("MP3=" + String(MIN_BIT0));
        }
        else if (strCmd.indexOf("MP4=") != -1) 
        {
          //maxBit0
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_BIT0=val;
          //logln("MP4=" + String(MAX_BIT0));
        }
        else if (strCmd.indexOf("MP5=") != -1) 
        {
          //minBit1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_BIT1=val;
          //logln("MP5=" + String(MIN_BIT1));
        }
        else if (strCmd.indexOf("MP6=") != -1) 
        {
          //maxBit1
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MAX_BIT1=val;
          //logln("MP6=" + String(MAX_BIT1));
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
          //logln("MP7=" + String(MAX_PULSES_LEAD));
        }
        else if (strCmd.indexOf("MP8=") != -1) 
        {
          //minLead
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[4];
          //
          MIN_LEAD=val;
          //logln("MP8=" + String(MIN_LEAD));
        }
        else if (strCmd.indexOf("MP9=") != -1) 
        {
          //maxLead
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = buff[4];
          //
          MAX_LEAD=val;
          //logln("MP9=" + String(MAX_LEAD));
        }
        // Control de volumen por botones - MASTER
        else if (strCmd.indexOf("VOLUP") != -1) 
        {
          MAIN_VOL += 1;
          
          if (VOL_LIMIT_HEADPHONE)
          {
            if (MAIN_VOL > MAX_VOL_FOR_HEADPHONE_LIMIT)
            {
              MAIN_VOL = MAX_VOL_FOR_HEADPHONE_LIMIT;
            }
          }
          else
          {
            if (MAIN_VOL > 100)
            {
              MAIN_VOL = 100;
            }
          }

          myNex.writeNum("menuAudio.volM.val", int(MAIN_VOL));
          myNex.writeNum("menuAudio.volLevelM.val", int(MAIN_VOL));
          myNex.writeStr("tape.tapeVol.txt",String(int(MAIN_VOL)) + "%");

          kitStream.setVolume(MAIN_VOL / 100);

        }        
        else if (strCmd.indexOf("VOLDW") != -1) 
        {
          MAIN_VOL -= 1;
          
          if (MAIN_VOL < 0)
          {
            MAIN_VOL = 0;
          }

          myNex.writeNum("menuAudio.volM.val", int(MAIN_VOL));
          myNex.writeNum("menuAudio.volLevelM.val", int(MAIN_VOL));
          myNex.writeStr("tape.tapeVol.txt",String(int(MAIN_VOL)) + "%");


          kitStream.setVolume(MAIN_VOL / 100);
          // logln("");
          // logln("VOL DOWN");
          // logln("");
        }
        else if (strCmd.indexOf("TONE") != -1) 
        {
            SAMPLINGTEST = true;
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
            delay(500);
            // Actualizamos estado checkbox
            myNex.writeNum("menu.dhcp.val", int(DHCP_ENABLE));
            myNex.writeNum("menu.ppled.val", int(PWM_POWER_LED));
            myNex.writeNum("menu.sortFil.val", int(SORT_FILES_FIRST_DIR));
            myNex.writeNum("menu.rbuf.val", int(RADIO_BUFFERED));
        }  
        else if (strCmd.indexOf("PMENU0") != -1)
        {
            // Estamos en la pantalla MENU
            writeString("mainmenu.verFirmware.txt=\" powadcr " + String(VERSION) + "\"");     
        }  
        else if (strCmd.indexOf("PMENU3") != -1)
        {
        }  
        else if (strCmd.indexOf("PMENU4") != -1)
        {
        }         
        else if (strCmd.indexOf("PMENU5") != -1)
        {
        }         
        else if (strCmd.indexOf("PTAPE") != -1)
        {
            // Estamos en la pantalla TAPE
            #ifdef DEBUGMODE
              logAlert("PAGE TAPE");
            #endif
            CURRENT_PAGE = 1;
            updateInformationMainPage(true);
            TAPE_PAGE_SHOWN = true;
        }
        else if (strCmd.indexOf("CHKUPD") != -1)
        {
          //-------------------------------------------------------------------------
          //
          // Buscamos nueva firmware
          //
          //-------------------------------------------------------------------------
          #ifdef AUTO_UPDATE    
            if (WIFI_CONNECTED) 
            {
              logln("Manual firmware update requested");
              myNex.writeStr("update.status.txt","Please wait...");
              manualFirmwareUpdate();        
            } else {
              myNex.writeStr("update.status.txt","WiFi required for updates");
            }
          #endif          
        }    
        else if (strCmd.indexOf("IDSC=") != -1) 
        {
          uint8_t buff[8];
          strCmd.getBytes(buff, 7);
          int val = (int)buff[5];
          //
          if (val==1)
          {IGNORE_DSC = true;}
          else  
          {IGNORE_DSC = false;}
        }        
        else if (strCmd.indexOf("UPDATE") != -1)
        {
          //-------------------------------------------------------------------------
          //
          // Buscamos nueva firmware
          //
          //-------------------------------------------------------------------------
          #ifdef AUTO_UPDATE    
            // -------------------------------------------------------------------------
            // Actualizacion del HMI
            // -------------------------------------------------------------------------
            updateHMIfirmware();
            delay(5000);
            // -------------------------------------------------------------------------
            // Actualización OTA por SD
            // -------------------------------------------------------------------------
            updateESP32firmware();
            myNex.writeStr("update.status.txt","Rebooting powaDCR ...");
            delay(2000);
            ESP.restart();
          #endif          
        }           
        else
        {}

      //Fin del procedimiento
      }        

      void writeString(String stringData) 
      {
      
          // Esperamos a que todos los datos salientes se hayan enviado
          SerialHW.flush();
          // Enviamos el comando de inicio de datos
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
          //
          SerialHW.flush();

      }
      
      void writeStringBlock(String stringData) 
      {
          
          // Esperamos a que todos los datos salientes se hayan enviado
          SerialHW.flush();
          //
          for (int i = 0; i < stringData.length(); i++) 
          {
            // Enviamos los datos
            SerialHW.write(stringData[i]);
          }
          //
          SerialHW.flush();
      }      

      void write(String stringData)
      {
          SerialHW.flush();
          //
          for (int i = 0; i < stringData.length(); i++) 
          {
            // Enviamos los datos
            SerialHW.write(stringData[i]);
          }
          //
          SerialHW.flush();
      }

      void sendbin(char *data)
      {
          SerialHW.flush();
          for (int i = 0; i < sizeof(data); i++) 
          {
            // Enviamos los datos
            SerialHW.write(data[i]);
          }
          SerialHW.flush();
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

      void writeXSTR(int x, int y, int w, int h, int font, int ink, int bckColor, int xcenter, int ycenter, int maxChar, String txt)
      {       
          // usage: xstr <x>,<y>,<w>,<h>,<font>,<pco>,<bco>,<xcen>,<ycen>,<sta>,<text>
          // 
          // <x> is the x coordinate of upper left corner of defined text area
          // <y> is the y coordinate of upper left corner of defined text area
          // <w> is the width of the defined text area
          // <h> is the height of the defined text area
          // <font> is the number of the Resource Font
          // <pco> is the foreground color of text (Color Constant or 565 color value)
          // <bco> is a) background color of text, or b) picid if <sta> is set to 0 or 2
          // <xcen> is the Horizontal Alignment (0 – left, 1 – centered, 2 – right)
          // <ycen> is the Vertical Alignment (0 – top/upper, 1 – center, 3 – bottom/lower)
          // <sta> is background Fill (0 – crop image, 1 – solid color, 2 – image, 3 – none)
          // <text> is the string content (constant or .txt attribute), ie “China”, or va0.txt
          // 
          // Example:
          // xstr 10,10,100,30,1,WHITE,GREEN,1,1,1,va0.txt
          
          
          String strToSend = "";
          char strClear[maxChar] = {" "};

          strToSend = "xstr " + String(x) + "," 
                                    + String(y) + "," 
                                    + String(w) + "," 
                                    + String(h) + "," 
                                    + String(font) + "," 
                                    + String(ink) + "," 
                                    + String(bckColor) + "," 
                                    + String(xcenter) + "," 
                                    + String(ycenter) + ","
                                    +  String(2) + ","
                                    + "\"" + strClear + "\"";
          writeString(strToSend);
          writeString("doevents");
          strToSend = "xstr " + String(x) + "," 
                                    + String(y) + "," 
                                    + String(w) + "," 
                                    + String(h) + "," 
                                    + String(font) + "," 
                                    + String(ink) + "," 
                                    + String(bckColor) + "," 
                                    + String(xcenter) + "," 
                                    + String(ycenter) + ","
                                    +  String(3) + ","
                                    + "\"" + txt  + "\"";
          writeString(strToSend);

      }

      void refreshInfoDebug()
      {
        myNex.writeStr("debug.dataOffset1.txt",dataOffset1);
        myNex.writeStr("debug.dataOffset2.txt",dataOffset2);
        myNex.writeStr("debug.dataOffset3.txt",dataOffset3);
        myNex.writeStr("debug.dataOffset4.txt",dataOffset4);

        myNex.writeStr("debug.offset1.txt",Offset1);
        myNex.writeStr("debug.offset2.txt",Offset2);
        myNex.writeStr("debug.offset3.txt",Offset3);
        myNex.writeStr("debug.offset4.txt",Offset4);

        // DEBUG Information
        myNex.writeStr("debug.blockLoading.txt",String(BLOCK_SELECTED));
        
        // Informacion de particiones
        myNex.writeStr("debug.partLoading.txt",String(PARTITION_BLOCK+1));
        myNex.writeStr("debug.totalParts.txt", (TOTAL_PARTS > 0) ? String(TOTAL_PARTS) : "1");

        // Esto solo para TAP
        myNex.writeStr("debug.dbgBlkInfo.txt",dbgBlkInfo);
        myNex.writeStr("debug.dbgPauseAB.txt",dbgPauseAB);
        myNex.writeStr("debug.dbgSync1.txt",dbgSync1);
        myNex.writeStr("debug.dbgSync2.txt",dbgSync2);
        myNex.writeStr("debug.dbgBit1.txt",dbgBit1);
        myNex.writeStr("debug.dbgBit0.txt",dbgBit0);
        myNex.writeStr("debug.dbgTState.txt",dbgTState);
        myNex.writeStr("debug.dbgRep.txt",dbgRep);      
      }

      void updateInformationMainPage(bool FORZE_REFRESH = false) 
      {            
          int blType = 0;
          String blName = "";
          // Para el caso de los players, no se usa esto.
          if (TYPE_FILE_LOAD != "MP3" && TYPE_FILE_LOAD != "WAV" && TYPE_FILE_LOAD != "FLAC" && TYPE_FILE_LOAD != "RADIO")
          {
                if ((STOP || PAUSE) && !REC)
                {
                  PROGRAM_NAME = FILE_LOAD;
                }
                else
                {
                    if (PLAY)
                    {
                        if (TYPE_FILE_LOAD == "TAP")
                        {
                          blName = myTAP.descriptor[BLOCK_SELECTED].name;
                          blType = myTAP.descriptor[BLOCK_SELECTED].type;
                        }
                        else
                        {

                          //logln("REC mode" + String(REC));
                          blName = myTZX.descriptor[BLOCK_SELECTED].name;
                          blType = myTZX.descriptor[BLOCK_SELECTED].type;
                        }

                        // Si no hay nombre, ponemos "..."
                        blName.trim();
                        
                        if (blName != "")
                        {
                            PROGRAM_NAME = blName;

                            if (blType == 1)
                            {
                              PROGRAM_NAME = "BYTE: " + PROGRAM_NAME;
                              LAST_PROGRAM_NAME = PROGRAM_NAME;
                            }
                            else if (blType == 7)
                            {
                              // Screen
                              PROGRAM_NAME = "BYTE: " + PROGRAM_NAME;
                              LAST_PROGRAM_NAME = PROGRAM_NAME;
                            }
                            else if (blType == 0)
                            {
                              PROGRAM_NAME = "PROGRAM: " + PROGRAM_NAME;
                              LAST_PROGRAM_NAME = PROGRAM_NAME;
                            }                       
                        }
                        else
                        {
                            // Si no hay nombre, ponemos "..."
                            PROGRAM_NAME = LAST_PROGRAM_NAME + " [code]";
                        }       
                      }                                
                }                  

                if (PLAY)
                {
                  if (CURRENT_PAGE == 3)
                  {
                      refreshInfoDebug();
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

                        if (lastPrgName!=PROGRAM_NAME || FORZE_REFRESH)
                        {
                          writeString("name.txt=\"" + PROGRAM_NAME + "\"");
                          writeString("tape2.name.txt=\"" + PROGRAM_NAME + "\"");
                        }
                        lastPrgName = PROGRAM_NAME;                
                    }

                }

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
                {
                    if(TYPE_FILE_LOAD == "TAP")
                    {
                        writeString("currentBlock.val=" + String(BLOCK_SELECTED + 1));
                    }
                    else
                    {
                        writeString("currentBlock.val=" + String(BLOCK_SELECTED));
                    }
                    lastBl2 = BLOCK_SELECTED;
                }            
          }

          // Actualizamos el LAST_MESSAGE
          if (lastMsn != LAST_MESSAGE || FORZE_REFRESH)
          {
            writeString("g0.txt=\"" + LAST_MESSAGE + "\"");
            // writeXSTR(66,247,342,16,2,65535,0,1,1,50,LAST_MESSAGE);
          }
          lastMsn = LAST_MESSAGE;
          
          // Actualizamos la barra de progreso
          if (lastPr1 != PROGRESS_BAR_BLOCK_VALUE || FORZE_REFRESH)
          {writeString("progressBlock.val=" + String(PROGRESS_BAR_BLOCK_VALUE));}
          lastPr1 = PROGRESS_BAR_BLOCK_VALUE;

          if (lastPr2 != PROGRESS_BAR_TOTAL_VALUE || FORZE_REFRESH)
          {writeString("progressTotal.val=" + String(PROGRESS_BAR_TOTAL_VALUE));}
          lastPr2 = PROGRESS_BAR_TOTAL_VALUE;

                            
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
              //logln("Verify comando: " + str);
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
        // Esperamos a que todos los datos salientes se hayan enviado
        SerialHW.flush();

        // if (SerialHW.available() >= 1) 
        // {
          // get the new uint8_t:
        while (SerialHW.available() > 0)
        {
          String strCmd = SerialHW.readString();
          getSeveralCommands(strCmd);
        }

        // Limpiamos el buffer de entrada
        while (SerialHW.available() > 0) 
        {
          SerialHW.read(); // Lee y descarta los datos
        }
        // }
      }

      String readStr() 
      {
        String strCmd = "";

        // Esperamos a que todos los datos salientes se hayan enviado
        SerialHW.flush();

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

        // Esperamos a que todos los datos salientes se hayan enviado
        SerialHW.flush();

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

      void getMemFree()
      {
          #ifdef DEBUGMODE
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
          #endif
          //
          updateMem();
      }           

      void updateMem()
      {
          UBaseType_t hwm_core0 = uxTaskGetStackHighWaterMark(Task0);
          UBaseType_t hwm_core1 = uxTaskGetStackHighWaterMark(Task1);

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
              writeString("menu.freePSRAM.txt=\""  + String(ESP.getFreePsram() / 1024) + " KB | " + String(ESP.getFreeHeap() / 1024) + " KB | " + String(hwm_core0 * 4 / 1024) + "KB | " + String(hwm_core1 * 4 / 1024) + " KB\"");
            #endif
          }
          lst_stackFreeCore1 = BLOCK_SELECTED;
          lst_psram_free = ESP.getFreePsram();
          lst_stack_free = ESP.getFreeHeap();
      }

      void refreshPulseIcons(bool polValue, bool lvlLowZeroValue)
      {            
          if (polValue == 1) 
          {
              if (lvlLowZeroValue == 1)
              {          
                writeString("tape.pulseInd.pic=36");
                writeString("tape0.pulseInd.pic=36");
              }
              else
              {
                writeString("tape.pulseInd.pic=33");
                writeString("tape0.pulseInd.pic=33");
              }
          }
          else
          {
              if (lvlLowZeroValue == 1)
              {          
                writeString("tape.pulseInd.pic=35");
                writeString("tape0.pulseInd.pic=35");
              }
              else
              {
                writeString("tape.pulseInd.pic=34");
                writeString("tape0.pulseInd.pic=34");
              }          
          }
      }  

      String getFileExtension(const String &fileName) {
        int dotIndex = fileName.lastIndexOf('.');
        if (dotIndex != -1) {
            // Si hay un punto, extraemos la extensión
            fileName.substring(dotIndex + 1).toUpperCase();
            return fileName.substring(dotIndex + 1); // Devuelve la extensión sin el punto
        }
        return ""; // Devuelve una cadena vacía si no hay extensión
      }

      String getFileNameFromPath(const String &filePath) {
          int lastSlash = filePath.lastIndexOf('/');
          if (lastSlash == -1) {
              lastSlash = filePath.lastIndexOf('\\'); // Por si se usa '\' como separador
          }
          if (lastSlash != -1) {
              return filePath.substring(lastSlash + 1); // Devuelve el nombre del archivo
          }
          return filePath; // Si no hay separador, devuelve la cadena completa
      }

// Variables globales de control

      void openBlockMediaBrowser(tAudioList* source) 
      {
          int totalblocks = 0;
          
          if (IRADIO_EN) 
          {
            // Esto lo hacemos para la visualizacion
            totalblocks = TOTAL_BLOCKS + 1; 
          }
          else
          {
            totalblocks = TOTAL_BLOCKS;
          }

          int max = MAX_BLOCKS_IN_BROWSER;
          if (totalblocks > max) {
              max = MAX_BLOCKS_IN_BROWSER;
          } else {
              max = totalblocks - 1;
          }
          BB_BROWSER_MAX = max;

          // Paso 0: Información general
          if (BB_BROWSER_STEP == 0) {
              BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;

              myNex.writeStr("mp3browser.path.txt",HMI_FNAME);
              myNex.writeStr("mp3browser.totalBl.txt",String(totalblocks - 1));
              myNex.writeStr("mp3browser.bbpag.txt",String(BB_PAGE_SELECTED));
              myNex.writeStr("mp3browser.size0.txt","SIZE[MB]");
              // writeString("mp3browser.path.txt=\"" + HMI_FNAME + "\"");
              // writeString("mp3browser.totalBl.txt=\"" + String(totalblocks - 1) + "\"");
              // writeString("mp3browser.bbpag.txt=\"" + String(BB_PAGE_SELECTED) + "\"");
              // writeString("mp3browser.size0.txt=\"SIZE[MB]\"");

              double ctpage = (double)totalblocks / (double)MAX_BLOCKS_IN_BROWSER;
              int totalPages = trunc(ctpage);
              if ((totalblocks % MAX_BLOCKS_IN_BROWSER != 0) && ctpage > 1) {
                  totalPages += 1;
              }
              myNex.writeStr("mp3browser.totalPag.txt",String(totalPages));
              // writeString("mp3browser.totalPag.txt=\"" + String(totalPages) + "\"");

              BB_BROWSER_STEP = 1; // Siguiente paso: primer item
              return;
          }

          // Paso 1..max: Mostrar cada item
          int i = BB_BROWSER_STEP;
          if (i <= BB_BROWSER_MAX) 
          {
              if (i + BB_PTR_ITEM > totalblocks - 1) 
              {
                  myNex.writeStr("mp3browser.id" + String(i) + ".txt","");
                  myNex.writeStr("mp3browser.name" + String(i) + ".txt","");
                  // writeString("mp3browser.id" + String(i) + ".txt=\"\"");
                  // writeString("mp3browser.name" + String(i) + ".txt=\"\"");
              } 
              else 
              {
                if (IRADIO_EN)
                {
                  myNex.writeStr("mp3browser.id" + String(i) + ".txt",String(i + BB_PTR_ITEM));
                  myNex.writeStr("mp3browser.name" + String(i) + ".txt",source[i + BB_PTR_ITEM - 1].filename);
                  // writeString("mp3browser.id" + String(i) + ".txt=\"" + String(i + BB_PTR_ITEM) + "\"");
                  // writeString("mp3browser.name" + String(i) + ".txt=\"" + source[i + BB_PTR_ITEM - 1].filename + "\"");                      
                }
                else
                {
                  String name = source[i + BB_PTR_ITEM - 1].filename;
                  myNex.writeStr("mp3browser.id" + String(i) + ".txt",String(i + BB_PTR_ITEM));
                  myNex.writeStr("mp3browser.name" + String(i) + ".txt",getFileNameFromPath(name));
                  //
                  // writeString("mp3browser.id" + String(i) + ".txt=\"" + String(i + BB_PTR_ITEM) + "\"");
                  // writeString("mp3browser.name" + String(i) + ".txt=\"" + getFileNameFromPath(name) + "\"");
                }
              }                
              BB_BROWSER_STEP++;
              // Si hemos terminado, reseteamos flags
              if (BB_BROWSER_STEP > BB_BROWSER_MAX) {
                  BB_OPEN = false;
                  BB_UPDATE = false;
                  BB_BROWSER_STEP = 0;
              }
          }
      }
            
      void openBlocksBrowser(tTZX myTZX = tTZX(), tTAP myTAP = tTAP())
      {
        // Rellenamos el browser con todos los bloques
        const int pa = 5;
        int max = MAX_BLOCKS_IN_BROWSER;
        int totalPages = 0;

        if (TOTAL_BLOCKS > max)
        {
          max = MAX_BLOCKS_IN_BROWSER;
        }
        else
        {
          max = TOTAL_BLOCKS - 1;
        }

        BB_PAGE_SELECTED = (BB_PTR_ITEM / MAX_BLOCKS_IN_BROWSER) + 1;

        myNex.writeStr("blocks.path.txt",HMI_FNAME);
        myNex.writeStr("blocks.totalBl.txt",String(TOTAL_BLOCKS - 1));
        myNex.writeStr("blocks.bbpag.txt",String(BB_PAGE_SELECTED));  
        myNex.writeStr("blocks.size0.txt","SIZE[B]");

        // writeString("blocks.path.txt=\"" + HMI_FNAME + "\"");
        // writeString("blocks.totalBl.txt=\"" + String(TOTAL_BLOCKS - 1) + "\"");
        // writeString("blocks.bbpag.txt=\"" + String(BB_PAGE_SELECTED) + "\"");
        // writeString("blocks.size0.txt=\"SIZE[B]\"");

        totalPages = ((TOTAL_BLOCKS) / MAX_BLOCKS_IN_BROWSER);
        if ((FILE_TOTAL_FILES) % MAX_BLOCKS_IN_BROWSER != 0)
        {
          totalPages += 1;
        }

        myNex.writeStr("blocks.totalPag.txt",String(totalPages));

        for (int i = 1; i <= max; i++)
        {
          if (i + BB_PTR_ITEM > TOTAL_BLOCKS - 1)
          {
            // Los dejamos limpios pero sin informacion
            myNex.writeStr("blocks.id" + String(i) + ".txt","");
            delay(pa);
            myNex.writeStr("blocks.data" + String(i) + ".txt","");
            delay(pa);
            myNex.writeStr("blocks.size" + String(i) + ".txt","");
            delay(pa);
            myNex.writeStr("blocks.name" + String(i) + ".txt","");
            delay(pa);

            // writeString("blocks.id" + String(i) + ".txt=\"\"");
            // writeString("blocks.data" + String(i) + ".txt=\"\"");
            // writeString("blocks.size" + String(i) + ".txt=\"\"");
            // writeString("blocks.name" + String(i) + ".txt=\"\"");
          }
          else
          {
            // En otro caso metemos informacion
            myNex.writeStr("blocks.id" + String(i) + ".txt",String(i + BB_PTR_ITEM));
            delay(pa);

            // writeString("blocks.id" + String(i) + ".txt=\"" + String(i + BB_PTR_ITEM) + "\"");

            if (TYPE_FILE_LOAD != "TAP")
            {

              if (String(myTZX.descriptor[i + BB_PTR_ITEM].typeName).indexOf("ID 21") != -1)
              {
                myNex.writeNum("blocks.id" + String(i) + ".pco",2016);
                delay(pa);
                myNex.writeNum("blocks.data" + String(i) + ".pco",2016);
                delay(pa);
                myNex.writeNum("blocks.size" + String(i) + ".pco",2016);
                delay(pa);
                myNex.writeNum("blocks.name" + String(i) + ".pco",2016);
                delay(pa);
                // writeString("blocks.id" + String(i) + ".pco=2016");
                // writeString("blocks.data" + String(i) + ".pco=2016");
                // writeString("blocks.size" + String(i) + ".pco=2016");
                // writeString("blocks.name" + String(i) + ".pco=2016");
              }
              else if (String(myTZX.descriptor[i + BB_PTR_ITEM].typeName).indexOf("ID 20") != -1)
              {
                myNex.writeNum("blocks.id" + String(i) + ".pco",64512);
                delay(pa);
                myNex.writeNum("blocks.data" + String(i) + ".pco",64512);
                delay(pa);
                myNex.writeNum("blocks.size" + String(i) + ".pco",64512);
                delay(pa);
                myNex.writeNum("blocks.name" + String(i) + ".pco",64512);
                delay(pa);

                // writeString("blocks.id" + String(i) + ".pco=64512");
                // writeString("blocks.data" + String(i) + ".pco=64512");
                // writeString("blocks.size" + String(i) + ".pco=64512");
                // writeString("blocks.name" + String(i) + ".pco=64512");
              }
              else
              {
                myNex.writeNum("blocks.id" + String(i) + ".pco",57051);
                delay(pa);
                myNex.writeNum("blocks.data" + String(i) + ".pco",57051);
                delay(pa);
                myNex.writeNum("blocks.size" + String(i) + ".pco",57051);
                delay(pa);
                myNex.writeNum("blocks.name" + String(i) + ".pco",57051);
                delay(pa);

                // writeString("blocks.id" + String(i) + ".pco=57051");
                // writeString("blocks.data" + String(i) + ".pco=57051");
                // writeString("blocks.size" + String(i) + ".pco=57051");
                // writeString("blocks.name" + String(i) + ".pco=57051");
              }

              int tzxSize = myTZX.descriptor[i + BB_PTR_ITEM].size;
              myNex.writeStr("blocks.data" + String(i) + ".txt",myTZX.descriptor[i + BB_PTR_ITEM].typeName);
               delay(pa);
              //writeString("blocks.data" + String(i) + ".txt=\"" + myTZX.descriptor[i + BB_PTR_ITEM].typeName + "\"");
              String tmpname = myTZX.descriptor[i + BB_PTR_ITEM].name;
              tmpname.trim();
              bool soloEspacios = (tmpname.length() == 0);
              if (!soloEspacios)
              {
                myNex.writeStr("blocks.name" + String(i) + ".txt",myTZX.descriptor[i + BB_PTR_ITEM].name);
                delay(pa);

                //writeString("blocks.name" + String(i) + ".txt=\"" + myTZX.descriptor[i + BB_PTR_ITEM].name + "\"");
              }
              else
              {
                myNex.writeStr("blocks.name" + String(i) + ".txt","|__ [BL-" + String(i) + "]");
                delay(pa);
                //writeString("blocks.name" + String(i) + ".txt=\"..[BL-" + String(i) + "]\"");
              }
              myNex.writeStr("blocks.size" + String(i) + ".txt",String(tzxSize));
              delay(pa);
              //writeString("blocks.size" + String(i) + ".txt=\"" + String(tzxSize) + "\"");
            }
            else
            {
              int tapSize = myTAP.descriptor[i + BB_PTR_ITEM - 1].size;
              myNex.writeStr("blocks.data" + String(i) + ".txt",myTAP.descriptor[i + BB_PTR_ITEM - 1].typeName);
              delay(pa);
              //writeString("blocks.data" + String(i) + ".txt=\"" + myTAP.descriptor[i + BB_PTR_ITEM - 1].typeName + "\"");

              String tmpname = myTAP.descriptor[i + BB_PTR_ITEM - 1].name;
              tmpname.trim();
              bool soloEspacios = (tmpname.length() == 0);
              if (!soloEspacios)
              {
                myNex.writeStr("blocks.name" + String(i) + ".txt",myTAP.descriptor[i + BB_PTR_ITEM - 1].name);
                delay(pa);
                //writeString("blocks.name" + String(i) + ".txt=\"" + myTAP.descriptor[i + BB_PTR_ITEM - 1].name + "\"");
              }
              else
              {
                myNex.writeStr("blocks.name" + String(i) + ".txt","|__ [BL-" + String(i) + "]");
                delay(pa);
                //writeString("blocks.name" + String(i) + ".txt=\"..[BL-" + String(i) + "]\"");
              }
              myNex.writeStr("blocks.size" + String(i) + ".txt",String(tapSize));
              delay(pa);
              //writeString("blocks.size" + String(i) + ".txt=\"" + String(tapSize) + "\"");
            }
          }
          
        }
      }

      // ---------------------------------------------------------------------------------------
      // HTTPClient
      // ---------------------------------------------------------------------------------------
    #ifdef AUTO_UPDATE 
    
    // ✅ FUNCIÓN SIMPLE PARA OBTENER ÚLTIMO TAG
    String getLatestReleaseTag() {
        if (!WIFI_CONNECTED) {
            logln("Error: WiFi not connected");
            LAST_MESSAGE = "WiFi not connected";
            return "";
        }
        
        HTTPClient http;
        WiFiClientSecure client;
        client.setInsecure();
        
        String apiUrl = "https://api.github.com/repos/hash6iron/powadcr/releases/latest";
        
        http.begin(client, apiUrl);
        http.addHeader("User-Agent", "PowaDCR/" + String(VERSION));
        
        logln("Getting latest release from: " + apiUrl);
        myNex.writeStr("update.status.txt","Checking latest version...");
        
        int httpCode = http.GET();
        
        if (httpCode != HTTP_CODE_OK) {
            logln("HTTP error: " + String(httpCode));
            myNex.writeStr("update.status.txt","GitHub API error: " + String(httpCode));
            http.end();
            return "";
        }
        
        String payload = http.getString();
        http.end();
        
        // ✅ PARSEAR JSON SIMPLE - SOLO EL TAG
        DynamicJsonDocument doc(4096);
        if (deserializeJson(doc, payload) != DeserializationError::Ok) {
            logln("JSON parse error");
            myNex.writeStr("update.status.txt","Parse error");
            return "";
        }
        
        String tagName = doc["tag_name"].as<String>();
        
        logln("Latest release tag: " + tagName);
        myNex.writeStr("update.status.txt","Latest version: " + tagName);
        
        return tagName;
    }

    // ✅ FUNCIÓN OPTIMIZADA PARA ARCHIVOS GRANDES
    bool downloadFileSimple(const String& url, const String& localPath) {
        HTTPClient http;
        WiFiClient* client = nullptr;
        WiFiClientSecure* secureClient = nullptr;
        
        // ✅ HTTP O HTTPS
        if (url.startsWith("https://")) {
            secureClient = new WiFiClientSecure();
            secureClient->setInsecure();
            http.begin(*secureClient, url);
        } else {
            client = new WiFiClient();
            http.begin(*client, url);
        }
        
        // ✅ TIMEOUTS LARGOS PARA ARCHIVOS GRANDES
        http.setTimeout(300000);  // 5 minutos
        http.setConnectTimeout(30000); // 30 segundos para conectar
        
        http.addHeader("User-Agent", "PowaDCR/" + String(VERSION));
        http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
        
        int httpCode = http.GET();
        
        if (httpCode != HTTP_CODE_OK) {
            logln("HTTP error downloading: " + String(httpCode));
            
            // ✅ MANEJO ESPECÍFICO DE REDIRECCIONES
            if (httpCode == HTTP_CODE_FOUND || httpCode == HTTP_CODE_MOVED_PERMANENTLY) {
                String redirectURL = http.getLocation();
                logln("Redirect to: " + redirectURL);
                
                http.end();
                if (client) delete client;
                if (secureClient) delete secureClient;
                
                return downloadFileSimple(redirectURL, localPath);
            }
            
            http.end();
            if (client) delete client;
            if (secureClient) delete secureClient;
            return false;
        }
        
        // ✅ OBTENER TAMAÑO DEL ARCHIVO
        int contentLength = http.getSize();
        logln("File size: " + String(contentLength) + " bytes");
        
        if (contentLength > 0) {
            LAST_MESSAGE = "Downloading " + String(contentLength / 1024) + " KB...";
            writeString("statusFILE.txt=\"" + LAST_MESSAGE + "\"");
        }
        
        // ✅ CREAR ARCHIVO
        File file = SD_MMC.open(localPath.c_str(), FILE_WRITE);
        if (!file) {
            logln("Cannot create file: " + localPath);
            http.end();
            if (client) delete client;
            if (secureClient) delete secureClient;
            return false;
        }
        
        // ✅ BUFFER MÁS GRANDE PARA ARCHIVOS GRANDES
        const size_t bufferSize = 8192; // 8KB buffer
        uint8_t* buffer = (uint8_t*)malloc(bufferSize);
        
        if (!buffer) {
            logln("Cannot allocate buffer");
            file.close();
            http.end();
            if (client) delete client;
            if (secureClient) delete secureClient;
            return false;
        }
        
        WiFiClient* stream = http.getStreamPtr();
        size_t totalBytes = 0;
        unsigned long lastProgressUpdate = 0;
        
        // ✅ DESCARGAR CON PROGRESO
        while (http.connected() && (contentLength <= 0 || totalBytes < contentLength)) {
            
            // ✅ VERIFICAR DATOS DISPONIBLES
            size_t availableData = stream->available();
            if (availableData == 0) {
                delay(10);
                continue;
            }
            
            // ✅ LEER EN CHUNKS GRANDES
            size_t bytesToRead = min(availableData, bufferSize);
            size_t bytesRead = stream->readBytes(buffer, bytesToRead);
            
            if (bytesRead == 0) {
                delay(10);
                continue;
            }
            
            // ✅ ESCRIBIR AL ARCHIVO EN CHUNKS
            size_t bytesWritten = file.write(buffer, bytesRead);
            if (bytesWritten != bytesRead) {
                logln("Write error - Expected: " + String(bytesRead) + ", Written: " + String(bytesWritten));
                break;
            }
            
            totalBytes += bytesRead;
            
            // ✅ PROGRESO CADA 2 SEGUNDOS
            if (millis() - lastProgressUpdate > 2000) {
                if (contentLength > 0) {
                    int progress = (totalBytes * 100) / contentLength;
                    LAST_MESSAGE = "Downloaded " + String(progress) + "% (" + String(totalBytes / 1024) + " KB)";
                } else {
                    LAST_MESSAGE = "Downloaded " + String(totalBytes / 1024) + " KB";
                }
                writeString("statusFILE.txt=\"" + LAST_MESSAGE + "\"");
                lastProgressUpdate = millis();
                
                // ✅ FLUSH PERIÓDICO PARA ARCHIVOS GRANDES
                file.flush();
                
                logln("Progress: " + String(totalBytes) + " / " + String(contentLength) + " bytes");
            }
            
            // ✅ YIELD PARA NO BLOQUEAR WATCHDOG
            yield();
            delay(1);
        }
        
        // ✅ LIMPIEZA
        free(buffer);
        file.flush();
        file.close();
        http.end();
        
        if (client) delete client;
        if (secureClient) delete secureClient;
        
        // ✅ VERIFICAR DESCARGA COMPLETA
        bool success = false;
        if (contentLength > 0) {
            success = (totalBytes >= contentLength * 0.99); // Tolerancia del 1%
        } else {
            success = (totalBytes > 1000); // Al menos 1KB descargado
        }
        
        if (success) {
            logln("Download completed successfully!");
            logln("Total bytes downloaded: " + String(totalBytes));
            return true;
        } else {
            logln("Download failed or incomplete");
            logln("Expected: " + String(contentLength) + " bytes");
            logln("Downloaded: " + String(totalBytes) + " bytes");
            
            // ✅ ELIMINAR ARCHIVO INCOMPLETO
            if (SD_MMC.exists(localPath.c_str())) {
                SD_MMC.remove(localPath.c_str());
                logln("Incomplete file deleted");
            }
            
            return false;
        }
    }

    // ✅ FUNCIÓN SIMPLE PARA DESCARGAR FIRMWARE - SIN VERIFICACIÓN DE ESPACIO
    bool downloadFirmwareFiles(const String& tagVersion) {
        if (tagVersion.length() == 0) {
            logln("Error: No tag version provided");
            myNex.writeStr("update.status.txt", "No version specified");
            return false;
        }
        
        // ✅ URLs DE DESCARGA DIRECTA
        String firmwareUrl = "https://github.com/hash6iron/powadcr/releases/download/" + tagVersion + "/firmware.bin";
        String interfaceUrl = "https://github.com/hash6iron/powadcr/releases/download/" + tagVersion + "/"+ HMI_MODEL +".tft";
        
        // ✅ RUTAS LOCALES
        String firmwarePath = "/firmware.bin";
        String interfacePath = "/"+ HMI_MODEL +".tft";
        
        logln("Downloading firmware files for version: " + tagVersion);
        myNex.writeStr("update.status.txt", "Downloading " + tagVersion + "...");
        
        // ✅ DESCARGAR FIRMWARE.BIN (EL MÁS GRANDE)
        logln("Downloading: " + firmwareUrl);
        myNex.writeStr("update.status.txt", "Downloading firmware.bin (>1MB)...");
        
        if (!downloadFileSimple(firmwareUrl, firmwarePath)) {
            logln("Failed to download firmware.bin");
            myNex.writeStr("update.status.txt","Failed: firmware.bin");
            return false;
        }
        
        // ✅ PAUSA ENTRE DESCARGAS
        delay(2000);
        
        // ✅ DESCARGAR POWADCR_IFACE.TFT
        logln("Downloading: " + interfaceUrl);
        myNex.writeStr("update.status.txt", "Downloading interface...");
        
        if (!downloadFileSimple(interfaceUrl, interfacePath)) {
            logln("Failed to download " + HMI_MODEL + ".tft");
            myNex.writeStr("update.status.txt","Failed: " + HMI_MODEL + ".tft");
            return false;
        }
        
        // ✅ VERIFICAR TAMAÑOS BÁSICOS (OPCIONAL)
        File firmwareFile = SD_MMC.open(firmwarePath.c_str(), FILE_READ);
        File interfaceFile = SD_MMC.open(interfacePath.c_str(), FILE_READ);
        
        if (firmwareFile && interfaceFile) {
            size_t firmwareSize = firmwareFile.size();
            size_t interfaceSize = interfaceFile.size();
            
            logln("Firmware size: " + String(firmwareSize) + " bytes");
            logln("Interface size: " + String(interfaceSize) + " bytes");
            
            // Verificación básica de tamaños mínimos
            if (firmwareSize > 1000000 && interfaceSize > 2000000) { // Firmware >1MB, Interface >2MB
                
                myNex.writeStr("update.status.txt", "Files downloaded");
                logln("Both files downloaded successfully!");
                
                firmwareFile.close();
                interfaceFile.close();
                return true;
            } else {
                logln("Downloaded files seem too small - possible corruption");
                myNex.writeStr("update.status.txt","Downloaded files corrupted");
                // Eliminar archivos corruptos
                SD_MMC.remove(firmwarePath.c_str());
                SD_MMC.remove("/powadcr_iface.tft");
            }
        }
        
        if (firmwareFile) firmwareFile.close();
        if (interfaceFile) interfaceFile.close();
        
        return false;
    }
    
    // ✅ FUNCIÓN PRINCIPAL PARA ACTUALIZACIÓN MANUAL
    void manualFirmwareUpdate() {
        logln("=== MANUAL FIRMWARE UPDATE ===");
        
        // ✅ PASO 1: OBTENER ÚLTIMO TAG
        String latestTag = getLatestReleaseTag();
        if (latestTag.length() == 0) {
            LAST_MESSAGE = "Failed to get version info";
            writeString("statusFILE.txt=\"" + LAST_MESSAGE + "\"");
            return;
        }
        
        // ✅ COMPARAR CON VERSIÓN ACTUAL
        String currentVersion = String(VERSION);
        HMI_MODEL = myNex.readStr("update.firmVersion.txt");
        myNex.writeStr("update.cVersion.txt",currentVersion.c_str());
        myNex.writeStr("update.nVersion.txt",latestTag.c_str());

        logln("Current version: " + currentVersion);
        logln("Latest version: " + latestTag);
        
        if (latestTag.compareTo(currentVersion) <= 0) {
            myNex.writeStr("update.status.txt","No update needed");
            logln("No update needed");
            return;
        }
        
        // ✅ PASO 2: DESCARGAR ARCHIVOS
        myNex.writeStr("update.status.txt","New update available");
        
        if (downloadFirmwareFiles(latestTag)) {
            myNex.writeStr("update.status.txt","All firmwares downloaded. Update now.");
            logln("Firmware update downloaded successfully!");
            logln("Files ready: /firmware.bin and /powadcr_iface.tft");
        } else {
            myNex.writeStr("update.status.txt","Update failed.");
            logln("Firmware update failed!");
        }
    }

    #endif

    void updateHMIfirmware()
    {
      char strpathtft[128] = {};
      String hmifile = "/" + myNex.readStr("update.firmVersion.txt") + ".tft";
      logln("Search for HMI file: " + hmifile);
      
      strcpy(strpathtft, hmifile.c_str());

      File firmwaretft = SD_MMC.open(strpathtft, FILE_READ);
      if (firmwaretft)
      {
        //writeStatusLCD("New display firmware");
        myNex.writeStr("update.status.txt","Start to flashing display ...");
        delay(2000);
        // NOTA: Este metodo necesita que la pantalla esté alimentada con 5V
        uploadFirmDisplay(strpathtft);
        // Esperamos al reinicio de la pantalla
        myNex.writeStr("screen.statusLCD.txt","Waiting for update page ...");
        delay(3000);
        writeString("page update");
        writeString("page update");
        delay(500);   
        //
        myNex.writeStr("update.status.txt","Display firmware updated");
        //tapeAnimationOFF();       
      }
      else
      {
        myNex.writeStr("update.status.txt","No display firmware found");
        delay(2000);
      }
    }

    void onOTAStart()
    {
      //pageScreenIsShown = false;
    }

    static void onOTAProgress(size_t currSize, size_t totalSize)
    {
      // log_v("CALLBACK:  Update process at %d of %d bytes...", currSize, totalSize);

    #ifdef DEBUGMODE
      logln("Uploading status: " + String(currSize / 1024) + "/" + String(totalSize / 1024) + " KB");
    #endif

      // if (!pageScreenIsShown)
      // {
      //   hmi.writeString("page screen");
      //   hmi.writeString("screen.updateBar.bco=23275");
      //   pageScreenIsShown = true;
      // }

      int prg = 0;
      prg = (currSize * 100) / totalSize;

      //hmi.writeString("statusLCD.txt=\"FIRMWARE UPDATING " + String(prg) + " %\"");
      myNex.writeStr("update.status.txt","Flashing Audiokit ... " + String(prg) + " %");
      //hmi.writeString("screen.updateBar.val=" + String(prg));
    }

    void onOTAEnd(bool success)
    {
      if (success)
      {
        myNex.writeStr("update.status.txt","Update finished");
      }
      else
      {
        myNex.writeStr("update.status.txt","Update error");
      }
    }

    void updateESP32firmware()
    {
        // log_v("");
        // log_v("Search for firmware..");
        char strpath[20] = {};
        strcpy(strpath, "/firmware.bin");

        File firmware = SD_MMC.open(strpath, FILE_READ);

        if (firmware)
        {
          logln("Firmware file opened " + String(strpath));
          myNex.writeStr("update.status.txt","Checking firmware");
          
          // Verificar magic byte antes del update
          uint8_t magicByte = firmware.peek();
          logln("First byte of firmware: 0x" + String(magicByte, HEX));
          
          if (magicByte != 0xE9) 
          {
              logln("ERROR: Invalid firmware file - Wrong magic byte");
              myNex.writeStr("update.status.txt","Invalid firmware file");
              firmware.close();
              
              // Eliminar archivo inválido
              if (SD_MMC.remove("/firmware.bin")) {
                  logln("Invalid firmware file deleted");
              }
              return;
          }    
          myNex.writeStr("update.status.txt","Flashing Audiokit ...");
          logln("Firmware found on SD_MMC");
          onOTAStart();
          logln("Updating firmware...");

          Update.onProgress(onOTAProgress);

          size_t firmwareSize = firmware.available();
          logln("Firmware size: " + String(firmwareSize) + " bytes");

          if(!Update.begin(firmwareSize))
          {
            logln("Error initializing updater obj.");
            Update.getError();
            Update.printError(Serial);
          }
          else
          {
              uint8_t buf[2048];
              int bytesRead = 0;
              size_t totalWritten = 0;

              while ((bytesRead = firmware.read(buf, sizeof(buf))) > 0) 
              {
                Update.write(buf, bytesRead);
                totalWritten += bytesRead;
                Serial.printf("Escritos: %d/%d bytes\n", totalWritten, firmwareSize);
              }        
          }

          if (Update.end())
          {
            log_v("Update finished!");
            myNex.writeStr("update.status.txt","Update finished");
            onOTAEnd(true);
            delay(2000);
          }
          else
          {
            log_e("Update error!");
            myNex.writeStr("update.status.txt","Update error");
            
            Update.getError();
            Update.printError(Serial);
            onOTAEnd(false);
            delay(2000);
          }

          firmware.close();

          if (SD_MMC.remove(strpath))
          {
            log_v("Firmware deleted succesfully!");
          }
          else
          {
            log_e("Firmware delete error!");
          }
          delay(2000);

          ESP.restart();
        }
        else
        {
          log_v("not found!");
          myNex.writeStr("update.status.txt","No firmware found");
          delay(2000);
        }
    }

    void uploadFirmDisplay(char *filetft)
    {
      
      File file;
      String ans = "";
      
      logln("Uploading file " + String(filetft));
      try
      {
        file = SD_MMC.open(filetft,FILE_READ);
      }
      catch(String error)
      {
        myNex.writeStr("update.status.txt","Error opening TFT file");
        delay(2000);
        return;
      }
      
      logln("File opened");

      if (file)
      {
          uint32_t filesize = file.available();
          logln("Starting uploading");

          //Enviamos comando de conexión
          // Forzamos un reinicio de la pantalla
          write("DRAKJHSUYDGBNCJHGJKSHBDN");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          delay(200);

          SerialHW.write(0x00);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          delay(200);

          write("connect");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          delay(500);

          write("connect");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);

          ans = readStr();
          delay(500);

          write("runmod=2");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);

          write("print \"mystop_yesABC\"");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);

          ans = readStr();

          write("get sleep");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          delay(200);

          write("get dim");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          delay(200);

          write("get baud");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          delay(200);

          write("prints \"ABC\",0");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          delay(200);

          ans = readStr();

          SerialHW.write(0x00);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          delay(200);

          write("whmi-wris " + String(filesize) + ",921600,1");
          SerialHW.write(0xff);
          SerialHW.write(0xff);
          SerialHW.write(0xff);

          delay(500);
          ans = readStr();

          //
          file.seek(0);

          // Enviamos los datos

          uint8_t buf[4096];
          size_t avail;
          size_t readcount;
          int bl = 0;

          while (filesize > 0)
          {
            // Leemos un bloque de 4096 bytes o el residual final < 4096
            readcount = file.read(buf, filesize > 4096 ? 4096 : filesize);

            // Lo enviamos
            SerialHW.write(buf, readcount);

            // Si es el primer bloque esperamos respuesta de 0x05 o 0x08
            // en el caso de 0x08 saltaremos a la posición que indica la pantalla.

            if (bl == 0)
            {
              String res = "";

              // Una vez enviado el primer bloque esperamos 2s
              delay(2000);

              while (1)
              {
                res = readStr();

                if (res.indexOf(0x08) != -1)
                {
                  int offset = (pow(16, 6) * int(res[4])) + (pow(16, 4) * int(res[3])) + (pow(16, 2) * int(res[2])) + int(res[1]);

                  if (offset != 0)
                  {
                    file.seek(0);
                    delay(50);
                    file.seek(offset);
                    delay(50);
                  }
                  break;
                }
                delay(50);
              }
            }
            else
            {
              String res = "";
              while (1)
              {
                // Esperams un ACK 0x05
                res = readStr();
                if (res.indexOf(0x05) != -1)
                {
                  break;
                }
              }
            }

            //
            bl++;
            // Vemos lo que queda una vez he movido el seek o he leido un bloque
            // ".available" mide lo que queda desde el puntero a EOF.
            filesize = file.available();
          }

          file.close();   
      }

      // Eliminamos el fichero
      delay(500);
      SD_MMC.remove(filetft);

    }

      // Constructor
      HMI()
      {}
};
