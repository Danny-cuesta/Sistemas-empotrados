//--PRÁCTICA 3 S.EMPOTRADOS


#include <Thread.h> //--> Para ArduinoThread
#include <StaticThreadController.h>
#include <ThreadController.h>
#include <avr/wdt.h> //--> Watchdog
#include <LiquidCrystal.h> //--> Para la pantalla LCD
#include <DHT.h> //--> Para el sensor de temperatura y humedad

#define DHTTYPE DHT11 // 3. Definir el modelo exacto del sensor (DHT11)

//--HILOS NECESARIOS
ThreadController controller; //-->  Maneja y controla los hilos (agrupa varias tareas y decide cuáles toca ejecutar)
Thread threadDistance; //--> Lanza el pulso del ultrasonidos y guarda la distancia en una variable
Thread threadTemperature; //--> Lee temperatura y humedad y las guarda en variables
Thread threadJoystickBoton; //--> Mira el joystick y el botón externo, y apunta “ha pasado esto”
Thread threadLCD; //--> Dibuja en el LCD lo que toque según el estado actual
Thread threadMachine; //--> Decide en qué estado está el sistema y cuándo se cambia de uno a otro, máquina de estados

//--VARIABLES QUE INDICAN EL ESTADO EN EL QUE ESTÁ EL SISTEMA y lo que pueden hacer los distintos modos
enum Modo{
  ARRANQUE,
  SERVICIO,
  ADMIN
};

enum Servicio{
  ESPERANDO,
  ENTORNO,
  CATALOGO,
  PREPARANDO,
  RETIRAR
};

enum Admin{
  MENU,
  VER_TEMP,
  VER_DIST,
  VER_CONT,
  PRECIOS_LISTA,
  PRECIOS_EDIT
};

//--Inicialización de las instancias de los enumeradores
Modo modo = ARRANQUE;
Servicio servicio = ESPERANDO;
Admin admin = MENU;
unsigned long tEstado = 0;   //--> Sirve para controlar el tiempo del estado actual


//--Variables de los leds
const byte ledVerdePin = 3;
const byte ledAzulPin = 4;

//--Variables del sensor de ultrasonidos
const byte PIN_TRIGGER = 5;
const byte PIN_ECHO = 6;
const float V_SOUND_AIR = 0.0343f; // cm/us
float distance = 999.0f; //--> Lo inicizalizamos a una distancia grande porque indica "no hay nadie a la vista"
long duration = 0;

//--Variables del sensor de temperatura
const byte dhtPin = A0;
DHT dht(dhtPin, DHT11);
float temperature = 0, humidity = 0;

//--Variables de la pantalla y lo que se imprimirá en ella
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

//--Variables del catálogo
const char* products[5] = {"Cafe Solo", "Cafe Cortado", "Cafe Doble", "Cafe Premium", "Chocolate"};
int costs[5] = {100, 110, 125, 150, 200};
unsigned long durationPrep = 0; //--> Indica cuanto durará la preparación de la bebida
byte selection = 0; //--> Producto marcado en el catálogo

//--Variables del menú admin
const char* opcionesAdmin[4] = {"Ver temperatura", "Ver distancia", "Ver contador", "Modif. precios"};
byte menuSel = 0; //--> Opción marcada del menú principal del admin
byte priceSel = 0; //--> Producto marcado en "Modificar precios"
int priceEdit = 0; //--> Copia del coste del producto seleccionado para editarlo

//--Variables del joystick
const int pinX = A4;
const int pinY = A5;
const int pinSW = 7;

//--Variables del botón
const byte botonPin = 2;
volatile bool pulsado = false;
volatile unsigned long tPulsado = 0;
volatile unsigned long tSoltado = 0;

//--Variables que indican cambios de eventos del joystick y del botón externo
bool evUp = false;
bool evDown = false;
bool evLeft = false;
bool evSelection = false;
bool evCancelar = false; //--> Evento que indica que se encuentra en estado de reinicio
//--Variables prev
bool prevUp = false;
bool prevDown = false;
bool prevLeft = false;
bool prevSelection = false;
bool prevBoton = false;

bool evAdmin = false;
bool admin1Time = false; //--> Para que cada pulsación larga, admin se active una vez
//unsigned long durationBoton = 0; //--> Para comprobar la duración del botón

//--Parpadeo del arranque
const unsigned long T_MEDIO_CICLO = 500;  //--> El LED1 cambia de estado cada 500ms (porque parpadea, se enciende y se apaga, cada 1s)
byte cambiosLed = 0; //--> Contador de los cambios de los leds


//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________



//--FUNCIONES EXTERNAS NECESARIAS DEL SISTEMA


//--Función para ir de un estado a otro
void irA(Servicio s){
  servicio = s;
  tEstado = millis();
}

//--------------------------------------------------


//--Función que devuelve el sistema al punto de partida
void reiniciarServicio(){
  digitalWrite(ledVerdePin, LOW);
  digitalWrite(ledAzulPin, LOW);
  irA(ESPERANDO);
}


//--------------------------------------------------


//--Función que imprime en la pantalla una fila, rellenando con espacios hasta 16 caracteres
void printFilaLCD(byte fila, const char* caracter){
  char buffer[17]; //--> Reserva 16 caracteres + \0, para preparar la linea antes de imprimirla en la pantalla
  //--> Se reserva en memoria y así cada impresión no se sobrescribe de mala manera, cada linea sale limpia
  snprintf(buffer, sizeof(buffer), "%-16s", caracter); //--> Función para formatear y guardar texto dentro de un array de caracteres
  //--> buffer: Dónde escribir
  //--> sizeof(buffer): Cuánto cabe como máximo (17), así nunca se escribe de más
  //--> "%-16s": El formato
  //--> caracter: El texto que se mete en el formato
  lcd.setCursor(0, fila); //--> Coloca el texto siempre a la izquierda y en la fila que se indique
  lcd.print(buffer); //--> Imprimimos
}



//--------------------------------------------------------------------------------------------------------------


//--FUNCIONES HILOS DEL SISTEMA



//--Función que LEE la distancia
void readDistance(){

  //--Para el SENSOR DE ULTRASONIDOS, generamos pulsos en el disparador
  digitalWrite(PIN_TRIGGER, LOW); //--> Nos aseguramos de que no haya ninguna señal y que el HIGH salga limpio
  delayMicroseconds(2);
  digitalWrite(PIN_TRIGGER, HIGH);
  delayMicroseconds(10);
  digitalWrite(PIN_TRIGGER, LOW);

  //--Leer el pulso de duración y la distancia
  duration = pulseIn(PIN_ECHO, HIGH, 6000UL); //--> 6000UL es el límite de tiempo de espera

  if (duration == 0){ //--> No ha devuelto ningún eco (no ha rebotado la señal), no hay nadie
    distance = 999.0f;
  }
  else{
    distance = (duration * V_SOUND_AIR) / 2.0f;
  }

}


//--------------------------------------------------------------------------------------------------------------


//--Función que LEE la temperatura y la humedad
void readDht(){

  //--Variables que leen cada cosa
  float t = dht.readTemperature();
  float h = dht.readHumidity();

  //--Comprobar que los números son válidos (NaN significa Not a Number)
  if(!isnan(t)){
    temperature = t;
  }
  if(!isnan(h)){
    humidity = h;
  }

}


//--------------------------------------------------------------------------------------------------------------



//--Función que lee la entrada del joystick y del botón
void readJoystickBoton(){

  //--Definición de variables (joystick)
  int x = analogRead(pinX);
  int y = analogRead(pinY);
  int botonJoystick = digitalRead(pinSW);
  bool up = false;
  bool down = false;
  bool left = false;
  bool selection = (digitalRead(pinSW) == LOW); //--> Se define con LOW porque está confirgurado con INPUT_PULLUP y se invierte la lógica

  //--Evaluar los eventos segun los valores del joystick
  if (y < 300){ //--> Arriba
    up = true;
  }
  if (y > 700){ //--> Abajo
    down = true;
  }
  if (x < 300){ //--> izquierda
    left = true;
  }

  //--Un evento solo cuando se pasa de "reposo" a "movido", si solo se cambia una vez debe pasar un producto
  if(up && !prevUp){
    evUp = true;
  }
  if(down && !prevDown){
    evDown = true;
  }
  if(left && !prevLeft){
    evLeft = true;
  }
  if(selection && !prevSelection){
    evSelection = true;
  }
  //--Definir valor de los eventos anteriores
  prevUp = up;
  prevDown = down;
  prevLeft = left;
  prevSelection = selection;
  

  //--Botón externo para entrar en el modo admin (si está presionado más de 5 segundos)
  noInterrupts(); //--> Desactivamos las interrupciones por un momento para poder copiar los valores correctamente
  bool bot = pulsado;
  unsigned long t0 = tPulsado;
  unsigned long t1 = tSoltado;
  interrupts(); //--> Se vuelven a activar las interrupciones del código

  //--5s seguidos pulsado: entrar o salir de Admin
  if (bot  && !admin1Time && millis() - t0 >= 5000){
    evAdmin = true;
    admin1Time = true;
  }

  //--Comprobar si al pulsar el botón, se quiso reiniciar el sistema
  if (!bot && prevBoton){
    unsigned long d = t1 - t0;
    if (d >= 2000 && d <= 3000){ //--> Si se pulsó de 2 a 3s, reiniciar sistema
      evCancelar = true;
    }
  }

  //--Al soltar, se prepara la siguiente pulsación (siempre después de la comprobación anterior)
  if (!bot){
    admin1Time = false;
  }

  prevBoton = bot; //--> Actualizamos el pulsado anterior del boton

}


//--------------------------------------------------------------------------------------------------------------



//--Función que imprime en la pantalla lo que toque segun el estado actual
void printLCD(){
  char buf[17];

  switch (modo){

    case ARRANQUE:
      printFilaLCD(0, "CARGANDO...");
      printFilaLCD(1, "");
      break;


    case SERVICIO:

      switch (servicio){ //--> Contemplar cada modo del servicio
        case ESPERANDO:
          printFilaLCD(0, "ESPERANDO");
          printFilaLCD(1, "CLIENTE...");
          break;

        case ENTORNO:
          snprintf(buf, sizeof(buf), "Temp: %d %cC", (int)temperature, 223); //--> 223 = símbolo "º" para los grados
          printFilaLCD(0, buf);
          snprintf(buf, sizeof(buf), "Hum: %d %%", (int)humidity);
          printFilaLCD(1, buf);
          break;

        case CATALOGO:
          printFilaLCD(0, products[selection]); //--> Imprimimos la selección marcada
          snprintf(buf, sizeof(buf), "%d.%02d EUR", costs[selection] / 100, costs[selection] % 100); //--> Formatea de céntimos a euros (de dos decimales)
          printFilaLCD(1, buf);
          break;

        case PREPARANDO:
          printFilaLCD(0, "PREPARANDO");
          snprintf(buf, sizeof(buf), "%s...", products[selection]); //--> Nombre de la selección
          printFilaLCD(1, buf);
          break;

        case RETIRAR:
          printFilaLCD(0, "RETIRE SU BEBIDA.");
          printFilaLCD(1, "");
          break;

      }
      break;


    case ADMIN: 

      switch (admin){

        case MENU:
          snprintf(buf, sizeof(buf), ">%s", opcionesAdmin[menuSel]); //--> La marcada lleva ">"
          printFilaLCD(0, buf);
          if (menuSel < 3){ //--> Si no se ha llegado al final, seguir imprimiendo en este formato
            snprintf(buf, sizeof(buf), " %s", opcionesAdmin[menuSel + 1]); //--> La siguiente, debajo
            printFilaLCD(1, buf);
          }
          else{
            printFilaLCD(1, ""); //--> Si no hay más opciones, no se imprime nada debajo
          }
          break;


        case VER_TEMP:
          snprintf(buf, sizeof(buf), "Temp: %d %cC", (int)temperature, 223);
          printFilaLCD(0, buf);
          snprintf(buf, sizeof(buf), "Hum: %d %%", (int)humidity);
          printFilaLCD(1, buf);
          break;


        case VER_DIST:
          printFilaLCD(0, "Distancia:");
          snprintf(buf, sizeof(buf), "%d cm", (int)distance);
          printFilaLCD(1, buf);
          break;


        case VER_CONT:
          printFilaLCD(0, "Contador:");
          snprintf(buf, sizeof(buf), "%lu s", millis() / 1000); //--> / 1000: Pasa el millis a segundos
          printFilaLCD(1, buf);
          break;


        case PRECIOS_LISTA:
          printFilaLCD(0, products[priceSel]);
          snprintf(buf, sizeof(buf), "%d.%02d EUR", costs[priceSel] / 100, costs[priceSel] % 100); //--> Formatea a euros
          printFilaLCD(1, buf);
          break;


        case PRECIOS_EDIT:
          printFilaLCD(0, products[priceSel]);
          snprintf(buf, sizeof(buf), "Nuevo: %d.%02d EUR", priceEdit / 100, priceEdit % 100); //--> Formatea a euros
          printFilaLCD(1, buf);
          break;

      }
      break;

  }

}


//--------------------------------------------------------------------------------------------------------------



//--Función que actua como máquina de estados y decide cuando el sistema cambia de uno a otro
void stateMachine(){

  //--PRUEBA DEL WATCHDOG (borrar antes de entregar)
  //  El sistema se recupera solo cuando se bloquea
  //if (evSelection && modo == ADMIN){
  //  while (true);
  //}


  //--Comprobar si está en modo admin
  if(evAdmin){ //--> Se ha pulsado previamente el botón durante 5 segundos

    //--Cambiar de estado en función de si está en uno o en otro
    if (modo == ADMIN){ //--> Salir de Admin
      modo = SERVICIO;
      digitalWrite(ledVerdePin, LOW);
      digitalWrite(ledAzulPin, LOW);
      reiniciarServicio();
    }
    else if (modo == SERVICIO){ //--> Entrar en admin
      modo = ADMIN;
      admin = MENU;
      //--Se encienden los dos leds
      digitalWrite(ledVerdePin, HIGH);
      digitalWrite(ledAzulPin, HIGH);
    }
  }
  else if(evCancelar && modo == SERVICIO){ //--> Reiniciar sistema
    reiniciarServicio();
  }


  //--Evaluar los valores del joysitck
  switch (modo){

    case ARRANQUE:

      //--¿Han pasado 500 ms desde la última vez que cambió el LED?
      if (millis() - tEstado >= T_MEDIO_CICLO){
        tEstado = millis(); //--> Apuntar la "hora" actual para empezar a contar los siguientes 500ms

        //--Invertir el estado del LED, medimos los cambios
        int estadoActual = digitalRead(ledAzulPin);
        digitalWrite(ledAzulPin, !estadoActual);
        cambiosLed++;

        //--Comprobar los cambios
        if (cambiosLed >= 6){ //--> Se enciende (cambio 1), Se apaga (cambio 2) -->  3 * 2 = 6 cambios (se ha encendido 3 veces)
          digitalWrite(ledAzulPin, LOW);
          modo = SERVICIO;
          irA(ESPERANDO); //--> Comenzamos esperando a clientes
        }
      }
      break;


    case SERVICIO:

      switch(servicio){ //--> Contemplar cada modo del servicio
        case ESPERANDO:
          //--Se detecta a un cliente si la distancia es inferior a 1m
          if(distance < 100){
            irA(ENTORNO);
          }
          break;


        case ENTORNO:
          //--Reservar 5 segundos para ver la tempetarura y la humedad (eso lo imprimirá la función printLCD())
          if(millis() - tEstado >= 5000){ //--> Cuando pasen los 5s, ir al catálogo de productos
            irA(CATALOGO);
          }
          break;


        case CATALOGO:
          //--Si se detecta que el joystick gira hacia arriba, la selección del producto disminuye (es la ANTERIOR) 
          if(evUp && selection > 0){
            selection--;
          }
          //--Hacia abajo, aumenta (la siguiente selección)
          if(evDown && selection < 4){
            selection++;
          }
          //--Se ha seleccionado un producto (selección activa)
          if(evSelection){
            durationPrep = random(4000, 8001); //--> Elegir un número random del tiempo estimado (para que no todos tarden igual)
            irA(PREPARANDO);
          }
          break;


        case PREPARANDO:{
          unsigned long t = millis() - tEstado;
          //--Definimos el brillo del led
          long brillo = map((long)t, 0, (long)durationPrep, 0, 255);
          analogWrite(ledVerdePin, constrain(brillo, 0, 255)); //--> El led aumentará progresivamente su brillo hasta obtener el máximo (durante el tiempo que tenemos)
          //--Si se ha llegado a la duración correspondiente, retirar bebida 
          if(t >= durationPrep){
            analogWrite(ledVerdePin, 0); //--> Apagar el led
            irA(RETIRAR);
          }
        }
          break;


        case RETIRAR:
          //--Esperar 3s y volver a detectar clientes
          if(millis() - tEstado >= 3000){
            irA(ESPERANDO);
          }
          break;

      }
      break;


    case ADMIN:

      switch(admin){
        case MENU:
          //--Si se detecta que el joystick gira hacia arriba, la selección del producto disminuye (es la ANTERIOR) 
          if(evUp && menuSel > 0){
            menuSel--;
          }
          //--Hacia abajo, aumenta (la siguiente selección)
          if(evDown && menuSel < 3){
            menuSel++;
          }
          //--Se ha seleccionado un producto (selección activa)
          if(evSelection){
            admin = (Admin)(menuSel + 1); //--> Saltar a la siguiente selección del menú
          }
          break;


        //--No indicamos nada porque solo hay que imprimir
        //  * CUANDO VARIOS CASE VAN SEGUIDOS SIN NINGÚN BREAK, COMPARTEN EL MISMO CÓDIGO *
        case VER_TEMP:
        case VER_DIST:
        case VER_CONT:
          //--Si se detecta que el joystick gira a la izquierda, volver al menu (en cualquiera de los casos anteriores)
          if(evLeft){
            admin = MENU;
          }
          break;


        case PRECIOS_LISTA:
          if(evUp && priceSel > 0){
            priceSel--;
          }
          //--Hacia abajo, aumenta (la siguiente selección)
          if(evDown && priceSel < 4){
            priceSel++;
          }
          //--Se ha seleccionado un producto (selección activa)
          if(evSelection){
            priceEdit = costs[priceSel]; //--> Copia el precio actual para editarlo
            admin = PRECIOS_EDIT;
          }
          //--Volver al estado anterior
          if(evLeft){
            admin = MENU;
          }
          break;


        case PRECIOS_EDIT:
          if (evUp){
            priceEdit += 5; //--> +0,05€
          }
          if (evDown && priceEdit >= 4){ //--> (priceEdit >= 5: El precio no puede quedarse nuna en negativo)
            priceEdit -= 5; //--> -0,05€
          }
          if (evSelection){ //--> Confirmamos
            costs[priceSel] = priceEdit; //--> Ahora modificamos el valor antiguo por el nuevo en la lista original
            admin = PRECIOS_LISTA; //--> Volvemos
          }
          //--Volver al estado anterior
          if(evLeft){
            admin = PRECIOS_LISTA;
          }
          break;

      }
      break;

  }

  //--Resetear variables
  evUp = evDown = evLeft = evSelection = false;
  evAdmin = evCancelar = false;

}



//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________




//--FUNCIÓN DE INICIO
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

  dht.begin(); //--> Preparar sensor de temperatura

  attachInterrupt(digitalPinToInterrupt(botonPin), interrupcionBoton, CHANGE); //--> Interrupción para el botón


  //--Configurar los hilos y añadirlos a la cola del controlador (scheduler)
  threadDistance.onRun(readDistance);
  threadDistance.setInterval(200);

  threadTemperature.onRun(readDht);
  threadTemperature.setInterval(2000);

  threadJoystickBoton.onRun(readJoystickBoton);
  threadJoystickBoton.setInterval(30);

  threadLCD.onRun(printLCD);
  threadLCD.setInterval(200);

  threadMachine.onRun(stateMachine);
  threadMachine.setInterval(50);

  controller.add(&threadDistance);
  controller.add(&threadTemperature);
  controller.add(&threadJoystickBoton);
  controller.add(&threadLCD);
  controller.add(&threadMachine);

  modo = ARRANQUE;
  tEstado = millis();
  randomSeed(analogRead(A1)); //--> Semilla distinta en cada arranque

  wdt_enable(WDTO_2S); //--> WATCHDOG: Si pasan 2 s sin refrescar, la placa se reinicia


}




//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________





//--FUNCIÓN DEL BUCLE PRINCIPAL
void loop() {

  controller.run(); //--> Ejecuta cada hilo en función de como lo tenga colocado en la cola
  wdt_reset(); //--> Reiniciar la placa del watchdog
  //--> Si el watchdog lleva 2s sin que nadie lo refresque, reinicia el microcontrolador

}



//_________________________________________________________________________________________________________
//_________________________________________________________________________________________________________




//--Función llamada por la interrupción
void interrupcionBoton() {

  if (digitalRead(botonPin) == LOW){
    tPulsado = millis(); //--> Guarda el tiempo en el que se pulsó
    pulsado = true;
  }
  else{
    tSoltado = millis(); //--> Guarda el tiempo en el que se soltó
    pulsado = false;
  }

}




