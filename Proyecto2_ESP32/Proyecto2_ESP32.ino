#include <Bluepad32.h>
//Pines del enlace UART
#define PIN_RX_STM32 26
#define PIN_TX_STM32 25
#define UART_BAUDRATE 115200
//Bits enviados por UART
#define CONTROL_VALIDO 0x80
#define CONTROL_IZQUIERDA 0x01
#define CONTROL_DERECHA 0x02
#define CONTROL_SALTO 0x04
#define CONTROL_DISPARO 0x08
#define CONTROL_PAUSA 0x10
//Mascaras del control PS4
#define DPAD_IZQUIERDA 0x08
#define DPAD_DERECHA 0x04
#define BOTON_OPTIONS 0x04
#define UMBRAL_JOYSTICK 180
#define ENVIO_INTERVALO_MS 20
HardwareSerial enlaceSTM32(2);
ControllerPtr controlPS4 = nullptr;
uint32_t ultimoEnvio = 0;
//Enviar estado actual
void EnviarEstado(uint8_t estado)
{
  enlaceSTM32.write(estado);
}
//Detectar nuevo control
void ControlConectado(ControllerPtr control)
{
  if (controlPS4 == nullptr)
  {
    controlPS4 = control;
    controlPS4->setColorLED(0, 0, 255);
    Serial.println("Control PS4 conectado");
  }
}
//Detectar control desconectado
void ControlDesconectado(ControllerPtr control)
{
  if (controlPS4 == control)
  {
    controlPS4 = nullptr;
    EnviarEstado(CONTROL_VALIDO);
    Serial.println("Control PS4 desconectado");
  }
}
//Leer botones del control
uint8_t LeerControl(void)
{
  uint8_t estado = CONTROL_VALIDO;
  if ((controlPS4 == nullptr) ||
      !controlPS4->isConnected() ||
      !controlPS4->isGamepad())
  {
    return estado;
  }
  uint8_t dpad = controlPS4->dpad();
  int32_t joystickX = controlPS4->axisX();
  if (((dpad & DPAD_IZQUIERDA) != 0) ||
      (joystickX < -UMBRAL_JOYSTICK))
  {
    estado |= CONTROL_IZQUIERDA;
  }
  if (((dpad & DPAD_DERECHA) != 0) ||
      (joystickX > UMBRAL_JOYSTICK))
  {
    estado |= CONTROL_DERECHA;
  }
  if (controlPS4->a())
  {
    estado |= CONTROL_SALTO;
  }
  if (controlPS4->x())
  {
    estado |= CONTROL_DISPARO;
  }
  if ((controlPS4->miscButtons() & BOTON_OPTIONS) != 0)
  {
    estado |= CONTROL_PAUSA;
  }
  return estado;
}
void setup()
{
  Serial.begin(115200);
  enlaceSTM32.begin(UART_BAUDRATE, SERIAL_8N1,
                    PIN_RX_STM32, PIN_TX_STM32);
  BP32.setup(&ControlConectado, &ControlDesconectado);
  EnviarEstado(CONTROL_VALIDO);
  Serial.println("Esperando control PS4");
}
void loop()
{
  BP32.update();
  uint32_t ahora = millis();
  if ((ahora - ultimoEnvio) >= ENVIO_INTERVALO_MS)
  {
    ultimoEnvio = ahora;
    EnviarEstado(LeerControl());
  }
  delay(1);
}