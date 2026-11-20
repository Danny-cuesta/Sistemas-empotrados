//--PRÁCTICA 3 S.EMPOTRADOS


#include <Thread.h> //--> Para ArduinoThread
#include <StaticThreadController.h>
#include <ThreadController.h>
#include <avr/wdt.h> //--> Watchdog
#include <LiquidCrystal.h> //--> Para la pantalla LCD
#include <DHT.h> //--> Para el sensor de temperatura y humedad


//--HILOS NECESARIOS
ThreadController controller; //-->  Maneja y controla los hilos (agrupa varias tareas y decide cuáles toca ejecutar)
Thread threadDistance; //--> Lanza el pulso del ultrasonidos y guarda la distancia en una variable
Thread threadTemperature; //--> Lee temperatura y humedad y las guarda en variables
Thread threadEntradaJoystick; //--> Mira el joystick y el botón externo, y apunta “ha pasado esto”
Thread threadPantalla; //--> Dibuja en el LCD lo que toque según el estado actual
Thread threadMaquina; //--> Decide en qué estado está el sistema y cuándo se cambia de uno a otro, máquina de estados

//--VARIABLES QUE INDICAN EL ESTADO EN EL QUE ESTÁ EL SISTEMA y lo que pueden hacer los distintos modos
enum Modo{
  ARRANQUE,
  SERVICIO,
  ADMIN
};

enum SERVICIO{
  ESPERANDO,
  ENTORNO,
  CATALOGO,
  PREPARANDO,
  RETIRAR
};

enum ADMIN{
  MENU,
  VER_TEMP,
  VER_DIST,
  VER_CONT,
  PRECIOS_LISTA,
  PRECIOS_EDIT
};


//--Variables de los leds
const byte ledVerdePin = 3;
const byte ledAzulPin = 4;

//--Variables del botón
const byte botonPin = 2;
volatile bool pulsado = false;

//--Variables del sensor de ultrasonidos
const byte PIN_TRIGGER = 5;
const byte PIN_ECHO = 6;
const float V_SOUND_AIR = 0.0343f; // cm/us
float distance = 0.0f;
long duration  = 0;

//--Variables del sensor de temperatura
const byte temperaturePin = A0;
DHT dht(dhtPin, DHT11);
float temperatura = 0, humedad = 0;

//--Variables de la pantalla
LiquidCrystal lcd(13, 12, 11, 10, 9, 8);
//                 RS, E, D4, D5, D6, D7
// VSS: GND
// VDD: 5V
// V0: Sirve para controlar el contraste (se conecta al potenciómetro)
// RS: Register Select, le dice a la pantalla si lo que se envía es un comando (borrar, mover cursor...) o un carácter para mostrar
// RW: GND
// E: Enable habilita la lectura de datos
// D4-D7: Por aquí Arduino envía los datos
// A: 5V
// K: GND

//--Variables del joystick
const int pinX = A4;
const int pinY = A5;
const int pinSW = 7;


//-----------------------------------------------------------------------------------------------



//--Función de inicio
void setup() {
  Serial.begin(9600);
  pinMode(ledVerdePin, OUTPUT);
  pinMode(ledAzulPin, OUTPUT);
  pinMode(botonPin, INPUT_PULLUP); //--> Tiene resistencia interna
  pinMode(PIN_TRIGGER, OUTPUT);
  pinMode(PIN_ECHO, INPUT);
  pinMode(pinSW, INPUT_PULLUP);

  digitalWrite(PIN_TRIGGER, LOW); //--> Nos aseguramos que no envíe ninguna señal
  //--Al empezar, todos los leds están apagados
  digitalWrite(ledVerdePin, LOW);
  digitalWrite(ledAzulPin, LOW);

  //--Preparar la pantalla
  lcd.begin(16, 2); //--> Son las dimensiones de la pantalla
  lcd.print("Hola mundo"); //--> Código de ejemplo

  attachInterrupt(digitalPinToInterrupt(botonPin), interrupcionBoton, FALLING); //--> Interrupción para el botón
}




//-----------------------------------------------------------------------------------------------




//--Función de bucle
void loop() {


  //--MODO ARRANQUE


  //--Definición de variables (joystick)
  int x = analogRead(pinX);
  int y = analogRead(pinY);
  int botonJoystick = digitalRead(pinSW);


  //--El LED1 debe parpadear 3 veces en intervalos de 1 segundo

  Serial.print("CARGANDO...");

  for(int i = 0; i < 3; i++){
    digitalWrite(ledVerdePin, HIGH);
    delay(1000);
    digitalWrite(ledVerdePin, LOW);
    delay(1000);
 }

  //--Posteriormente el LED1 debe apagarse por completo
  digitalWrite(ledVerdePin, LOW);


  //--MODO SERVICIO


  //--Para el SENSOR DE ULTRASONIDOS, generamos pulsos en el disparador
  digitalWrite(PIN_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIGGER, LOW);

  //--Leer el pulso de duración y la distancia
  duration = pulseIn(PIN_ECHO, HIGH);
  distance = (duration * V_SOUND_AIR) / 2.0f;

  //--Se enciende el led rojo a una distancia de menos de 5cm
  if (distance < 5){ 
    digitalWrite(ledAzulPin, HIGH);
    digitalWrite(ledVerdePin, HIGH);
  }  

  else{
    Serial.print("ESPERANDO CLIENTE...");
  }


  //--El botón está pulsado
  //if(pulsado){
  //  digitalWrite(ledAzulPin, HIGH);
  //  digitalWrite(ledVerdePin, HIGH);
  //  delay(200); //--> Para evitar el efecto rebote
  //  pulsado = false;  //--> Reset estado del boton
  //}

  //else{
  //  digitalWrite(ledAzulPin, LOW);
  //  digitalWrite(ledVerdePin, LOW);
  //}

  //--Para la pantalla
  //lcd.setCursor(0, 1); //--> Mueve el cursor a la posición donde se va a escribir


}



//-----------------------------------------------------------------------------------------------



//--Función llamada por la interrupción
void interrupcionBoton() {
  pulsado = true;
}

