/*
 * Adafruit.ino
 *
 * Created:
 * Author:
 * Description:
 */
/****************************************/
// Encabezado (Libraries)

#include "WiFi.h"
#include "AdafruitIO_WiFi.h"

// Credenciales
#define WIFI_SSID     "Hola Causas"
#define WIFI_PASS     "Puente2019"
#define IO_USERNAME   "Gigi_PuenteAF"
#define IO_KEY        "aio_skJP090sMGJPvZjquLp2C6tYd7tE"

// Configuracion UART
#define MAESTRO_RX_PIN         16
#define MAESTRO_TX_PIN         17
#define MAESTRO_BAUDRATE       9600
#define SERIAL_BUFFER_SIZE     96

// Configuracion de publicacion
#define PUBLISH_INTERVAL_MS    15000UL

// Objetos globales de Adafruit IO
AdafruitIO_WiFi io(IO_USERNAME, IO_KEY, WIFI_SSID, WIFI_PASS);
AdafruitIO_Group *telemetryGroup = io.group("telemetry");

// Variables globales del ESP32
char serialBuffer[SERIAL_BUFFER_SIZE];
uint8_t serialIndex = 0U;
unsigned long lastPublishMs = 0UL;

int latestBoxes = 0;
int latestTempC = -127;
int latestAlarm = 0;
int latestDetectMs = 0;
float latestDetectS = 0.0f;
int latestStepperState = 0;
int latestStepperCycles = 0;
int latestButton = 0;
bool telemetryDirty = false;

/****************************************/
// Function prototypes

// Parseo de telemetria
static bool extractIntField(const char *line, const char *key, int *value);
static void parseTelemetry(const char *line);

// Publicacion a Adafruit
static void publishTelemetryIfDue(void);

// Recepcion UART
static void processMaestroSerial(void);

/****************************************/
// Main Function

void setup() {
  Serial.begin(115200);
  Serial1.begin(MAESTRO_BAUDRATE, SERIAL_8N1, MAESTRO_RX_PIN, MAESTRO_TX_PIN);

  Serial.println();
  Serial.println("Conectando a Adafruit IO...");

  io.connect();

  while (io.status() < AIO_CONNECTED) {
    Serial.print(".");
    delay(500);
  }

  Serial.println();
  Serial.println(io.statusText());
  Serial.println("Adafruit IO conectado.");

  // Verificacion del grupo telemetry
  if (!telemetryGroup->exists()) {
    Serial.println("Creando grupo telemetry...");
    telemetryGroup->create();
  }

  lastPublishMs = millis();
}

void loop() {
  // Principal del ESP32
  io.run();
  processMaestroSerial();
  publishTelemetryIfDue();
}

/****************************************/
// NON-Interrupt subroutines

// Parseo de telemetria
static bool extractIntField(const char *line, const char *key, int *value) {
  const char *start = strstr(line, key);
  char *endPtr;

  if (start == NULL) {
    return false;
  }

  start += strlen(key);
  *value = (int)strtol(start, &endPtr, 10);
  return (endPtr != start);
}

static void parseTelemetry(const char *line) {
  int parsedValue;

  if (extractIntField(line, "boxes=", &parsedValue)) {
    latestBoxes = parsedValue;
  }

  if (extractIntField(line, "temp_c=", &parsedValue)) {
    latestTempC = parsedValue;
  }

  if (extractIntField(line, "alarm=", &parsedValue)) {
    latestAlarm = parsedValue;
  }

  if (extractIntField(line, "detect_ms=", &parsedValue)) {
    latestDetectMs = parsedValue;
    latestDetectS = ((float)latestDetectMs) / 1000.0f;
  }

  if (extractIntField(line, "stepper=", &parsedValue)) {
    latestStepperState = parsedValue;
  }

  if (extractIntField(line, "cycles=", &parsedValue)) {
    latestStepperCycles = parsedValue;
  }

  if (extractIntField(line, "button=", &parsedValue)) {
    latestButton = parsedValue;
  }

  telemetryDirty = true;
}

// Publicacion a Adafruit
static void publishTelemetryIfDue(void) {
  if (!telemetryDirty) {
    return;
  }

  if (millis() - lastPublishMs < PUBLISH_INTERVAL_MS) {
    return;
  }

  telemetryGroup->set("boxes", latestBoxes);
  telemetryGroup->set("temp_c", latestTempC);
  telemetryGroup->set("alarm", latestAlarm);
  telemetryGroup->set("detect_ms", latestDetectMs);
  telemetryGroup->set("detect_s", latestDetectS);
  telemetryGroup->set("stepper", latestStepperState);
  telemetryGroup->set("cycles", latestStepperCycles);
  telemetryGroup->set("button", latestButton);

  if (telemetryGroup->save()) {
    Serial.println("Adafruit IO: datos publicados.");
    lastPublishMs = millis();
    telemetryDirty = false;
  } else {
    Serial.println("Adafruit IO: fallo al publicar.");
  }
}

// Recepcion UART
static void processMaestroSerial(void) {
  while (Serial1.available() > 0) {
    char incoming = (char)Serial1.read();

    if (incoming == '\r') {
      continue;
    }

    if (incoming == '\n') {
      serialBuffer[serialIndex] = '\0';

      if (serialIndex > 0U) {
        Serial.print("UART -> ");
        Serial.println(serialBuffer);
        parseTelemetry(serialBuffer);
      }

      serialIndex = 0U;
      continue;
    }

    if (serialIndex < (SERIAL_BUFFER_SIZE - 1U)) {
      serialBuffer[serialIndex++] = incoming;
    } else {
      serialIndex = 0U;
    }
  }
}

/****************************************/
// Interrupt routines

// Este programa no usa rutinas de interrupcion.
