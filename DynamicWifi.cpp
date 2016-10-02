#include "DynamicWifi.h"

DynamicWifi::DynamicWifi(char* ssid)
    :server(new WiFiServer(80)) {
  WiFi.softAP(ssid);
  debug_println("Creating AP with ssid: ");
  debug_println(ssid);
  debug_print("Soft-AP IP address = ");
  debug_println(WiFi.softAPIP());
  server->begin();
}

DynamicWifi::~DynamicWifi() {
  delete server;
  WiFi.softAPdisconnect(true);
  debug_println("Wifi off");
}

String httpProtocolNotSupported() {
  String response = String("HTTP/1.1 505");
}

bool DynamicWifi::tryConfigure() {
  WiFiClient client = server->available();
  if (client) {
    debug_println("Client connected");
    while (client.connected()) {
      if (client.available()) {
        String verb = client.readStringUntil(' ');
        String path = client.readStringUntil(' ');
        String protocol = client.readStringUntil('\r');
        client.read();
        {
          String header;
          do {
            header = client.readStringUntil('\r');
            client.read();
            // Get headers here
          } while (header.length() > 0);
        }
        String body = client.readString();

        debug_print("Verb: ");
        debug_println(verb);
        debug_print("Path: ");
        debug_println(path);
        debug_print("Protocol: ");
        debug_println(protocol);
        debug_print("Body: ");
        debug_println(body);

        if (protocol != "HTTP/1.1") {
          status(client, 505); // HTTP Version Not Supported
          break;
        }

        if (verb.equals("GET")) {
          handleGet(client);
        } else if (verb.equals("POST")) {
          handlePost(client, body);
        } else {
          status(client, 405); // Method not allowed
        }
        break;
      }
    }
    delay(1);
    client.stop();
    debug_println("\nClient disconnected");
  }
  return ssid.length() > 0 && password.length() > 0;
}

void DynamicWifi::handleGet(WiFiClient client) {
  String htmlPage =
   String("HTTP/1.1 200 OK\r\n") +
          "Content-Type: text/html\r\n" +
          "Connection: close\r\n" +  // the connection will be closed after completion of the response
          "\r\n" +
          "<!DOCTYPE HTML>" +
          "<html>" +
          "<body>"
          "<form method=\"post\">"
          "SSID:<br>"
          "<input type=\"text\" name=\"ssid\">"
          "<br>"
          "Password:<br>"
          "<input type=\"password\" name=\"password\">"
          "<br>"
          "<input type=\"submit\" value=\"Submit\">"
          "</form>"
          "</body>"
          "</html>" +
          "\r\n";
  client.print(htmlPage);
}

void DynamicWifi::handlePost(WiFiClient client, String body) {
  int i = 0;
  do {
    int indexOfAmp = body.indexOf('&', i);
    String keyVal = "";
    if (indexOfAmp > 0) {
      keyVal = body.substring(i, indexOfAmp);
      i = indexOfAmp + 1;
      debug_println("PreKey");
    } else {
      keyVal = body.substring(i, body.length());
      debug_println("PostKey");
      i = body.length();
    }
    debug_print("KeyVal: ");
    debug_println(keyVal);
    if (keyVal.startsWith("ssid=")) {
      ssid = keyVal.substring(5, keyVal.length());
    } else if (keyVal.startsWith("password=")) {
      password = keyVal.substring(9, keyVal.length());
    } else {
      debug_print("Ignoring ");
      debug_println(keyVal);
    }
  } while(i < body.length());
  client.print("HTTP/1.1 ");
  if (ssid.length() <= 0 || password.length() <= 0) {
    status(client, 400);
  } else {
    status(client, 200);
    client.print("Network '");
    client.print(ssid);
    client.print("' saved!");
  }
}

void DynamicWifi::status(WiFiClient client, short status) {
  client.print("HTTP/1.1 ");
  client.print(status);
  client.print(" \r\n\r\n");
}

String DynamicWifi::getSsid() {
  return ssid;
}

String DynamicWifi::getPassword() {
  return password;
}