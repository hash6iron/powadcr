#pragma once

//#include "WebServer.h"
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <SPIFFS.h>
#include "SdFat.h"

class powaFileServer
{
    private:

        AsyncWebServer *_server;
        SdFat32 _sdf32;
        File32 _mFile;

        String listFiles(bool ishtml) 
        {
            String returnText = "";
            log("Listing files stored on SPIFFS");
            File root = SPIFFS.open("/");
            File foundfile = root.openNextFile();

            if (ishtml) 
            {
                returnText += "<table><tr><th align='left'>Name</th><th align='left'>Size</th></tr>";
            }
            
            while (foundfile) 
            {
                if (ishtml) 
                {
                    returnText += "<tr align='left'><td>" + String(foundfile.name()) + "</td><td>" + humanReadableSize(foundfile.size()) + "</td></tr>";
                } 
                else 
                {
                    returnText += "File: " + String(foundfile.name()) + "\n";
                }
                
                foundfile = root.openNextFile();
            }
            
            if (ishtml) 
            {
                returnText += "</table>";
            }
            
            root.close();
            foundfile.close();
            
            return returnText;
        }

        // Make size of files human readable
        // source: https://github.com/CelliesProjects/minimalUploadAuthESP32
        String humanReadableSize(const size_t bytes) {
        if (bytes < 1024) return String(bytes) + " B";
        else if (bytes < (1024 * 1024)) return String(bytes / 1024.0) + " KB";
        else if (bytes < (1024 * 1024 * 1024)) return String(bytes / 1024.0 / 1024.0) + " MB";
        else return String(bytes / 1024.0 / 1024.0 / 1024.0) + " GB";
        }

        // void configureWebServer() 
        // {
        //     _server->on("/", HTTP_GET, [](AsyncWebServerRequest * request) 
        //     {
        //         String logmessage = "Client:" + request->client()->remoteIP().toString() + + " " + request->url();
        //         log(logmessage);
        //         request->send_P(200, "text/html", index_html, processor);
        //     });

        //     // run handleUpload function when any file is uploaded
        //     _server->on("/upload", HTTP_POST, [](AsyncWebServerRequest *request) 
        //     {
        //         request->send(200);
        //     }, handleUpload);
        // }

        // // handles uploads
        // static void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) 
        // {
        //     String logmessage = "Client:" + request->client()->remoteIP().toString() + " " + request->url();
        //     log(logmessage);

        //     if (!index) 
        //     {
        //         logmessage = "Upload Start: " + String(filename);
        //         // open the file on first call and store the file handle in the request object
        //         request->_tempFile = SPIFFS.open("/" + filename, "w");
        //         log(logmessage);
        //     }

        //     if (len) 
        //     {
        //         // stream the incoming chunk to the opened file
        //         request->_tempFile.write(data, len);
        //         logmessage = "Writing file: " + String(filename) + " index=" + String(index) + " len=" + String(len);
        //         log(logmessage);
        //     }

        //     if (final) 
        //     {
        //         logmessage = "Upload Complete: " + String(filename) + ",size: " + String(index + len);
        //         // close the file handle as the upload is now done
        //         request->_tempFile.close();
        //         log(logmessage);
        //         request->redirect("/");
        //     }
        // }

        // static String processor(const String& var) 
        // {
        //     if (var == "FILELIST") 
        //     {
        //         return listFiles(true);
        //     }
            
        //     if (var == "FREESPIFFS") 
        //     {
        //         return humanReadableSize((SPIFFS.totalBytes() - SPIFFS.usedBytes()));
        //     }

        //     if (var == "USEDSPIFFS") 
        //     {
        //         return humanReadableSize(SPIFFS.usedBytes());
        //     }

        //     if (var == "TOTALSPIFFS") 
        //     {
        //         return humanReadableSize(SPIFFS.totalBytes());
        //     }

        //     return String();
        // }

    public:

        void beginServer()
        {
            File32 root;
        }

        powaFileServer(AsyncWebServer *server, SdFat32 sdf)
        {   
            // Constructor de la clase
            _server = server;
            _sdf32 = sdf;
        }
};