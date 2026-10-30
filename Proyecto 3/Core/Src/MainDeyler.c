/*
  PARQUEO-MATIC V2 - ESP32 coordinador central
  Proyecto 3 / Electronica Digital 2

  Topologia:
    ESP32     : maestro I2C, servidor HTTP+WebSocket
    NUCLEO 1 : esclavo I2C 0x31; sensores P1-P4, 4 NeoPixels, TFT
    NUCLEO 2 : esclavo I2C 0x32; sensores P5-P8, 4 NeoPixels, 7 segmentos

  ESP32 NO controla NeoPixels ni sensores ni display.

  Protocolo:
    ESP32 lee de CADA NUCLEO 3 bytes:
       [0] 0x5A
       [1] mascara local (bits 0..3, 1=ocupado)
       [2] 0x5A XOR mascara_local
    ESP32 escribe a CADA NUCLEO 4 bytes:
       [0] 0xA5
       [1] mascara GLOBAL (P1=bit 0 ... P8=bit 7)
       [2] mascara VALIDEZ (bit 1=estado conocido)
       [3] 0xA5 XOR mascara_global XOR mascara_validez

  En la Nucleo 1, mascara local bit0..3 = P1..P4;
  en la Nucleo 2, mascara local bit0..3 = P5..P8.

  Librerias: Wire.h, WiFi.h, WebServer.h (Arduino-ESP32)
             WebSocketsServer.h (instalar "WebSockets" de Markus Sattler)
*/

#include <Arduino.h>
#include <WiFi.h>
#include <Wire.h>
#include <WebServer.h>
#include <WebSocketsServer.h>

// ================== 1. CONFIGURACION ==================
// true = simulacion desde la web (sin necesidad de tener Nucleos conectadas)
// false = lee sensores reales de AMBAS Nucleos por I2C
constexpr bool MODO_SIMULACION = true;

const char* WIFI_SSID = "iPhone 18 Pro Max de Deyler";
const char* WIFI_PASSWORD = "holahola";
const char* AP_SSID = "ParqueoMatic";
const char* AP_PASSWORD = "ParqueoMatic26";

constexpr uint8_t SDA_ESP = 21;
constexpr uint8_t SCL_ESP = 22;
constexpr uint8_t DIR_NUCLEO_TFT = 0x31;  // P1..P4
constexpr uint8_t DIR_NUCLEO_7SEG = 0x32; // P5..P8
constexpr uint32_t VELOCIDAD_I2C = 100000;
constexpr uint32_t PERIODO_I2C_MS = 150;  // ~6.7 consultas/segundo
constexpr uint32_t TIMEOUT_NUCLEO_MS = 1500;
constexpr uint8_t CAB_LOCAL = 0x5A;
constexpr uint8_t CAB_GLOBAL = 0xA5;

WebServer servidor(80);
WebSocketsServer sockets(81);

// Cada mascara usa bits 0..3.
uint8_t mascaraLocalA = 0;
uint8_t mascaraLocalB = 0;
uint8_t mascaraGlobal = 0;
uint8_t mascaraValidez = 0;
uint32_t ultimaRespuestaA = 0;
uint32_t ultimaRespuestaB = 0;
bool haRespondidoA = false;
bool haRespondidoB = false;
uint32_t ultimaConsulta = 0;

// ================== 2. WEB INTEGRADA ==================
const char PAGINA_WEB[] PROGMEM = R"HTML(
<!DOCTYPE html>
<html lang="es">
<head>
<meta charset="utf-8">
<meta name="viewport" content="width=device-width,initial-scale=1">
<title>Parqueo-matic | Centro de control</title>
<style>
:root{font-family:system-ui,Segoe UI,Arial,sans-serif;color-scheme:dark}
*{box-sizing:border-box}body{margin:0;background:#0b1220;color:#ecf1f8}
main{max-width:960px;margin:auto;padding:26px 16px 50px}
header{display:flex;align-items:center;justify-content:space-between;flex-wrap:wrap;gap:12px}
h1{margin:0;font-size:clamp(1.5rem,4vw,2.3rem)}
.sub{color:#a4b4c8;margin:8px 0 21px}.chip{border:1px solid #4b5d72;color:#d2dfec;border-radius:50px;padding:9px 12px;font-size:.9rem}
.resumen{display:grid;grid-template-columns:repeat(3,1fr);gap:11px}
.stat{background:#172339;border:1px solid #283c55;border-radius:14px;padding:15px}
.stat span{display:block;color:#a4b5cb;font-size:.9rem}.stat strong{font-size:2.3rem}
.sector{display:flex;justify-content:space-between;align-items:center;gap:10px;margin:28px 0 12px}
.sector h2{margin:0;font-size:1.08rem}.etiqueta{color:#b9c9db;font-size:.9rem}
.plazas{display:grid;grid-template-columns:repeat(4,1fr);gap:12px}
.plaza{text-align:center;border:2px solid #596477;border-radius:14px;background:#16243a;padding:16px 8px;min-height:125px}
.plaza strong{font-size:1.65rem;display:block;margin-bottom:14px}
.plaza.libre{background:#102d23;border-color:#22c55e}
.plaza.ocupado{background:#381d29;border-color:#ef4444}
.plaza.desconocido{background:#31291b;border-color:#e6a63e}
.plaza small{display:block;font-size:.88rem}
button{border:0;border-radius:8px;background:#e8edf6;color:#172339;padding:8px 11px;cursor:pointer;margin-top:12px;font-weight:700}
.nota{border-radius:12px;background:#142138;border:1px solid #28405c;padding:14px 17px;color:#c1cee0;margin-top:22px;line-height:1.55}
@media(max-width:620px){.plazas{grid-template-columns:repeat(2,1fr)}.stat strong{font-size:1.9rem}.stat{padding:12px}}
</style>
</head>
<body>
<main>
<header><h1>PARQUEO-MATIC</h1><span id="conexion" class="chip">Conectando...</span></header>
<p class="sub">Monitor central de los ocho espacios, dos Nucleo STM32 y ESP32</p>
<section class="resumen">
 <div class="stat"><span>Disponibles</span><strong id="libres">—</strong></div>
 <div class="stat"><span>Ocupados</span><strong id="ocupados">—</strong></div>
 <div class="stat"><span>Total</span><strong>8</strong></div>
</section>
<div class="sector"><h2>Sector A · Nucleo 1 / TFT</h2><span id="nucleoA" class="etiqueta">Sin datos</span></div>
<div id="plazasA" class="plazas"></div>
<div class="sector"><h2>Sector B · Nucleo 2 / 7 segmentos</h2><span id="nucleoB" class="etiqueta">Sin datos</span></div>
<div id="plazasB" class="plazas"></div>
<div class="nota"><div id="modo">Esperando datos...</div><div id="evento">Ultimo cambio: ninguno.</div></div>
</main>
<script>
let socket, anterior=null, validezAnterior=null;
const $ = id=>document.getElementById(id);
function render(d){
 $('conexion').textContent=d.simulacion?'SIMULACION':(d.completo?'2 NUCLEO CONECTADAS':'I2C INCOMPLETO');
 $('libres').textContent=d.completo?d.libres:'—';
 $('ocupados').textContent=d.completo?d.ocupados:'—';
 $('nucleoA').textContent=d.simulacion?'Simulado':(d.nucleo1?'Comunicacion OK':'SIN COMUNICACION');
 $('nucleoB').textContent=d.simulacion?'Simulado':(d.nucleo2?'Comunicacion OK':'SIN COMUNICACION');
 $('modo').textContent=d.simulacion?'Modo simulacion: los botones cambian el estado virtual.':
   (d.completo?'Los 8 estados provienen de las dos Nucleo.':
   'Esperando la informacion de ambas Nucleo; total no disponible.');
 for(let i=0;i<8;i++){
   const conocido=!!(d.validez & (1<<i));
   const ocupado=!!(d.mask & (1<<i));
   const clase=conocido?(ocupado?'ocupado':'libre'):'desconocido';
   const texto=conocido?(ocupado?'Ocupado':'Disponible'):'Desconocido';
   const contenedor=i<4?$('plazasA'):$('plazasB');
   let tarjeta=$('p'+i);
   if(!tarjeta){tarjeta=document.createElement('div');tarjeta.id='p'+i;contenedor.appendChild(tarjeta)}
   tarjeta.className='plaza '+clase;
   tarjeta.innerHTML='<strong>P'+(i+1)+'</strong><small>'+texto+'</small>'+
     (d.simulacion?'<button onclick="alternar('+(i+1)+')">Cambiar estado</button>':'');
 }
 if (anterior!==null && d.validez===validezAnterior && anterior!==d.mask){
    const cambios=[];
    for(let i=0;i<8;i++) if((d.validez&(1<<i)) && ((anterior^d.mask)&(1<<i)))
      cambios.push('P'+(i+1)+': '+((d.mask&(1<<i))?'ocupado':'disponible'));
    if(cambios.length) $('evento').textContent='Ultimo cambio: '+cambios.join(', ');
 }
 anterior=d.mask;validezAnterior=d.validez;
}
async function consultar(){try{const r=await fetch('/api/estado',{cache:'no-store'});if(r.ok)render(await r.json())}catch(e){}}
async function alternar(n){try{const r=await fetch('/api/toggle?p='+n,{method:'POST'});if(r.ok)render(await r.json())}catch(e){}}
function conectar(){socket=new WebSocket('ws://'+location.hostname+':81/');
 socket.onmessage=e=>{try{render(JSON.parse(e.data))}catch(err){}};
 socket.onclose=()=>setTimeout(conectar,2000);
 socket.onerror=()=>socket.close();}
consultar();conectar();setInterval(consultar,5000);
</script>
</body>
</html>
)HTML";

// ================== 3. ESTADO GLOBAL ==================
uint8_t contarBits(uint8_t m) {
  uint8_t n=0;
  for (uint8_t i=0;i<8;i++) if (m & (1U<<i)) n++;
  return n;
}

bool nucleoConectada(bool recibida, uint32_t cuando) {
  return recibida && ((uint32_t)(millis()-cuando) <= TIMEOUT_NUCLEO_MS);
}

bool nucleoAConectada() {
  return MODO_SIMULACION || nucleoConectada(haRespondidoA, ultimaRespuestaA);
}

bool nucleoBConectada() {
  return MODO_SIMULACION || nucleoConectada(haRespondidoB, ultimaRespuestaB);
}

// Devuelve true solo cuando cambio mascara o validez.
bool reconstruirGlobal() {
  if (MODO_SIMULACION) return false;
  uint8_t nuevaValidez = 0;
  if (nucleoAConectada()) nuevaValidez |= 0x0F;
  if (nucleoBConectada()) nuevaValidez |= 0xF0;
  const uint8_t nuevaMascara = (mascaraLocalA & 0x0F) | ((mascaraLocalB & 0x0F) << 4);
  const bool cambio = (nuevaValidez != mascaraValidez) ||
                      ((nuevaMascara & nuevaValidez) != (mascaraGlobal & nuevaValidez));
  mascaraValidez = nuevaValidez;
  mascaraGlobal = nuevaMascara;
  return cambio;
}

String jsonEstado() {
  const bool completo = (mascaraValidez == 0xFF);
  const uint8_t ocupados = contarBits(mascaraGlobal);
  char buf[270];
  char libresTxt[8];
  char ocupadosTxt[8];
  if (completo) {
    snprintf(libresTxt, sizeof(libresTxt), "%u", (unsigned)(8-ocupados));
    snprintf(ocupadosTxt, sizeof(ocupadosTxt), "%u", (unsigned)ocupados);
  } else {
    strcpy(libresTxt, "null");
    strcpy(ocupadosTxt, "null");
  }
  snprintf(buf,sizeof(buf),
    "{\"mask\":%u,\"validez\":%u,\"libres\":%s,\"ocupados\":%s,\"completo\":%s,\"nucleo1\":%s,\"nucleo2\":%s,\"simulacion\":%s}",
    mascaraGlobal,mascaraValidez,
    libresTxt, ocupadosTxt,
    completo ? "true" : "false",
    nucleoAConectada() ? "true" : "false",
    nucleoBConectada() ? "true" : "false",
    MODO_SIMULACION ? "true" : "false");
  return String(buf);
}

void publicarEstado() {
 String mensaje = jsonEstado();
sockets.broadcastTXT(mensaje);
  Serial.printf("[GLOBAL] mask=0x%02X, validez=0x%02X, libres=%s\n",
    mascaraGlobal,mascaraValidez,
    mascaraValidez==0xFF ? String(8-contarBits(mascaraGlobal)).c_str() : "DESCONOCIDO");
}

// ================== 4. I2C MAESTRO ==================
bool leerCuatroParqueos(uint8_t direccion, uint8_t &mascaraLocal) {
  // La Nucleo responde cuando el maestro hace una lectura de 3 bytes.
  const uint8_t n = Wire.requestFrom(direccion, (uint8_t)3, true);
  if (n != 3 || Wire.available() < 3) {
    while (Wire.available()) Wire.read();
    return false;
  }
  const int cabecera=Wire.read();
  const int datos=Wire.read();
  const int verificacion=Wire.read();
  if (cabecera!=CAB_LOCAL || (datos & 0xF0)!=0 ||
      verificacion!=(CAB_LOCAL ^ datos)) return false;
  mascaraLocal=(uint8_t)(datos & 0x0F);
  return true;
}

bool enviarGlobal(uint8_t direccion) {
  Wire.beginTransmission(direccion);
  Wire.write(CAB_GLOBAL);
  Wire.write(mascaraGlobal);
  Wire.write(mascaraValidez);
  Wire.write((uint8_t)(CAB_GLOBAL ^ mascaraGlobal ^ mascaraValidez));
  return Wire.endTransmission(true) == 0;
}

void actualizarI2C() {
  if ((uint32_t)(millis()-ultimaConsulta) < PERIODO_I2C_MS) return;
  ultimaConsulta = millis();

  if (!MODO_SIMULACION) {
    uint8_t local;
    if (leerCuatroParqueos(DIR_NUCLEO_TFT, local)) {
      mascaraLocalA=local;haRespondidoA=true;ultimaRespuestaA=millis();
    }
    if (leerCuatroParqueos(DIR_NUCLEO_7SEG, local)) {
      mascaraLocalB=local;haRespondidoB=true;ultimaRespuestaB=millis();
    }
    if (reconstruirGlobal()) publicarEstado();
  }

  // Se distribuye a las DOS Nucleos el estado de todos los sensores.
  // Incluimos validez para que nadie muestre un total equivocado
  // si uno de los microcontroladores deja de responder.
  enviarGlobal(DIR_NUCLEO_TFT);
  enviarGlobal(DIR_NUCLEO_7SEG);
}

// ================== 5. HTTP Y WEBSOCKETS ==================
void alternarParqueo() {
  if (!MODO_SIMULACION) {
    servidor.send(403,"text/plain","Simulacion desactivada");
    return;
  }
  const int p=servidor.hasArg("p")?servidor.arg("p").toInt():0;
  if(p<1||p>8){servidor.send(400,"text/plain","Parqueo invalido");return;}
  mascaraGlobal ^= (uint8_t)(1U << (p-1));
  mascaraValidez = 0xFF;
  publicarEstado();
  servidor.sendHeader("Cache-Control","no-store");
  servidor.send(200,"application/json",jsonEstado());
}

void eventoSocket(uint8_t cliente, WStype_t tipo, uint8_t* payload, size_t largo) {
  (void)payload;(void)largo;
if (tipo == WStype_CONNECTED) {
    String mensaje = jsonEstado();
    sockets.sendTXT(cliente, mensaje);
}
}

void iniciarServidor() {
  servidor.on("/",HTTP_GET,[](){servidor.send_P(200,"text/html; charset=utf-8",PAGINA_WEB);});
  servidor.on("/api/estado",HTTP_GET,[](){
    servidor.sendHeader("Cache-Control","no-store");
    servidor.send(200,"application/json",jsonEstado());
  });
  servidor.on("/api/toggle",HTTP_POST,alternarParqueo);
  servidor.onNotFound([](){servidor.send(404,"text/plain","Pagina no encontrada");});
  servidor.begin();
  sockets.begin();
  sockets.onEvent(eventoSocket);
  Serial.println("[WEB] HTTP:80 y WebSockets:81 activos");
}

// ================== 6. WIFI ==================
void iniciarWiFi() {
  const bool configurado = String(WIFI_SSID)!="NOMBRE_DE_TU_WIFI";
  if(configurado) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID,WIFI_PASSWORD);
    Serial.printf("[WiFi] Conectando a %s",WIFI_SSID);
    uint32_t desde=millis();
    while(WiFi.status()!=WL_CONNECTED && (uint32_t)(millis()-desde)<12000) {
      delay(250);Serial.print(".");
    }
    Serial.println();
    if(WiFi.status()==WL_CONNECTED) {
      Serial.print("[WEB] http://");Serial.println(WiFi.localIP());
      return;
    }
    WiFi.disconnect(true);
    Serial.println("[WiFi] No disponible; creando red propia");
  }
  WiFi.mode(WIFI_AP);
  if(WiFi.softAP(AP_SSID,AP_PASSWORD)) {
    Serial.print("[WiFi] Red: ");Serial.println(AP_SSID);
    Serial.print("[WEB] http://");Serial.println(WiFi.softAPIP());
  } else Serial.println("[WiFi] ERROR iniciando punto de acceso");
}

// ================== 7. SETUP / LOOP ==================
void setup() {
  Serial.begin(115200);
  delay(250);
  Serial.println("\n=== PARQUEO-MATIC V2 | ESP32 MAESTRO ===");
  Serial.printf("Modo: %s\n",MODO_SIMULACION ? "SIMULACION" : "REAL");

  // El ESP32 inicia como MAESTRO I2C. No usa Wire.onReceive.
  if(Wire.begin((int)SDA_ESP,(int)SCL_ESP,VELOCIDAD_I2C)) {
    Wire.setTimeOut(25);
    Serial.println("[I2C] Maestro SDA=21, SCL=22, 100 kHz");
    Serial.println("[I2C] Esclavos: Nucleo TFT=0x31 y Nucleo 7seg=0x32");
  } else Serial.println("[I2C] ERROR: no se pudo iniciar bus");

  if(MODO_SIMULACION) mascaraValidez=0xFF;
  iniciarWiFi();
  iniciarServidor();
  publicarEstado();
}

void loop() {
  actualizarI2C();
  servidor.handleClient();
  sockets.loop();
  delay(2);
}
