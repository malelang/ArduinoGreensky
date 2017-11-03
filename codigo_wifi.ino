#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <EEPROM.h>
#include <WiFiClient.h> 
#include <DHT11.h>


//-------------------VARIABLES GLOBALES--------------------------
int contconexion = 0;
unsigned long previousMillis = 0;
const char* hostServer="192.168.0.7";
int pin=2;
DHT11 dht11(pin);
float temp, hum;
int humedad;

char ssid[50];      
char pass[50];

const char *ssidConf = "NodeMCU";
const char *passConf = "12345678";

String mensaje = "";
String respuesta="1";
String url = "/enviardatos.php?"; 

//-----------CODIGO HTML PAGINA DE CONFIGURACION---------------
String pagina = "<!DOCTYPE html>"
"<html>"
"<head>"
"<title>Tutorial Eeprom</title>"
"<meta charset='UTF-8'>"
"</head>"
"<body>"
"</form>"
"<form action='guardar_conf' method='get'>"
"SSID:<br><br>"
"<input class='input1' name='ssid' type='text'><br>"
"PASSWORD:<br><br>"
"<input class='input1' name='pass' type='password'><br><br>"
"<input class='boton' type='submit' value='GUARDAR'/><br><br>"
"</form>"
"<a href='escanear'><button class='boton'>ESCANEAR</button></a><br><br>";

String paginafin = "</body>"
"</html>";

//------------------------SETUP WIFI-----------------------------
void setup_wifi() {
// Conexión WIFI
  WiFi.mode(WIFI_STA); //para que no inicie el SoftAP en el modo normal
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED and contconexion <50) { //Cuenta hasta 50 si no se puede conectar lo cancela
    ++contconexion;
    delay(250);
    Serial.print(".");
    digitalWrite(13, HIGH);
    delay(250);
    digitalWrite(13, LOW);
  }
  if (contconexion <50) {   
      Serial.println("");
      Serial.println("WiFi conectado");
      Serial.println(WiFi.localIP());
      digitalWrite(13, HIGH);  
  }
  else { 
      Serial.println("");
      Serial.println("Error de conexion");
      digitalWrite(13, LOW);
  }
}

//--------------------------------------------------------------
WiFiClient espClient;
ESP8266WebServer server(80);
//--------------------------------------------------------------

//-------------------PAGINA DE CONFIGURACION--------------------
void paginaconf() {
  server.send(200, "text/html", pagina + mensaje + paginafin); 
}

//--------------------MODO_CONFIGURACION------------------------
void modoconf() {
   
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);
  delay(100);
  digitalWrite(13, HIGH);
  delay(100);
  digitalWrite(13, LOW);

  WiFi.softAP(ssidConf, passConf);
  IPAddress myIP = WiFi.softAPIP(); 
  Serial.print("IP del acces point: ");
  Serial.println(myIP);
  Serial.println("WebServer iniciado...");

  server.on("/", paginaconf); //esta es la pagina de configuracion

  server.on("/guardar_conf", guardar_conf); //Graba en la eeprom la configuracion

  server.on("/escanear", escanear); //Escanean las redes wifi disponibles
  
  server.begin();

  while (true) {
      server.handleClient();
  }
}

//---------------------GUARDAR CONFIGURACION-------------------------
void guardar_conf() {
  
  Serial.println(server.arg("ssid"));//Recibimos los valores que envia por GET el formulario web
  grabar(0,server.arg("ssid"));
  Serial.println(server.arg("pass"));
  grabar(50,server.arg("pass"));

  mensaje = "Configuracion Guardada...";
  paginaconf();
}

//----------------Función para grabar en la EEPROM-------------------
void grabar(int addr, String a) {
  int tamano = a.length(); 
  char inchar[50]; 
  a.toCharArray(inchar, tamano+1);
  for (int i = 0; i < tamano; i++) {
    EEPROM.write(addr+i, inchar[i]);
  }
  for (int i = tamano; i < 50; i++) {
    EEPROM.write(addr+i, 255);
  }
  EEPROM.commit();
}

//-----------------Función para leer la EEPROM------------------------
String leer(int addr) {
   byte lectura;
   String strlectura;
   for (int i = addr; i < addr+50; i++) {
      lectura = EEPROM.read(i);
      if (lectura != 255) {
        strlectura += (char)lectura;
      }
   }
   return strlectura;
}

//---------------------------ESCANEAR----------------------------
void escanear() {  
  int n = WiFi.scanNetworks(); //devuelve el número de redes encontradas
  Serial.println("escaneo terminado");
  if (n == 0) { //si no encuentra ninguna red
    Serial.println("no se encontraron redes");
    mensaje = "no se encontraron redes";
  }  
  else
  {
    Serial.print(n);
    Serial.println(" redes encontradas");
    mensaje = "";
    for (int i = 0; i < n; ++i)
    {
      // agrega al STRING "mensaje" la información de las redes encontradas 
      mensaje = (mensaje) + "<p>" + String(i + 1) + ": " + WiFi.SSID(i) + " (" + WiFi.RSSI(i) + ") Ch: " + WiFi.channel(i) + " Enc: " + WiFi.encryptionType(i) + " </p>\r\n";
      //WiFi.encryptionType 5:WEP 2:WPA/PSK 4:WPA2/PSK 7:open network 8:WPA/WPA2/PSK
      delay(10);
    }
    Serial.println(mensaje);
    paginaconf();
  }
}
//---------------------- Función para Enviar Datos a la Base de Datos SQL -------------------
void gestionar_registros()
{
  int flag=0;

    int out2=0;
    //conecta al servidor
    WiFiClient cliente;
    Serial.print("\n\nSe realiza la conexion");
    Serial.print("\n\nConectando al Servidor:");
   
    while(!out2)
    {
      if(!cliente.connect(hostServer,80))
      {
         Serial.print(" .");
         delay(500);
         
      }
      else
      {
         Serial.print("\nConectado al Servidor");
         out2=!out2;
      }
    }
    //arma datos para enviar al servidor según el parámetro a
     Serial.print("\n\nPreparando datos");
     String data=armar_datos();
     Serial.print("\n\nEnviando Peticion");
    //hace petición al servidor
    
       
    String peticion="POST " + url +data+ " HTTP/1.0\r\n"+"Host: "+hostServer+"\r\n"+"Accept: *"+"/"+"*\r\n"+"Content-Length: "+ data.length()+"\r\n"+
               "Content-Type: application/x-www-form-urlencoded\r\n"+"\r\n temp";
    
    cliente.print(peticion);
    //recibe respuesta
    Serial.print("\n\nRespuesta: ");
    while(cliente.available())
    {
       respuesta=String(cliente.readString());
       Serial.print("\n\n"+respuesta);     
    }
    respuesta=respuesta.substring(respuesta.length()-1,respuesta.length());
    flag++;
    cliente.stop();
    
  


  delay(5000);
}// end gestionar_registros

//----------------- ARMAR DATOS --------------------------
String armar_datos()
{
   String data="";
   int val;
   
     dht11.read(hum, temp);
     Serial.print(hum);
     Serial.print(temp);
     String tempString=String(temp);
     String humString=String(hum);
     data = "temp="+tempString+"&"+"hum="+humString;
     Serial.print(data);
      
   return data;
}//end armar_datos

//------------------------SETUP-----------------------------
void setup() {

  pinMode(13, OUTPUT); // D7 
  pinMode(4, INPUT); // D2
  
  // Inicia Serial
  Serial.begin(115200);
  Serial.println("");

  EEPROM.begin(512);

  pinMode(14, INPUT);  //D5
  if (digitalRead(14) == 0) {
    modoconf();
  }

  leer(0).toCharArray(ssid, 50);
  leer(50).toCharArray(pass, 50);

  setup_wifi();
}

//--------------------------LOOP--------------------------------
void loop() {

  unsigned long currentMillis = millis();

  if (currentMillis - previousMillis >= 5000) { //envia la temperatura cada 5 segundos
    previousMillis = currentMillis;
   Serial.print("funcionando...");
   gestionar_registros();
  }
}
