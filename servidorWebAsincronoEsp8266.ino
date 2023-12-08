/*********
  Rui Santos
  Complete project details at https://randomnerdtutorials.com/esp8266-dht11dht22-temperature-and-humidity-web-server-with-arduino-ide/
*********/

// Import required libraries
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <Hash.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <ESP8266HTTPClient.h>
#include <WiFiClient.h>
#include <ArduinoJson.h>
#include <SoftwareSerial.h>
// #include <SoftwareSerial.h>
// #include <Adafruit_Sensor.h>
// #include <DHT.h>

// Replace with your network credentials
const char* ssid = "mipc";
const char* password = "441!qA03";

#define DHTPIN 5     // Digital pin connected to the DHT sensor

// Uncomment the type of sensor in use:
//#define DHTTYPE    DHT11     // DHT 11
#define DHTTYPE    DHT22     // DHT 22 (AM2302)
//#define DHTTYPE    DHT21     // DHT 21 (AM2301)


// current temperature & humidity, updated in loop()
float t = 0.0;
String ult_foto = "";
int val = 0;
String fecha_foto = "";
String ipServer = "172.22.80.168";
SoftwareSerial miSerial(5, 4);
const int LEDPin = 15;
const int PIRPin = 16;
const int accessPin = 13;
const int noAccessPin = 12;
int acceso = 0;

// Create AsyncWebServer object on port 80
AsyncWebServer server(80);

// Generally, you should use "unsigned long" for variables that hold time
// The value will quickly become too large for an int to store
unsigned long previousMillis = 0;    // will store last time DHT was updated

// Updates DHT readings every 10 seconds
const long interval = 2000;  

const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    body {
      font-family: Arial, sans-serif;
      margin: 0;
      padding: 0;
      background-color: #f2f2f2;
    }

    h2 {
      font-size: 2.5rem;
      color: #333;
    }

    p {
      font-size: 1.8rem;
      color: #666;
    }

    .units {
      font-size: 1.2rem;
      color: #888;
    }

    .dht-labels {
      font-size: 1.5rem;
      vertical-align: middle;
      padding-bottom: 15px;
      color: #555;
    }

    .boton {
      display: inline-block;
      padding: 15px 30px;
      font-size: 1.6rem;
      cursor: pointer;
      text-align: center;
      text-decoration: none;
      outline: none;
      border: none;
      border-radius: 5px;
      transition: background-color 0.3s ease;
    }

    /* Estilos para el botón de encendido */
    .encender {
      background-color: #4CAF50; /* Color verde */
      color: white;
    }

    .encender:hover {
      background-color: #45a049; /* Cambio de color al pasar el mouse */
    }

    /* Estilos para el botón de apagar */
    .apagar {
      background-color: #f44336; /* Color rojo */
      color: white;
    }

    .apagar:hover {
      background-color: #d32f2f; /* Cambio de color al pasar el mouse */
    }

    #last-photo {
      height: auto;
      display: block;
      margin: 20px auto;
      box-shadow: 0 0 10px rgba(0, 0, 0, 0.1);
    }
  </style>
</head>
<body>
  <div>
    <h2>Sistema de gestion de accesos por fototrampeo: <span id="estado">%ESTADO%</span></h2>
    <a href='?status=on'><button class="boton encender">Encender </button></a>
    <a href='?status=off'><button class="boton apagar">Apagar </button></a>
  </div>
  <p>
    <span class="dht-labels">Ultima foto capturada en el sistema</span>
    <span id="fecha-foto">%FECHA%</span>
    <img id="last-photo" src="%LAST%"/>
    <a href='?permitir=si'><button class="boton">Permitir acceso </button></a>
    <a href='?permitir=no'><button class="boton">Denegar acceso </button></a>
  </p>
  <form action="http://192.168.48.215/dashboard/procesar.php" method="post" enctype="multipart/form-data">
    <label for="imagen"
  </form>
</body>
<script>

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("last-photo").src = this.responseText;
    }
  };
  xhttp.open("GET", "/last", true);
  xhttp.send();
}, 5000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("fecha-foto").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/fecha", true);
  xhttp.send();
}, 5000 ) ;
</script>
</html>)rawliteral";

// Replaces placeholder with DHT values
String processor(const String& var){
  //Serial.println(var);
  if(var == "LAST"){
    return String(ult_foto);
  }
  else if (var == "FECHA"){
    return String(fecha_foto);
  }
  else if (var == "ESTADO"){
    if(val) return "ENCENDIDO";
    return "APAGADO";
  }
  return String();
}

void sendPostRequest(String encodedFoto) {
  // URL del servidor y ruta al script PHP que manejará la imagen
  String serverUrl = "http://"+ipServer+"/dashboard/procesar_hex.php";

  // Imagen en formato base64
  String imagenBase64 = encodedFoto;

  // Configurar la solicitud HTTP
  WiFiClient client;
  HTTPClient http;
  http.begin(client, serverUrl);

  // Configurar el tipo de contenido
  http.addHeader("Content-Type", "application/x-www-form-urlencoded");

  // Crear el cuerpo de la solicitud con los datos de la imagen
  String postData = "imagen_base64=" + imagenBase64;

  // Realizar la solicitud POST
  int httpResponseCode = http.POST(postData);

  // Verificar la respuesta del servidor
  if (httpResponseCode > 0) {
    Serial.print("Respuesta del servidor: ");
    Serial.println(httpResponseCode);
    String payload = http.getString();
    Serial.println(payload);
  } else {
    Serial.print("Error en la solicitud. Código de error: ");
    Serial.println(httpResponseCode);
  }

  // Cerrar la conexión
  http.end();
}

void setup(){
  // Serial port for debugging purposes
  // miSerial.begin(9600);
  pinMode(LEDPin, OUTPUT);
  pinMode(accessPin, OUTPUT);
  pinMode(noAccessPin, OUTPUT);
  pinMode(PIRPin, INPUT);

  Serial.begin(115200);
  miSerial.begin(9600);
  miSerial.println("Nuevo serial abierto");
  
  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  Serial.println("Connecting to WiFi");
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println(".");
  }

  // Print ESP8266 Local IP Address
  Serial.println(WiFi.localIP());

  // Route for root / web page
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){

    acceso=0;
    int paramsNr = request->params();
    Serial.println(paramsNr);

    for(int i=0; i<paramsNr;i++){
      AsyncWebParameter* p = request->getParam(i);
      Serial.print("Param name: ");
      Serial.println(p->name());
      Serial.print("Param value: ");
      Serial.println(p->value());
      Serial.println("------");
      if (p->name().equals("status")){
        if(p->value().equals("on")) val=1;
        else if (p->value().equals("off")) val=0;
      }else{
        if(p->name().equals("permitir")){
          if(p->value().equals("si")) acceso=1;
          else if (p->value().equals("no")) acceso=0;
        }
      }
      Serial.print("Val: ");
      Serial.println(val);
    }

    request->send_P(200, "text/html", index_html, processor);
  });
  server.on("/last", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(ult_foto).c_str());
  });
  server.on("/fecha", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", String(fecha_foto).c_str());
  });

  // Start server
  server.begin();

}
 
void loop(){  
  int value = digitalRead(PIRPin);
  if (value == HIGH)
  {

    digitalWrite(LEDPin, HIGH);
    delay(500);
    // digitalWrite(LEDPin, LOW);
  }
  else
  {
    digitalWrite(LEDPin, LOW);
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= interval) {
    previousMillis = currentMillis;
    if (val){
      
      String photoFile="";
      // Realiza la solicitud GET
      String url = "http://"+ipServer+"/dashboard/lastfile.php";  // Reemplaza con la URL de tu servidor y ruta
      if (WiFi.status() == WL_CONNECTED){
        WiFiClient client;
        HTTPClient http;

        String serverPath = url;
        http.begin(client, serverPath);

        int httpResponseCode = http.GET();
        if (httpResponseCode>0){
          Serial.print("HTTP Response code: ");
          Serial.println(httpResponseCode);
          String payload = http.getString();
          Serial.print("PAYLOAD: ");
          Serial.println(payload);
          
          // Tamaño suficientemente grande para contener el JSON y un poco de margen
          const size_t capacity = JSON_OBJECT_SIZE(200) + 60;
          DynamicJsonDocument doc(capacity);

          // Deserializar el JSON
          DeserializationError error = deserializeJson(doc, payload);

          // Verificar si hubo un error durante la deserialización
          if (error) {
            Serial.print("Error al deserializar JSON: ");
            Serial.println(error.c_str());
            return;
          }

          // Obtener valores del JSON
          const char* nombreCompleto = doc["nombre_completo"];
          const char* fecha = doc["fecha"];

          // Imprimir los valores obtenidos
          Serial.print("Nombre completo: ");
          Serial.println(nombreCompleto);
          Serial.print("Fecha: ");
          Serial.println(fecha);
        
          
          photoFile = nombreCompleto;
          fecha_foto = fecha;
          // Imprimir los valores obtenidos
          Serial.print("Nombre completo: ");
          Serial.println(nombreCompleto);
          Serial.print("Fecha: ");
          Serial.println(fecha);


        }else{
          Serial.print("Error code: ");
          Serial.println(httpResponseCode);
        }
        http.end();
      }else{
        Serial.println("WIFI disconnected");
      }


      // La foto
      String newH = "http://"+ipServer+"/dashboard/capturedImages/";
      if(photoFile.isEmpty()) ult_foto = newH+"down.jpg";
      else{
        ult_foto = newH+photoFile;
      } 
      Serial.println(ult_foto);

      Serial.println("Aceso: " + String(acceso));
      if (acceso == 1){
        digitalWrite(accessPin, HIGH);
        digitalWrite(noAccessPin, LOW);
      }else{
        digitalWrite(accessPin, LOW);
        digitalWrite(noAccessPin, HIGH);
      }

      // Comprobamos si se ha detectado movimiento
      // miSerial.flush();
      miSerial.readString();
      if(value == HIGH){
        // miSerial.flush();
        miSerial.readString();
        Serial.println("Se ha detectado movimiento. Solicitando imagen del intruso.");
        delay(2000);
        miSerial.write(1);
        delay(2000);
        Serial.println("Current buffer: " + String(miSerial.available()));
        // miSerial.flush();
        delay(2000);

        // Esperamos la recepción de informacion
        while (miSerial.available() <= 0){
          Serial.println("Current buffer: " + String(miSerial.available()));
          
          Serial.println("Esperando recepción de imagen...");
          delay(1000);
        }

        // A partir de aqui comienza la transferencia

        String imagen="";
        String mensaje="";
        mensaje = miSerial.readString();
        Serial.println("MENSAJE RECIBIDO: Se enviarán " + mensaje + " paquetes");
        delay(2000);
        miSerial.write(1);
        for(int i=0; i<mensaje.toInt(); i++){
          Serial.println("Se recibe el paquete " + String(i));
          String paquete = miSerial.readString();
          Serial.println("NUevo paquete: " + paquete);
          imagen+=paquete;
          delay(1000);
          miSerial.write(1);
        }
        Serial.println("Mandando foto al servidor...");
        Serial.println("Mensaje completo: "+ imagen);
        sendPostRequest(imagen);
        
        miSerial.flush();
        
        Serial.println("La recepcion ha terminado aqui");
        delay(3000);
      }

      


    }
    else{
        String downPhoto = "http://"+ipServer+"/dashboard/capturedImages/down.jpg";
        ult_foto = downPhoto;
        Serial.println(ult_foto);
    }

    Serial.println(Serial.read());
  }

  

  // String mensaje = "";
  // if(miSerial.available() > 0){
    
  //   String imagen="";
  //   mensaje = miSerial.readString();
  //   Serial.println("MENSAJE RECIBIDO: " + mensaje);
  //   delay(2000);
  //   miSerial.write(1);
  //   for(int i=0; i<mensaje.toInt(); i++){
  //     Serial.println("Se recibe el paquete " + String(i));
  //     String paquete = miSerial.readString();
  //     Serial.println("NUevo paquete: " + paquete);
  //     imagen+=paquete;
  //     delay(1000);
  //     miSerial.write(1);
  //   }
  //   Serial.println("Mandando foto al servidor...");
  //   Serial.println("Mensaje completo: "+ imagen);
  //   sendPostRequest(imagen);
    
  //   miSerial.flush();
    
  //   Serial.println("La recepcion ha terminado aqui");
  //   delay(3000);

  // }

}