// Reinicializa todas las variables de estado de bloque para un nuevo ciclo de
// captura
#include "globales.h"
void resetBlockState(bool expectProgramHead) {}
// Reinicializa todas las variables críticas de estado para evitar residuos
// entre sesiones
void resetRecordingState() {}
/**
 * Ejemplo de uso:
 *   TAPrecorder taprec;
 *   bool ok = taprec.convertWAVtoTAP("/test.wav", "/output.tap");
 *   if (ok) Serial.println("Conversión exitosa");
 *   else Serial.println("Error en la conversión");
 *
 * Notas importantes:
 * - La lógica de decodificación de bits es un esqueleto y debe ajustarse para
 * timings reales del ZX Spectrum.
 * - El archivo WAV debe ser PCM, 16 bits, 44.1kHz, mono.
 * - El archivo .TAP generado puede requerir validación en un emulador ZX
 * Spectrum.
 * - Para mejores resultados, ajustar la detección de cruces por cero y la
 * reconstrucción de bits/bytes según la señal real.
 *
 * Pruebas recomendadas:
 * 1. Usar un WAV de una carga real de ZX Spectrum.
 * 2. Convertir y abrir el .TAP en un emulador (Fuse, ZXSpin, etc.).
 * 3. Si el TAP no es válido, revisar la lógica de timings y reconstrucción de
 * bytes.
 */
/* +++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    Nombre: TAPrecorder.ino

    Creado por:
      Copyright (c) Antonio Tamairón. 2023  /
 https://github.com/hash6iron/powadcr
      @hash6iron / https://powagames.itch.io/

    Descripción:
    Signal analyzer for REC function on powaDCR

    Version: 0.1

    Historico de versiones
    ----------------------
    v.0.1 - Version inicial

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

#include <cmath>

#pragma once

//#include <Stream.h>

class TAPrecorder {

public:
  bool errorInDataRecording = false;
  bool fileWasNotCreated = false;
  bool stopRecordingProccess = false;
  // int errorsCountInRec = 0;

  bool wasRenamed = false;
  bool nameFileRead = false;
  bool notAudioInNotified = false;
  // bool wasSelectedThreshold = false;
  int totalBlockTransfered = 0;
  // bool WasfirstStepInTheRecordingProccess = false;
  bool actuateAutoRECStop = false;

private:
  // AudioKit _kit;

  hw_timer_t *timer = NULL;

  HMI _hmi;
  // SdFat32 _sdf32;

  static const size_t BUFFER_SIZE_REC = 256; // 256 (09/07/2024)

  // Comunes
  char fileNameRename[25] = {""};
  char recDir[57] = {""};

  //
  // Umbral de histeresis para el filtro Schmitt al 10% de Vmax
  const int defaultThH = 3000;
  const int defaultThL = -3000;
  const int defaultAMP = 16384;

  int threshold_high = defaultThH;
  int threshold_low = defaultThL;
  int errorDetected = 0;
  //
  bool headerNameCaptured = false;
  bool blockSizeCaptured = false;
  // uint8_t nextSizeLSB;
  // uint8_t nextSizeMSB;

  int funTmax = 0;

// Para 44.1KHz / 8 bits / mono
//
// Guide tone
#if USE_ZX_SPECTRUM_SR == 1
  // Para 22200Hz / 8 bits / mono - ZX Spectrum
  //
  // Guide tone
  const int16_t wToneMin = 11; // min 23 Leader tone min pulse width
  const int16_t wToneMax = 20; // max 40 Leader tone max pulse width

  // Silencio
  const int16_t wSilence = 25;

  // Bit 0
  const int16_t wBit0_min = 1; // min 2
  const int16_t wBit0_max = 7; // max 15
  // Bit 1
  const int16_t wBit1_min = 8;  // min 16  02/11/2024
  const int16_t wBit1_max = 20; // max 40  02/11/2024
  // SYNC == BIT_0
  const int16_t wSyncMin = wBit0_min; // min 2
  const int16_t wSyncMax = wBit0_max; // max 15
#else
  // // Para 32800Hz / 8 bits / mono - ZX Spectrum
  // const int16_t wToneMin = 16;            //min 23 Leader tone min pulse
  // width const int16_t wToneMax = 29;            //max 40 Leader tone max
  // pulse width

  // // Silencio
  // const int16_t wSilence = 36;

  // // Bit 0
  // const int16_t wBit0_min = 1;            //min 2
  // const int16_t wBit0_max = 10;           //max 15
  // // Bit 1
  // const int16_t wBit1_min = 12;           //min 16  02/11/2024
  // const int16_t wBit1_max = 29;           //max 40  02/11/2024
  // // SYNC == BIT_0
  // const int16_t wSyncMin = wBit0_min;     //min 2
  // const int16_t wSyncMax = wBit0_max;     //max 15
  const int16_t wToneMin = int((23 * STANDARD_SR_REC_ZX_SPECTRUM) / 44100);
  const int16_t wToneMax = round((40 * STANDARD_SR_REC_ZX_SPECTRUM) / 44100);

  // Silencio
  const int16_t wSilence = round((65 * STANDARD_SR_REC_ZX_SPECTRUM) / 44100);

  // Bit 0
  const int16_t wBit0_min = int((2 * STANDARD_SR_REC_ZX_SPECTRUM) / 44100);
  const int16_t wBit0_max = round((15 * STANDARD_SR_REC_ZX_SPECTRUM) / 44100);
  // Bit 1
  const int16_t wBit1_min = int((16 * STANDARD_SR_REC_ZX_SPECTRUM) / 44100);
  const int16_t wBit1_max = round((40 * STANDARD_SR_REC_ZX_SPECTRUM) / 44100);
  // SYNC == BIT_0
  const int16_t wSyncMin = wBit0_min; // min 2
  const int16_t wSyncMax = wBit0_max; // max 15

#endif

  // Con EdgeWindow
  // const int ewLeaderTone = 45;
  // const int ewSync = 13;
  // const int ewBit0 = 18;
  // const int ewSilence = 150;

  // Controlamos el estado del TAP
  int statusSchmitt = 0;
  // Contamos los pulsos del tono piloto
  int pulseCount = 0;
  int lostPulses = 0;
  // Contamos los pulsos del sync1
  long silenceCount = 0;
  long lastSilenceCount = 0;
  long silenceSample = 0;

  // Recorder nuevo
  int cResidualToneGuide = 0;
  int cToneGuide = 0;
  int wPulseHigh = 0;
  int wPulseZero = 0;
  int pulseSilence = 0;

  //
  int high = 32767;
  int low = -32768;
  int zero = 0;

  char bitChStr[9] = {'\0'};
  uint8_t bitByte = 0;
  // uint8_t* datablock = nullptr;
  int ptrOffset = 0;
  // int badPulseW = 0;
  bool blockWithoutPrgHead = false;
  String bitString = "";

  uint8_t checksum = 0;
  uint8_t byteRead = 0;
  uint8_t lastByteRead = 0;

  // Zero crossing routine
  int _measuredPulseUpWidth = 0;
  int _measuredPulseDownWidth = 0;
  bool _pulseUpWasMeasured = false;
  bool _pulseDownWasMeasured = false;

  int _measureState = 0;
  int _measureSilence = 0;
  bool _pulseInverted = false;
  bool _silenceTimeOverflow = false;
  bool _silenceDetected = false;
  char tapeName[11];

  size_t lastByteCount = 0;
  size_t byteCount = 0;
  //

  // Info block
  int stateInfoBlock = 0;
  bool isPrgHead = true;

  struct tHeader {
    int type;
    char name[11] = {""};
    uint8_t sizeLSB;
    uint8_t sizeMSB;
    uint8_t sizeNextBlLSB;
    uint8_t sizeNextBlMSB;
    int blockSize = 19;
    int nextBlockSize = 0;
  };

  tHeader header;

  // struct tBlock
  // {
  //   uint8_t id = 0;
  //   char name[11] = {""};
  //   uint8_t offset = 0;
  //   uint8_t size = 0;
  //   bool error = false;
  //   bool headerBlock = false;
  // };

  // tBlock blockrec[2000];

  int blockStartOffset = 0;
  int lastBlockEndOffset = 0;

  void tapeAnimationON() {
    _hmi.writeString("tape2.tmAnimation.en=1");
    _hmi.writeString("tape.tmAnimation.en=1");
    delay(250);
    _hmi.writeString("tape2.tmAnimation.en=1");
    _hmi.writeString("tape.tmAnimation.en=1");
  }

  void tapeAnimationOFF() {
    _hmi.writeString("tape2.tmAnimation.en=0");
    _hmi.writeString("tape.tmAnimation.en=0");
    delay(250);
    _hmi.writeString("tape2.tmAnimation.en=0");
    _hmi.writeString("tape.tmAnimation.en=0");
  }

  void recAnimationON() {
    _hmi.writeString("tape.RECst.val=1");
    delay(250);
    _hmi.writeString("tape.RECst.val=1");
  }

  void recAnimationOFF() {
    _hmi.writeString("tape.RECst.val=0");
    delay(250);
    _hmi.writeString("tape.RECst.val=0");
  }

  void recAnimationFIXED_ON() {
    _hmi.writeString("tape.recIndicator.bco=63848");
    delay(250);
    _hmi.writeString("tape.recIndicator.bco=63848");
  }

  void recAnimationFIXED_OFF() {
    _hmi.writeString("tape.recIndicator.bco=32768");
    delay(250);
    _hmi.writeString("tape.recIndicator.bco=32768");
  }

  uint8_t calculateChecksum(uint8_t *bBlock, int startByte, int numBytes) {
    // Calculamos el checksum de un bloque de bytes
    uint8_t newChk = 0;

    // Calculamos el checksum (no se contabiliza el ultimo numero que es
    // precisamente el checksum)

    for (int n = startByte; n < (startByte + numBytes); n++) {
      newChk = newChk ^ bBlock[n];
    }

    return newChk;
  }

  void proccesByteCaptured(int byteCount, uint8_t byteRead) {

    // Flag para indicar DATA o HEAD
    if (byteCount == 0) {

      // Si esperabamos una cabecera PROGRAM ...
      if (isPrgHead == true && header.sizeLSB == 17) {
        // .. y vemos que el byte 1 es un flag DATA
        if (byteRead >= 128) {
          // No hay cabecera. Bloque DATA sin cabecera program.
          // entonvces el size del bloque lo tengo que añadir despues
          blockWithoutPrgHead = true;
          isPrgHead = false;
          // BYTES_TOBE_LOAD = 1;
          header.blockSize = 1;
          blockSizeCaptured = false;
          return;
        } else {
          // Cabecera
          // Entonces es una cabecera PROGRAM tal como se esperaba
          header.blockSize = 19;
          blockWithoutPrgHead = false;
          isPrgHead = true;
          blockSizeCaptured = true;
          // BYTES_TOBE_LOAD = 19;
        }
      }
    }
    // Flag tipo de cabecera para el HEAD
    else if (byteCount == 1) {
      // 0 - program
      // 1 - Number array
      // 2 - Char array
      // 3 - Code file
      header.type = byteRead;
      proccesInfoBlockType();
    }

    // Si es una cabecera diferente a DATA,
    // tomamos nombres y tamaño del DATA
    if (header.type < 128 && isPrgHead) {
      if (byteCount > 1 && byteCount < 12) {
        // Almacenamos el nombre. Los caracteres raros los sustituimos por
        // blancos
        if (byteRead > 20) {
          header.name[byteCount - 2] = (char)byteRead;
        } else {
          int valueTmp = 32;
          header.name[byteCount - 2] = (char)valueTmp;
        }
      }
      // Tamaño del siguiente bloque
      else if (byteCount == 12) {
        header.sizeNextBlLSB = byteRead;
      } else if (byteCount == 13) {

        header.sizeNextBlMSB = byteRead;
        int size = header.sizeNextBlLSB + (header.sizeNextBlMSB * 256);
        // BYTES_TOBE_LOAD = size;
        blockSizeCaptured = true;
        header.blockSize = size;

        logln("");
        logln("");
        logln("Next block size: " + String(size) + " bytes");
        logln("");
        logln("");

        // Guardamos el nombre del cassette que solo se coge una vez por
        // grabacion
        if (!headerNameCaptured) {
          strcpy(tapeName, header.name);
          headerNameCaptured = true;

          logln("Header name: " + String(header.name));
          logln("Tape name: " + String(tapeName));
        }
      }
    }
  }

  void showProgramName() {
    String prgName = "";

    if (sizeof(header.name) >= 10) {
      for (int n = 0; n < 10; n++) {
        prgName += header.name[n];
      }

      if (!nameFileRead) {
        PROGRAM_NAME = prgName;
        FILE_LOAD = prgName;
      } else {
        PROGRAM_NAME_2 = "";
      }
    } else {
      PROGRAM_NAME = header.name;
      // PROGRAM_NAME = "noname";
      // PROGRAM_NAME_2 = "noname";
    }

    PROGRAM_NAME_ESTABLISHED = true;
  }

  void proccesInfoBlockType() {

    if (blockWithoutPrgHead) {
      strncpy(LAST_TYPE, "BYTE.H-LESS", sizeof("BYTE.H-LESS"));
      return;
    }

    //
    // Bloque con cabecera
    //
    if (header.blockSize == 19) {
      if (header.type == 0) {
        strncpy(LAST_TYPE, "PROGRAM.HEAD", sizeof("PROGRAM.HEAD"));
      } else if (header.type == 1) {
        strncpy(LAST_TYPE, "NUMBER_ARRAY.HEAD", sizeof("NUMBER_ARRAY.HEAD"));
      } else if (header.type == 2) {
        strncpy(LAST_TYPE, "NUMBER_ARRAY.HEAD", sizeof("NUMBER_ARRAY.HEAD"));
      } else if (header.type == 3) {
        strncpy(LAST_TYPE, "BYTE.HEAD", sizeof("BYTE.HEAD"));
      }
    } else {
      //
      // Bloque data
      //
      if (header.blockSize > 0) {

        if (header.blockSize == 6912) {
          strncpy(LAST_TYPE, "SCREEN.DATA", sizeof("SCREEN.DATA"));
        } else {
          if (header.type != 0) {
            // Es un bloque de datos BYTE
            strncpy(LAST_TYPE, "BYTE.DATA", sizeof("BYTE.DATA"));
          } else if (header.type == 0) {
            // Es un bloque BASIC
            strncpy(LAST_TYPE, "BASIC.DATA", sizeof("BASIC.DATA"));
          }
        }
      }
    }

    // Información para la pagina debug
    dbgBlkInfo = String(LAST_TYPE);
  }

  void removeSpaces(char *str) {
    int i = 0, j = 0;
    while (str[i]) {
      if (str[i] != ' ') {
        str[j++] = str[i]; // Copia solo los caracteres que no son espacios
      }
      i++;
    }
    str[j] = '\0'; // Termina la cadena con un carácter nulo
  }

  int findFileWithNumber(const char *directory, const char *baseName) {

    String entry = "";
    File dir = SD_MMC.open(directory, FILE_READ);
    if (!dir || !dir.isDirectory()) {
#ifdef DEBUGMODE
      Serial.println("Error: Directory not found or invalid.");
#endif
      return -1; // El directorio no existe o no es válido
    }

    File file;
    char fileName[64];
    while (entry = file.getNextFileName()) {

      if (entry == "")
        break;

      strcpy(fileName, entry.c_str());
      // file.close();

      // Verificamos si el archivo tiene el formato <nombre>-<numero>.tap
      String fileStr = String(fileName);
      if (fileStr.startsWith(baseName) && fileStr.endsWith(".tap")) {
        int dashIndex = fileStr.indexOf('-');
        int dotIndex = fileStr.lastIndexOf('.');
        if (dashIndex != -1 && dotIndex != -1 && dashIndex < dotIndex) {
          String numberStr = fileStr.substring(dashIndex + 1, dotIndex);
          return numberStr.toInt(); // Devolvemos el número
        }
      }
    }

    return -1; // No se encontró ningún archivo con el formato especificado
  }

  bool renameRecFile(char newFileName[]) {
    // Reservamos memoria
    char cPath[57] = {""};
    char nfile[25] = {""};

    // Guardamos una copia de newFileName
    removeSpaces(newFileName);

    strcpy(nfile, newFileName);
    //

    String dirR = RECORDING_DIR + "/\0";
    strcpy(cPath, dirR.c_str());
    // Generamos un numero aleatorio para el final del nombre
    srand(time(0));
    delay(125);

    int copynum = findFileWithNumber(cPath, newFileName);
    int rn = (copynum == -1) ? 0 : copynum++;
    logln("Copy number of filename: " + String(rn));
    // Generamos el nuevo nombre del fichero
    // Le unimos la extensión .TAP
    String txtRn = "-" + String(rn) + ".tap";
    char const *extPath = txtRn.c_str();
    strcat(newFileName, extPath);
    // y unimos el fichero al path
    strcat(cPath, newFileName);

    // Comprobamos si existe el nuevo fichero
    logln("Ruta del REC: " + String(cPath));

    while (SD_MMC.exists(cPath)) {
      // Limpiamos el buffer
      memset(cPath, 0, sizeof(cPath));
      memset(newFileName, 0, strlen(newFileName));
      // Cogemos la copia original
      strcpy(cPath, dirR.c_str());
      strcpy(newFileName, nfile);
      // Incrementamos el número
      rn++;
      // Regeneramos el nuevo nombre del fichero
      txtRn = "-" + String(rn) + ".tap";
      char const *extPath = txtRn.c_str();
      strcat(newFileName, extPath);
      // y unimos el fichero al path
      strcat(cPath, newFileName);

#ifdef DEBUGMODE
      logln("");
      logln("Filename for rename: ");
      log(cPath);
      logln("");
#endif
    }

    logln("Nueva Ruta del REC para rename: " + String(cPath));
    char const *old_path = "/REC/_record";

    if (SD_MMC.rename(old_path, cPath)) {
      logln("File renamed successfully to: " + String(newFileName));
      return true;
    }

    return false;
  }

public:
  void set_HMI(HMI hmi) { _hmi = hmi; }

  void newBlock(File &mFile) {
    checksum = 0;
    // BYTES_TOBE_LOAD = 19;
    blockSizeCaptured = true;

    bitString = "";

    logln("New block");
    logln("Start : " + String(blockStartOffset));
    logln("End   : " + String(lastBlockEndOffset));
    logln("Size  : " + String(lastBlockEndOffset - blockStartOffset + 1) +
          " bytes");
    logln("");

    // Informacion para la pagina debug
    dataOffset1 = String(blockStartOffset);
    dataOffset4 = String(lastBlockEndOffset);

    blockStartOffset = lastByteCount;

    // Enviamos info a HMI
    if (CURRENT_PAGE == 3) {
      hmi.refreshInfoDebug();
    }

    // El nuevo bloque tiene que registrar su tamaño en el fichero .tap
    // depende si es HEAD or DATA
    if (isPrgHead) {
      // Es bloque CABECERA PROGRAM
      header.sizeLSB = 17;
      header.sizeMSB = 0;
    } else {
      // Este ahora es DATA
      header.sizeLSB = 0;
      header.sizeMSB = 0;
    }

    // Hacemos la conversión, calculamos el nuevo block size
    // y lo metemos en el fichero.

    int size2 = 0;
    int size = header.sizeLSB + (header.sizeMSB * 256);
    uint8_t MSB = 0;
    uint8_t LSB = 0;
    uint8_t BYTEZERO = 0;

    // Hay que añadir este valor sumando 2, en la cabecera
    // antes del siguiente bloque
    size2 = size + 2;
    LSB = size2;
    MSB = size2 >> 8;

    // Antes del siguiente bloque metemos el size
    mFile.write(LSB);
    mFile.write(MSB);
  }

  void addBlockSize(File &mFile, int byteCount) {
    int ptrTmpPos = 0;
    ptrTmpPos = mFile.position();
    int size = 0;
    uint8_t MSB = 0;
    uint8_t LSB = 0;

    mFile.seek(ptrOffset);

    // Hay que añadir este valor sumando 2, en la cabecera
    // antes del siguiente bloque
    size = byteCount;
    LSB = size;
    MSB = size >> 8;

    // Antes del siguiente bloque metemos el size
    mFile.write(LSB);
    mFile.write(MSB);

    // Recuperamos la posicion actual
    // del puntero del fichero
    mFile.seek(ptrTmpPos);
  }

  /**
   * Convierte un archivo WAV (PCM 16bits, 44.1kHz, mono) a un archivo .TAP para
   * ZX Spectrum.
   * @param wavPath Ruta del archivo WAV en la SD.
   * @param tapPath Ruta de salida del archivo .TAP en la SD.
   * @return true si la conversión fue exitosa, false en caso de error.
   *
   * Proceso:
   * 1. Leer cabecera WAV y validar formato.
   * 2. Leer datos PCM y analizar cruces por cero para detectar pulsos.
   * 3. Decodificar bits y bytes según timings ZX Spectrum.
   * 4. Escribir bloques en formato .TAP.
   */
  bool convertWAVtoTAP(const char *wavPath, const char *tapPath) {
    logln("Converting WAV to TAP file (Refactored Logic).");
    logln("--------------------------------------------");

    LAST_MESSAGE = "Converting to TAP file";

    File wavFile = SD_MMC.open(wavPath, FILE_READ);
    if (!wavFile) {
      logln("WAV open failed");
      return false;
    }

    // --- Validate WAV Header ---
    struct WAVHeader {
      char riff[4];
      uint32_t chunkSize;
      char wave[4];
      char fmt[4];
      uint32_t subchunk1Size;
      uint16_t audioFormat;
      uint16_t numChannels;
      uint32_t sampleRate;
      uint32_t byteRate;
      uint16_t blockAlign;
      uint16_t bitsPerSample;
    };
    WAVHeader header;
    wavFile.read((uint8_t *)&header, sizeof(WAVHeader));

    if (strncmp(header.riff, "RIFF", 4) != 0 ||
        strncmp(header.wave, "WAVE", 4) != 0) {
      logln("Invalid WAV Header");
      wavFile.close();
      return false;
    }
    if (header.sampleRate != 44100 || header.bitsPerSample != 16) {
      logln("Only 44.1KHz 16-bit WAV is supported");
      wavFile.close();
      return false;
    }

    // Find 'data' chunk
    uint32_t dataSize = 0;
    while (wavFile.available()) {
      char chunkId[4];
      wavFile.read((uint8_t *)chunkId, 4);
      uint32_t chunkSize;
      wavFile.read((uint8_t *)&chunkSize, 4);
      if (strncmp(chunkId, "data", 4) == 0) {
        dataSize = chunkSize;
        break;
      } else {
        wavFile.seek(wavFile.position() + chunkSize);
      }
    }

    if (dataSize == 0) {
      logln("No data chunk found");
      wavFile.close();
      return false;
    }

    File tapFile = SD_MMC.open(tapPath, FILE_WRITE);
    if (!tapFile) {
      logln("Failed to open TAP file for writing");
      wavFile.close();
      return false;
    }

    // --- Processing Constants for 44.1kHz ---
    // Timings in samples (approx)
    // Leader Pulse: 2168 T-states * (44100 / 3500000) ~= 27.3 samples
    // Sync1: 667 T ~= 8.4 samples
    // Sync2: 735 T ~= 9.2 samples
    // Bit0 Pulse: 855 T ~= 10.7 samples (Full bit ~ 21.5 samples)
    // Bit1 Pulse: 1710 T ~= 21.5 samples (Full bit ~ 43 samples)

    // Thresholds for single pulse width (in samples)
    const int MIN_LEADER = 15; // Relaxed from 20
    const int MAX_LEADER = 50; // Relaxed from 35
    const int MAX_SYNC_1 = 18; // Relaxed from 13
    const int MIN_NOISE = 2;   // Ignore pulses < 2 samples

    // Thresholds for FULL BIT (2 consecutive pulses)
    const int BIT_THRESHOLD = 30;
    const int TIMEOUT =
        2500; // Increased timeout (~50ms) to avoid premature cut

    const size_t BUFFER_SAMPLES = 1024;
    int16_t buffer[BUFFER_SAMPLES];

    size_t totalSamples = dataSize / 2;
    size_t processedSamples = 0;

    enum State { SEARCH_LEADER, WAIT_SYNC_2, READ_DATA };
    State state = SEARCH_LEADER;

    uint32_t leaderPulseCount = 0;
    uint32_t maxLeaderFound = 0; // Debug
    std::vector<uint8_t> blockBuffer;
    uint8_t currentByte = 0;
    int bitsRead = 0;

    int lastSample = 0;
    uint32_t lastCrossIndex = 0;
    int pulseWidth = 0;
    int prevPulseWidth = 0;
    bool haveFirstPulse = false;
    int blocksFound = 0;

    logln("Starting Conversion...");

    while (processedSamples < totalSamples) {
      size_t toRead = (totalSamples - processedSamples > BUFFER_SAMPLES)
                          ? BUFFER_SAMPLES
                          : (totalSamples - processedSamples);
      wavFile.read((uint8_t *)buffer, toRead * 2);

      for (size_t i = 0; i < toRead; i++) {
        int16_t sample = buffer[i];
        processedSamples++;

        // Zero Crossing Detection
        bool crossing =
            (lastSample < 0 && sample >= 0) || (lastSample >= 0 && sample < 0);

        if (crossing) {
          pulseWidth = processedSamples - lastCrossIndex;

          // Noise Filter
          if (pulseWidth < MIN_NOISE) {
            // Ignore very short glitches, treat as part of current pulse
            continue;
          }

          lastCrossIndex = processedSamples;

          // --- State Machine ---
          switch (state) 
          {
            case SEARCH_LEADER:
            {
              if (pulseWidth >= MIN_LEADER && pulseWidth <= MAX_LEADER) {
                leaderPulseCount++;
                if (leaderPulseCount > maxLeaderFound)
                  maxLeaderFound = leaderPulseCount;
              } else if (leaderPulseCount > 200 && pulseWidth <= MAX_SYNC_1) {
                state = WAIT_SYNC_2;
                logln("Sync1 found after " + String(leaderPulseCount) +
                      " leader pulses.");
              } else {
                leaderPulseCount = 0;
              }
              break;
            }
            
            case WAIT_SYNC_2:
            {
              blockBuffer.clear();
              currentByte = 0;
              bitsRead = 0;
              haveFirstPulse = false;
              state = READ_DATA;
              break;
            }
          
            case READ_DATA:
            {
              if (pulseWidth > TIMEOUT) 
              {
                // End of Block
                if (blockBuffer.size() > 0) {
                  uint16_t len = blockBuffer.size();
                  tapFile.write((uint8_t *)&len, 2);
                  tapFile.write(blockBuffer.data(), len);
                  logln("Block Saved: " + String(len) + " bytes");
                  blocksFound++;
                }
                state = SEARCH_LEADER;
                leaderPulseCount = 0;
              } 
              else 
              {
                if (!haveFirstPulse) 
                {
                  prevPulseWidth = pulseWidth;
                  haveFirstPulse = true;
                }
                else 
                {
                  int fullPeriod = prevPulseWidth + pulseWidth;
                  haveFirstPulse = false;

                  currentByte <<= 1;
                  if (fullPeriod > BIT_THRESHOLD) 
                  {
                    currentByte |= 1;
                  }
                  bitsRead++;
                  if (bitsRead == 8) 
                  {
                    blockBuffer.push_back(currentByte);
                    currentByte = 0;
                    bitsRead = 0;
                  }
                }
              }
              break;
            }
          }
        }
        lastSample = sample;

        if ((processedSamples % 100000) == 0) {
          int progress = (int)((processedSamples * 100ULL) / totalSamples);
          if (blocksFound == 0) {
            LAST_MESSAGE = "Scanning (" + String(progress) +
                           "%). MaxL: " + String(maxLeaderFound);
          } else {
            LAST_MESSAGE = "Saving (" + String(progress) +
                           "%). Blks: " + String(blocksFound);
          }
        }
      }
    }

    // Flush last block if file ends abruptly
    if (state == READ_DATA && blockBuffer.size() > 0) {
      uint16_t len = blockBuffer.size();
      tapFile.write((uint8_t *)&len, 2);
      tapFile.write(blockBuffer.data(), len);
      logln("Final Block Saved: " + String(len) + " bytes");
    }

    wavFile.close();
    tapFile.close();
    LAST_MESSAGE = "Conversion Done";
    logln("Conversion Completed.");
    return true;
  }

  /**
   * Convierte un archivo WAV a un archivo TZX con un bloque Direct Recording (ID 0x15).
   * El archivo WAV debe ser PCM, 8 o 16 bits, mono.
   * El archivo TZX generado contendrá un solo bloque Direct Recording.
   *
   * @param wavPath Ruta del archivo WAV de entrada
   * @param tzxPath Ruta del archivo TZX de salida
   * @return true si la conversión fue exitosa, false en caso de error
   */
  bool convertWAVtoTZXDR(const char *wavPath, const char *tzxPath) {
    logln("Converting WAV to TZX Direct Recording block...");
    LAST_MESSAGE = "Converting to TZX Direct Recording";

    File wavFile = SD_MMC.open(wavPath, FILE_READ);
    if (!wavFile) {
      logln("WAV open failed");
      return false;
    }

    // --- Leer cabecera WAV ---
    struct WAVHeader {
      char riff[4];
      uint32_t chunkSize;
      char wave[4];
      char fmt[4];
      uint32_t subchunk1Size;
      uint16_t audioFormat;
      uint16_t numChannels;
      uint32_t sampleRate;
      uint32_t byteRate;
      uint16_t blockAlign;
      uint16_t bitsPerSample;
    };
    WAVHeader header;
    wavFile.read((uint8_t *)&header, sizeof(WAVHeader));

    if (strncmp(header.riff, "RIFF", 4) != 0 || strncmp(header.wave, "WAVE", 4) != 0) {
      logln("Invalid WAV Header");
      wavFile.close();
      return false;
    }
    if (header.numChannels != 1 || (header.bitsPerSample != 8 && header.bitsPerSample != 16)) {
      logln("Only mono 8/16-bit WAV supported");
      wavFile.close();
      return false;
    }

    // Buscar chunk 'data'
    uint32_t dataSize = 0;
    while (wavFile.available()) {
      char chunkId[4];
      wavFile.read((uint8_t *)chunkId, 4);
      uint32_t chunkSize;
      wavFile.read((uint8_t *)&chunkSize, 4);
      if (strncmp(chunkId, "data", 4) == 0) {
        dataSize = chunkSize;
        break;
      } else {
        wavFile.seek(wavFile.position() + chunkSize);
      }
    }
    if (dataSize == 0) {
      logln("No data chunk found");
      wavFile.close();
      return false;
    }

    File tzxFile = SD_MMC.open(tzxPath, FILE_WRITE);
    if (!tzxFile) {
      LAST_MESSAGE = "Failed to open TZX file";
      logln("Failed to open TZX file for writing");
      wavFile.close();
      return false;
    }

    // --- Escribir cabecera TZX ---
    tzxFile.write((const uint8_t *)"ZXTape!", 7);
    tzxFile.write(0x1A);
    tzxFile.write(0x01); // Major version
    tzxFile.write(0x20); // Minor version

    // --- Preparar bloque Direct Recording (ID 0x15) ---
    tzxFile.write(0x15); // ID

    uint32_t blockLen = 0x0A + dataSize; // longitud del bloque (sin el ID)
    tzxFile.write((uint8_t *)&blockLen, 3); // 3 bytes, little endian

    // Duración de cada bit en T-states (aprox. para ZX Spectrum)
    uint16_t tstates = (uint16_t)(3500000.0 / header.sampleRate); // 3.5MHz clock
    tzxFile.write((uint8_t *)&tstates, 2);

    tzxFile.write(header.bitsPerSample); // Bits por muestra

    // Número de muestras (4 bytes, little endian)
    uint32_t numSamples = dataSize / (header.bitsPerSample / 8);
    tzxFile.write((uint8_t *)&numSamples, 4);

    tzxFile.write(0x80); // Valor de nivel alto (por defecto 0x80)

    // --- Escribir datos PCM ---
    // const size_t BUF_SIZE = 8192; // Buffer grande para acelerar E/S
    // uint8_t buf[BUF_SIZE];
    // uint32_t bytesLeft = dataSize;
    // uint32_t bytesProcessed = 0;
    // int lastPercent = -1;
    // while (bytesLeft > 0) {
    //   size_t toRead = (bytesLeft > BUF_SIZE) ? BUF_SIZE : bytesLeft;
    //   wavFile.read(buf, toRead);
    //   tzxFile.write(buf, toRead);
    //   bytesLeft -= toRead;
    //   bytesProcessed += toRead;
    //   int percent = (int)((bytesProcessed * 100ULL) / dataSize);
    //   // Solo actualiza cada 10% para minimizar impacto en HMI
    //   if (percent != lastPercent && percent % 10 == 0) {
    //     LAST_MESSAGE = String("TZX DR: ") + String(percent) + "%";
    //     lastPercent = percent;
    //   }
    // }

    // --- Escribir datos PCM ---
    const size_t BUF_SAMPLES = 8192; // 8 Kbytes para 16 bits
    int16_t buf16[BUF_SAMPLES];
    uint8_t buf8[BUF_SAMPLES];
    uint32_t bytesLeft = dataSize;
    uint32_t bytesProcessed = 0;
    int lastPercent = -1;
    while (bytesLeft > 0) {
        size_t toRead = (bytesLeft > BUF_SAMPLES * 2) ? BUF_SAMPLES * 2 : bytesLeft;
        
        wavFile.read((uint8_t*)buf16, toRead);
        size_t samplesRead = toRead / 2;
        for (size_t i = 0; i < samplesRead; i++) {
            buf8[i] = (uint8_t)(((int32_t)buf16[i] + 32768) >> 8); // 16b->8b
        }
        tzxFile.write(buf8, samplesRead);
        bytesLeft -= toRead;
        bytesProcessed += toRead;
        int percent = (int)((bytesProcessed * 100ULL) / dataSize);
        // Solo actualiza cada 10% para minimizar impacto en HMI
        if (percent != lastPercent) {
          LAST_MESSAGE = String("TZX DR: ") + String(percent) + "%";
          lastPercent = percent;
        }        
    }    

    wavFile.close();
    tzxFile.close();
    LAST_MESSAGE = "TZX Direct Recording conversion done";
    logln("TZX Direct Recording conversion completed.");
    return true;
  }

  bool recording() {
    // Reestablece todas las variables críticas de estado
    resetRecordingState();

    // Reset de nombres
    FILE_LOAD = "";
    PROGRAM_NAME = "";
    PROGRAM_NAME_2 = "";

    dbgBit0 = "";
    dbgBit1 = "";
    dbgSync1 = "";
    dbgSync2 = "";
    dbgTState = "";
    dbgPauseAB = "";
    dbgBlkInfo = "";
    dbgRep = "";

    long startTime = millis();
    int AmpHi = high;
    int AmpLo = low;
    int AmpZe = zero;

    int16_t oneValueR = 0;
    int16_t oneValueL = 0;
    int16_t audioInValue = 0;
    int16_t audioOutValue = 0;

    size_t resultOut = 0;
    size_t lenSamplesCaptured = 0;
    int chn = 2;

    int stateRecording = 0;
    int statusPulse = 0;
    int countSamplesHigh = 0;
    int countSamplesZero = 0;
    size_t bitCount = 0;
    // size_t byteCount = 0;
    size_t blockCount = 0;
    bool pulseOkHigh = false;
    bool pulseOkZero = false;

    // Pulso anterior minimo ancho
    uint8_t delta = wSyncMin;
    bool animationPause = false;
    bool targetReady = false;

    // kitStream.config().buffer_size = 262144;
    // kitStream.config().rx_tx_mode = RXTX_MODE;

    // ======================================================================================================

    // Creamos el buffer de grabacion
    uint8_t bufferRec[BUFFER_SIZE_REC];
    uint8_t bufferOut[BUFFER_SIZE_REC];

    int16_t *value_ptr = (int16_t *)bufferRec;

    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    // Creamos el fichero de salida
    // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
    const char fileName[20] = {"_record\0"};

    String dirR = RECORDING_DIR + "/\0";
    strcpy(recDir, dirR.c_str());

    if (!SD_MMC.mkdir(RECORDING_DIR)) {
#ifdef DEBUGMODE
      log("Error! Directory exists or wasn't created");
#endif
    } else {
      // Actualizamos el sistema de ficheros.
      _hmi.reloadCustomDir("/");
    }

    // Se crea un nuevo fichero temporal con la ruta del REC
    strcat(recDir, fileName);
    // SerialHW.println("Dir for REC: " + String(recDir));
    File tapf = SD_MMC.open(recDir, FILE_WRITE);
    tapf.seek(0);

    // Hacemos una lectura residual
    lenSamplesCaptured = kitStream.readBytes(bufferRec, BUFFER_SIZE_REC);
    lenSamplesCaptured = 0;

    // Inicializamos el buffer
    for (int i = 0; i < BUFFER_SIZE_REC; i++) {
      bufferRec[i] = 0;
    }
    delay(1000);

    // Ya no pasamos por aquí hasta parar el recorder
    //
    newBlock(tapf);
    headerNameCaptured = false;

    // ======================================================================================================
    // Esto lo hacemos porque el PAUSE entra como true en esta rutina

    REC = true;
    PAUSE = false;
    STOP = false;
    EJECT = false;
    errorInDataRecording = false;
    stopRecordingProccess = false;
    errorDetected = 0;

    // Cambiamos el sampling rate en el HW
    auto new_sr = kitStream.defaultConfig(RXTX_MODE);
    new_sr.input_device = ADC_INPUT_LINE2;
    new_sr.sample_rate = STANDARD_SR_REC_ZX_SPECTRUM;
    kitStream.setAudioInfo(new_sr);
    // Indicamos
    _hmi.writeString("tape.lblFreq.txt=\"" +
                     String(int(STANDARD_SR_REC_ZX_SPECTRUM / 1000)) + "KHz\"");

    unsigned long progress_millis = 0;
    unsigned long targetReadyTime = millis();
    progress_millis = millis();

    // Reset de variables de estado
    // ------------------------------------------------------------------
    blockStartOffset = 0;
    lastBlockEndOffset = 0;
    PROGRAM_NAME_ESTABLISHED = false;
    byteCount = 0;
    cResidualToneGuide = 0;
    cToneGuide = 0;
    wPulseHigh = 0;
    wPulseZero = 0;
    pulseSilence = 0;
    BLOCK_REC_COMPLETED = false;
    isPrgHead = true;
    statusPulse = 0;
    bitCount = 0;
    blockCount = 0;
    checksum = 0;
    errorInDataRecording = false;
    stopRecordingProccess = false;
    errorDetected = 0;
    pulseOkHigh = false;
    pulseOkZero = false;
    header.blockSize = 19;
    header.sizeLSB = 17;
    header.sizeMSB = 0;
    header.sizeNextBlLSB = 0;
    header.sizeNextBlMSB = 0;
    header.type = 0;
    headerNameCaptured = false;
    blockWithoutPrgHead = false;
    blockSizeCaptured = false;
    bitByte = 0;
    byteRead = 0;
    lastByteRead = 0;
    ptrOffset = 0;
    for (int i = 0; i < 10; i++) {
      header.name[i] = ' ';
      tapeName[i] = ' ';
    }
    strcpy(header.name, "noname");
    strcpy(tapeName, "PROGRAM_ZX");
    REC_FILENAME = "";
    // ------------------------------------------------------------------

    // Esto lo hacemos para mejorar la eficacia del recording
    while (REC && !STOP && !EJECT && !errorInDataRecording &&
           !stopRecordingProccess) {
      if ((millis() - targetReady > 5000) && !targetReady) {
        targetReady = true;
        LAST_MESSAGE = "Recorder ready. Play source data.";
      }

      // Capturamos la configuracion del threshold para el disparador de Schmitt
      if (EN_SCHMITT_CHANGE) {
        // Cuando se habilita la configuracion del disparador de Schmitt
        // se pueden ajustar los parametros de amplitud y banda de schmitt
        // (histeresis)

        // Amplitud
        AmpHi = (SCHMITT_AMP * 32767) / 100;
        AmpLo = (-SCHMITT_AMP * 32768) / 100; // -50% de amplitud
        AmpZe = 0;
        // Histeresis disparador de Schmitt
        threshold_high = (SCHMITT_THR * AmpHi) / 100;
        threshold_low = (SCHMITT_THR * AmpLo) / 100;
      } else {
        // Por defecto
        AmpHi = 32767;
        AmpLo = -32768;
        AmpZe = 0;
      }

      // Capturamos muestras
      lenSamplesCaptured = kitStream.readBytes(bufferRec, BUFFER_SIZE_REC);

      // Esperamos a que el buffer este lleno
      if (lenSamplesCaptured >= BUFFER_SIZE_REC) 
      {
        // Apuntamos al buffer de grabacion
        int16_t *value_ptr = (int16_t *)bufferRec;
        // Apuntamos al buffer de salida
        int16_t *ptrOut = (int16_t *)bufferOut;
        resultOut = 0;

        // Analizamos todas las muestras del buffer de entrada
        for (int j = 0; j < (lenSamplesCaptured / 4); j++) {
          // Leemos los samples del buffer
          // canal R
          oneValueR = *value_ptr++;
          // canal L
          oneValueL = *value_ptr++;

          // Por defecto el canal que se coge es el izquierdo.
          // en el caso de swapping se coge el derecho
          audioInValue = (SWAP_EAR_CHANNEL) ? oneValueR : oneValueL;

          // Aplicamos Schmitt Trigger (noise filtering)
          if (EN_SCHMITT_CHANGE) {
            // Aplicamos el filtro Schmitt
            audioInValue = (audioInValue >= 0 && audioInValue <= threshold_high)
                               ? AmpZe
                               : AmpHi;
            audioInValue = (audioInValue >= threshold_low && audioInValue < 0)
                               ? AmpZe
                               : AmpLo;
          }

          // Rectificamos la onda
          // Invertimos la onda (si se quiere)
          audioInValue =
              (EN_EAR_INVERSION) ? audioInValue * (-1) : audioInValue;

          // Eliminamos la parte negativa (subimos la señal al cero)
          // audioInValue = (audioInValue <= 0) ? AmpZe:audioInValue;
          audioInValue = (audioInValue <= 0)
                             ? (audioInValue + ((-1) * audioInValue))
                             : (2 * audioInValue);
          // Cortamos amplitud - Saturacion
          audioInValue = (audioInValue > 32767) ? 32767 : audioInValue;

          // ++++++++++++++++++++++++++++++++++++++++++
          // Control de cambios de flanco
          // ++++++++++++++++++++++++++++++++++++++++++
          if (!PAUSE && targetReady) {
            if (animationPause) {
              recAnimationOFF();
              delay(125);
              recAnimationFIXED_ON();
              tapeAnimationON();
              animationPause = false;
            }

            switch (statusPulse) {

            case 0:
              // Estado inicial
              if (audioInValue > threshold_high) {
                // Contabilizo esa muestra
                countSamplesHigh++;
                audioOutValue = audioInValue;

                if (countSamplesHigh > delta) {
                  // La onda empieza en AmpHi
                  statusPulse = 1;
                  pulseOkZero = false;
                  pulseOkHigh = false;
                }
              } else if (audioInValue == AmpZe) {
                // Contabilizo esa muestra
                countSamplesZero++;
                audioOutValue = AmpZe;

                if (countSamplesZero > delta) {
                  // La onda empieza en AmpZe
                  statusPulse = 2;
                  pulseOkZero = false;
                  pulseOkHigh = false;
                }
              }
              break;

            case 1:
              // Estasdo en AmpHi
              if (audioInValue > threshold_high) {
                countSamplesHigh++;
                audioOutValue = audioInValue;

                countSamplesZero = 0;

                // Deteccion predictiva de un silencio
                if (countSamplesHigh > wSilence) {
                  // Almaceno el ancho del pulso ZERO
                  wPulseHigh = countSamplesHigh;
                  // Reset de variables
                  countSamplesZero = 0;
                  countSamplesHigh = 0;
                  wPulseZero = 0;
                  // Actualizo flags
                  pulseOkZero = false;
                  pulseOkHigh = true;
                  audioOutValue = AmpHi;
                  // Cambio de estado
                  statusPulse = 0;
                }
              } else if (audioInValue == AmpZe && countSamplesHigh > delta) {
                // Cambio de estado
                statusPulse = 0;
                // Contabilizo la muestra leida
                countSamplesZero++;
                // Almaceno el ancho del pulso HIGH
                wPulseHigh = countSamplesHigh;
                // Reset de variables
                wPulseZero = 0;
                countSamplesHigh = 0;
                // Actualizo los flags
                pulseOkHigh = true;
                pulseOkZero = false;
                audioOutValue = AmpZe;
              }
              break;

            case 2:
              // Estado en AmpZe
              if (audioInValue == AmpZe) {
                countSamplesZero++;
                countSamplesHigh = 0;

                audioOutValue = AmpZe;

                // Deteccion predictiva de un silencio
                if (countSamplesZero > wSilence) {
                  // Almaceno el ancho del pulso ZERO
                  wPulseZero = countSamplesZero;
                  // Reset de variables
                  countSamplesZero = 0;
                  countSamplesHigh = 0;
                  wPulseHigh = 0;
                  // Actualizo flags
                  pulseOkZero = true;
                  pulseOkHigh = false;
                  // Cambio de estado
                  statusPulse = 0;
                }
              } else if (audioInValue > threshold_high &&
                         countSamplesZero > delta) {
                // Cambio de estado
                statusPulse = 0;
                // Contabilizo la muestra leida
                countSamplesHigh++;
                // Almaceno el ancho del pulso ZERO
                wPulseZero = countSamplesZero;
                // Reset de variables
                wPulseHigh = 0;
                countSamplesZero = 0;
                // Actualizo flags
                pulseOkZero = true;
                pulseOkHigh = false;

                audioOutValue = audioInValue;
              }
              break;

            default:
              break;
            }

            // Pasamos el valor a la salida
            // audioOutValue = audioInValue;

            // +++++++++++++++++++++++++++++++++++++++++++++
            // Analisis del tren de pulsos
            // +++++++++++++++++++++++++++++++++++++++++++++
            switch (stateRecording) 
            {
            // TONO GUIA
            case 0: { // Esperando un tono guia
              if (pulseOkHigh) {
                pulseOkHigh = false;

                if (wPulseHigh >= wToneMin && wPulseHigh <= wToneMax) {
                  cToneGuide++;
                  wPulseHigh = 0;

                  if (cToneGuide > 256) {
                    stateRecording = 1;
                    wPulseHigh = 0;
                    cToneGuide = 0;

                    if (isPrgHead) {
                      LAST_MESSAGE = "Waiting for HEAD";
                    } else {
                      LAST_MESSAGE = "Waiting for DATA";
                    }
                  }
                } else {
                  cToneGuide = 0;
                  wPulseHigh = 0;
                }
              }
              break;
            }

            // SYNC
            case 1: {
              // SYNC
              if (pulseOkHigh) {
                pulseOkHigh = false;

                if (wPulseHigh >= wSyncMin && wPulseHigh <= wSyncMax) {
                  wPulseHigh = 0;
                  stateRecording = 2;

                  // Info para la pagina debug
                  dbgSync1 = "1";
                  dbgSync2 = "1";
                } else {
                  // Info para la pagina debug
                  dbgSync1 = "";
                  dbgSync2 = "";
                }

                // // Detectamos si el pulso está invertido
                // if (wPulseHigh <= 9)
                // {
                //   EN_MIC_INVERSION = true; // Activamos la inversion de la
                //   señal
                // }
                // else
                // {
                //   EN_MIC_INVERSION = false; // Desactivamos la inversion de
                //   la señal
                // }
              }
              break;
            }

            // Espera bloque PROGRAM
            case 2: {
              // Capturando DATA
              // Bit 0
              if (pulseOkHigh) {
                if (wPulseHigh >= wBit0_min && wPulseHigh <= wBit0_max) {
                  // Esto lo hacemos para que ya no analice un silencio
                  pulseOkZero = false;
                  pulseOkHigh = false;

                  wPulseHigh = 0;
                  // Generamos un bit 0
                  bitByte += (0 * pow(2, 7 - bitCount));
                  bitCount++;

                  // Info para la pagina debug
                  dbgBit0 = "1";
                  dbgBit1 = "";
                }
                // Bit 1
                else if (wPulseHigh >= wBit1_min && wPulseHigh <= wBit1_max) {
                  // Esto lo hacemos para que ya no analice un silencio
                  pulseOkZero = false;
                  pulseOkHigh = false;

                  wPulseHigh = 0;
                  // Generamos un bit 1
                  bitByte += (1 * pow(2, 7 - bitCount));
                  bitCount++;

                  // Info para la pagina debug
                  dbgBit0 = "";
                  dbgBit1 = "1";
                }
                // No se puede usar la deteccion de bit erroneo porque si es mas
                // grande el ancho del pulso probablemente sea un silencio y hay
                // que esperar.
                else {
                  if (wPulseHigh < wSilence) {
                    logln("Other pulse found. [ Byte: " +
                          String(byteCount + 1) + ", bit: " + String(bitCount) +
                          ", pw: " + String(wPulseHigh) + "]");
                    //
                    LAST_MESSAGE =
                        "Error wrong pulse [ Byte: " + String(byteCount + 1) +
                        ", bit: " + String(bitCount) +
                        ", pw: " + String(wPulseHigh) + "]";
                    // Error en checksum
                    errorDetected = 1;
                    // Paramos la grabacion
                    REC = false;
                    delay(5000);
                  }
                }

                // ++++++++++++++++++++++++++++++++++++++++++++++++++++
                // Conteo de bytes
                // ++++++++++++++++++++++++++++++++++++++++++++++++++++
                if (bitCount > 7) {
                  // Se ha capturado 1 bytes
                  bitCount = 0;
                  byteRead = bitByte;
                  // Procesamos el byte leido para saber si lleva cabecera, si
                  // no, tipo de bloque, etc.
                  proccesByteCaptured(byteCount, byteRead);
                  bitByte = 0;
                  // Calculamos el CRC
                  checksum = checksum ^ byteRead;
                  //
                  uint8_t valueToBeWritten = byteRead;
                  // Escribimos en fichero el dato
                  tapf.write(valueToBeWritten);

                  if (blockSizeCaptured) {
                    LAST_MESSAGE = "Bytes captured: " + String(byteCount) +
                                   " / " + String(header.blockSize) + " bytes";
                  } else {
                    LAST_MESSAGE =
                        "Bytes captured: " + String(byteCount) + " bytes";
                  }

                  byteCount++;
                  // Actualizo el fin de bloque
                  lastBlockEndOffset++;
                  //
                  if (millis() - progress_millis > 125) {
                    PROGRESS_BAR_BLOCK_VALUE =
                        (byteCount * 100) / header.blockSize;
                    _hmi.writeString("progressBlock.val=" +
                                     String(PROGRESS_BAR_BLOCK_VALUE));
                    progress_millis = millis();
                  }
                }
              }

              // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
              //
              // Gestion del silencio despues de bloque
              // si no era bit1 o bit0
              // ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
              if (pulseOkZero || pulseOkHigh) {
                pulseOkZero = false;
                pulseOkHigh = false;

                // ++++++++++++++++++++++++++++++++++++++++
                // Encontre silencio. Acabo el estado DATA
                // ++++++++++++++++++++++++++++++++++++++++
                // Tambien acabo el bloque si encuentro un pulso por encima de
                // un wBit1_max

                if ((wPulseHigh >= wSilence) || (wPulseZero >= wSilence)) {

                  // Verificamos el checksum
                  if (checksum == 0) {
                    //
                    // Incrementamos bloque reconocido
                    blockCount++;
                    lastByteCount += byteCount + 1;
                    //
                    LAST_MESSAGE = "Block [ " + String(blockCount) + " ] - " +
                                   String(byteCount) + " bytes";
                    TOTAL_BLOCKS = blockCount;
                    BLOCK_SELECTED = blockCount;
                    LAST_SIZE = byteCount;

                    // Procesamos informacion del bloque
                    if (!PROGRAM_NAME_ESTABLISHED) {
                      showProgramName();
                    }

                    proccesInfoBlockType();
                    addBlockSize(tapf, byteCount);

                    // Reseteo variables usadas
                    wPulseHigh = 0;
                    wPulseZero = 0;
                    byteCount = 0;

                    // Cambio de estado
                    stateRecording = 3;
                  } else {
                    // Error en los datos. Error de checksum
                    // Procesamos informacion del bloque
                    // proccesInfoBlockType();
                    //
                    errorInDataRecording = true;
                    stopRecordingProccess = true;

                    // Error en checksum
                    errorDetected = 1;

                    // Paramos la grabacion
                    REC = false;
                    //
                    // delay(3000);
                  }
                }
              }
              break;
            }

            // Espera bloque DATA
            case 3: {
              if (pulseOkHigh) {
                pulseOkHigh = false;
                // Esperando un tono guia
                if (wPulseHigh >= wToneMin && wPulseHigh <= wToneMax) {
                  cToneGuide++;
                  wPulseHigh = 0;

                  if (cToneGuide > 256) {
                    // Reinicia el estado de bloque para un nuevo ciclo limpio
                    // Si el último bloque fue DATA, espera PROGRAM; si fue
                    // PROGRAM, espera DATA
                    bool nextIsProgram = !isPrgHead;

                    // Nos preparamos para otro ciclo de grabacion
                    // ----------------------------------------------
                    bitCount = 0;
                    byteCount = 0;
                    blockSizeCaptured = false;
                    headerNameCaptured = false;
                    blockWithoutPrgHead = false;
                    errorDetected = 0;
                    bitByte = 0;
                    byteRead = 0;
                    lastByteRead = 0;
                    checksum = 0;
                    cToneGuide = 0;
                    wPulseHigh = 0;
                    wPulseZero = 0;
                    pulseSilence = 0;
                    isPrgHead = nextIsProgram;
                    // Si es cabecera, inicializa el header
                    if (nextIsProgram) {
                      header.blockSize = 19;
                      header.sizeLSB = 17;
                      header.sizeMSB = 0;
                      header.sizeNextBlLSB = 0;
                      header.sizeNextBlMSB = 0;
                      header.type = 0;
                      for (int i = 0; i < 10; i++)
                        header.name[i] = ' ';
                      strcpy(header.name, "noname");
                    }
                    // ----------------------------------------------
                    stateRecording = 0;
                    wPulseHigh = 0;
                    wPulseZero = 0;
                    cToneGuide = 0;
                    // guardamos la posición del puntero del fichero en este
                    // momento
                    ptrOffset = tapf.position();
                    newBlock(tapf);
                  }
                } else {
                  cResidualToneGuide = 0;
                  wPulseHigh = 0;
                }
              }
              break;
            }

            default:
              break;
            }

          } else {
            recAnimationON();
            delay(125);
            recAnimationFIXED_OFF();
            tapeAnimationOFF();
            animationPause = true;
          }

          uint8_t k = 1;

          // if (REC_AUDIO_LOOP)
          // {
          // R-OUT
          *ptrOut++ = (audioOutValue * k) * (MAIN_VOL_R / 100);
          //
          if (EN_SPEAKER) {
            // L-OUT (Speaker channel)
            *ptrOut++ = (audioOutValue * k) * (MAIN_VOL_L / 100);
          } else {
            *ptrOut++ = 0;
          }
          // }
        }

        // Volcamos el output en el buffer de salida
        // esto es necesario que esté aqui porque si no la captura no se lleva a
        // cabo. a priori no tiene sentido alguno ya que no es necesario para la
        // interpretacion de los datos pero algo debe ocurrir indirectamente con
        // el buffer de audio, que mejora la captura de datos
        kitStream.write(bufferOut, lenSamplesCaptured);
      } 
      else 
      {
        LAST_MESSAGE = "No signal detected.";
        logln("Incomplete capturing data");
        REC = false;
        delay(3000);
      }
    }

    // +++++++++++++++++++++++++++++++++++++++++++++
    // Lo renombramos con el nombre del BASIC
    // +++++++++++++++++++++++++++++++++++++++++++++
    char newFileName[25];

    if (headerNameCaptured) {
      strcpy(newFileName, tapeName);
    } else {
      strncpy(newFileName, "PROGRAM", sizeof(newFileName));
    }

    if (errorDetected != 0) {
      // El checksum no era correcto
      char hex_string[20];
      sprintf(hex_string, "%X", checksum);
      LAST_MESSAGE = "Error in checksum. [ 0x" + String(hex_string) + " ]";
      // Reseteamos el checksum
      checksum = 0;
      delay(3000);
      // Eliminamos desde blockStartOffset hasta el final
      // LAST_MESSAGE = "Removing bad block";
      tapf.seek(0);
      tapf.seek(blockStartOffset);

      logln("");
      logln("Removing bad block");
      logln("Start : " + String(blockStartOffset));
      logln("End   : " + String(lastBlockEndOffset));
      logln("Size  : " + String(lastBlockEndOffset - blockStartOffset + 1) +
            " bytes");
      logln("");

      for (int n = 0; n <= (lastBlockEndOffset - blockStartOffset) + 1; n++) {
        tapf.write((uint8_t)0);
      }
      // delay(1250);

      errorInDataRecording = true;
      // Indicamos que la grabaci'on tenia errores en el ultimo bloque
      // y no es completa
      FILE_LOAD = String(newFileName);
      // PROGRAM_NAME = String(newFileName);
    } else {
      // Pasamos el nombre para mostrarlo arriba en el indicador.
      FILE_LOAD = String(newFileName);
      // PROGRAM_NAME = String(newFileName);
    }

    delay(125);
    //
    tapf.close();

    // Una vez cerrado. Lo renombro
    if (!renameRecFile(newFileName)) {
      logln("Error renaming file");
      LAST_MESSAGE = "Error renaming file";
    } else {
      // Ahora lo cargamos en el tape por si quiero reproducir directamente
      String frpath = "/REC/" + String(newFileName);
      REC_FILENAME = frpath;

      logln("File name: " + String(newFileName));
      logln("");

      delay(125);
    }

    // Cambiamos el sampling rate en el HW
    new_sr.sample_rate = SAMPLING_RATE;
    kitStream.setAudioInfo(new_sr);
    _hmi.writeString("tape.lblFreq.txt=\"" + String(int(SAMPLING_RATE / 1000)) +
                     "KHz\"");
    // Volvemos
    return true;
  }

  void prepareHMI() {
    // Inicializamos variables del HMI
    BLOCK_SELECTED = 0;
    TOTAL_BLOCKS = 0;
    PROGRAM_NAME = "";
    PROGRAM_NAME_2 = "";
    TYPE_FILE_LOAD = "";
    LAST_SIZE = 0;
    strncpy(LAST_TYPE, "", 1);
    strncpy(LAST_NAME, "", 1);
  }

  void initialize() {
    prepareHMI();

    errorInDataRecording = false;
    stopRecordingProccess = false;
    stateInfoBlock = 0;
    // WasfirstStepInTheRecordingProccess = false;
    //  Ponemos a cero todos los indicadores
    _hmi.resetIndicators();
  }

  void terminate(bool partialSave) {
    // ************************************************************************
    //
    // En este procedimiento cerramos el fichero, para validarlo por el SO
    //
    // *************************************************************************

    bool fileWasClosed = false;
    // Ponemos a cero todos los indicadores
    _hmi.resetIndicators();
  }

  TAPrecorder() {}
};