/*
Nombre del proyecto: Sistema IoT con comunicación MQTT
Autor: Manganiello Maximiliano
Fecha: 14/06/2026

Descripción:
Este programa implementa un dispositivo IoT basado en una placa
NodeMCU-32S con microcontrolador ESP32.

El sistema establece una conexión WiFi y se comunica con un broker
MQTT público. A través de diferentes tópicos es posible enviar las
lecturas del joystick, controlar un LED RGB y mantener un chat entre
los dispositivos conectados.

La comunicación se realiza mediante el protocolo MQTT utilizando
el broker HiveMQ.
*/

#include <WiFi.h>
#include <PubSubClient.h>

// Credenciales de la red WiFi
const char *ssid = "Nombre-WiFi";
const char *password = "Contraseña";

// Dirección del broker MQTT
const char *mqtt_server = "broker.hivemq.com";

// Tópicos para transmisión del joystick
const char *TOPIC_X    = "iot/maxi2026/joystick/x";
const char *TOPIC_Y    = "iot/maxi2026/joystick/y";
const char *TOPIC_SW   = "iot/maxi2026/joystick/sw";

// Tópicos para control del LED RGB y chat
const char *TOPIC_RGB  = "iot/maxi2026/rgb";
const char *TOPIC_CHAT = "iot/maxi2026/chat";

// Objetos para la conexión WiFi y MQTT
WiFiClient espClient;
PubSubClient client(espClient);

// Pines del LED RGB
#define RGB_R 25
#define RGB_G 26
#define RGB_B 27

// Pines del joystick
#define JOY_X 34
#define JOY_Y 35
#define JOY_SW 32

// Variable utilizada para controlar el tiempo entre envíos
unsigned long ultimoEnvio = 0;

// Función ejecutada cada vez que se recibe un mensaje MQTT
void callback(char* topic, byte* payload, unsigned int length) {

  String mensaje = "";

  // Conversión del mensaje recibido a String
  for (int i = 0; i < length; i++) {
    mensaje += (char)payload[i];
  }

  // Procesamiento de mensajes del chat
  if (String(topic) == TOPIC_CHAT) {

    if (mensaje == "X" || mensaje == "Y" || mensaje == "SW" || mensaje == "JOY") {

        Serial.print("COMANDO -> ");
        Serial.println(mensaje);

      } else {

        Serial.print("CHAT -> ");
        Serial.println(mensaje);
      }
    
    // Consulta de la posición del eje X
    if (mensaje == "X") {

      int x = analogRead(JOY_X);

      if (x > 3000)
        client.publish(TOPIC_CHAT, "ESP32: X = ARRIBA");

      else if (x < 1000)
        client.publish(TOPIC_CHAT, "ESP32: X = ABAJO");

      else
        client.publish(TOPIC_CHAT, "ESP32: X = CENTRO");
    }

    // Consulta de la posición del eje Y
    else if (mensaje == "Y") {

      int y = analogRead(JOY_Y);

      if (y > 3000)
        client.publish(TOPIC_CHAT, "ESP32: Y = DERECHA");

      else if (y < 1000)
        client.publish(TOPIC_CHAT, "ESP32: Y = IZQUIERDA");

      else
        client.publish(TOPIC_CHAT, "ESP32: Y = CENTRO");
    }

    // Consulta del pulsador del joystick
    else if (mensaje == "SW") {

      if (digitalRead(JOY_SW) == 0) {

        client.publish(
          TOPIC_CHAT,
          "ESP32: SW = PRESIONADO"
        );

      } else {

        client.publish(
          TOPIC_CHAT,
          "ESP32: SW = SIN PRESIONAR"
        );
      }
    }

    // Envío del estado completo del joystick
    else if (mensaje == "JOY") {

      String respuesta =
        "ESP32: X=" + String(analogRead(JOY_X)) +
        " Y=" + String(analogRead(JOY_Y)) +
        " SW=" + String(digitalRead(JOY_SW));

      client.publish(
        TOPIC_CHAT,
        respuesta.c_str()
      );
    }

    return;
  }

  // Procesamiento de comandos para el LED RGB
  if (String(topic) == TOPIC_RGB) {
    Serial.print("COMANDO -> ");
    Serial.println(mensaje);

    if (mensaje == "RED") {
    
      digitalWrite(RGB_R, HIGH);
      digitalWrite(RGB_G, LOW);
      digitalWrite(RGB_B, LOW);

    } else if (mensaje == "GREEN") {

      digitalWrite(RGB_R, LOW);
      digitalWrite(RGB_G, HIGH);
      digitalWrite(RGB_B, LOW);

    } else if (mensaje == "BLUE") {

      digitalWrite(RGB_R, LOW);
      digitalWrite(RGB_G, LOW);
      digitalWrite(RGB_B, HIGH);

    } else if (mensaje == "WHITE") {

      digitalWrite(RGB_R, HIGH);
      digitalWrite(RGB_G, HIGH);
      digitalWrite(RGB_B, HIGH);

    } else if (mensaje == "OFF") {

      digitalWrite(RGB_R, LOW);
      digitalWrite(RGB_G, LOW);
      digitalWrite(RGB_B, LOW);
    }
  }
}

// Establece la conexión con la red WiFi
void conectarWiFi() {

  Serial.print("Conectando WiFi");

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {

    delay(500);
    Serial.print(".");
  }

  Serial.println();
  Serial.println("WiFi conectado");
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());
}

// Establece la conexión con el broker MQTT
void conectarMQTT() {

  while (!client.connected()) {

    Serial.print("Conectando MQTT...");

    String clientId = "ESP32_";
    clientId += String(random(1000, 9999));

    if (client.connect(clientId.c_str())) {

      Serial.println("OK");

      // Suscripción a los tópicos necesarios
      client.subscribe(TOPIC_RGB);
      client.subscribe(TOPIC_CHAT);

      client.publish(TOPIC_CHAT, "ESP32 conectada");

    } else {

      Serial.print("Error: ");
      Serial.println(client.state());

      delay(2000);
    }
  }
}

void setup() {

  Serial.begin(115200);

  // Configuración de pines
  pinMode(RGB_R, OUTPUT);
  pinMode(RGB_G, OUTPUT);
  pinMode(RGB_B, OUTPUT);

  pinMode(JOY_SW, INPUT_PULLUP);

  // Conexión a la red WiFi
  conectarWiFi();

  // Configuración del broker MQTT
  client.setServer(mqtt_server, 1883);

  // Asociación de la función callback
  client.setCallback(callback);
}

void loop() {

  // Reconexión automática en caso de pérdida de conexión
  if (!client.connected()) {
    conectarMQTT();
  }

  client.loop();

  // Envío de mensajes del chat desde el monitor serie
  if (Serial.available()) {

    String texto = Serial.readStringUntil('\n');

    texto.trim();

    if (texto.length() > 0) {

      client.publish(
        TOPIC_CHAT,
        ("ESP32: " + texto).c_str()
      );
    }
  }

  // Envío periódico del estado del joystick
  if (millis() - ultimoEnvio > 250) {

    ultimoEnvio = millis();

    int x = analogRead(JOY_X);
    int y = analogRead(JOY_Y);
    int sw = digitalRead(JOY_SW);

    client.publish(TOPIC_X, String(x).c_str());
    client.publish(TOPIC_Y, String(y).c_str());
    client.publish(TOPIC_SW, String(sw).c_str());
  }
}
