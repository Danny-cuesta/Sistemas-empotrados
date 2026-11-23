# PRÁCTICA 3: Controlador máquina expendedora

## Sensores utilizados en la práctica:

• 2 LEDs estándar  
• Pantalla LCD  
• Módulo Joystick  
• Sensor de Temperatura y Humedad DHT11  
• Sensor de Distancia por Ultrasonidos  
• Pulsador / Botón externo  

## Detalles del código y la solución del sistema:

Para esta práctica, he usado Threads para poder controlar varios elementos simultáneamente sin tener que usar delay() y bloquear el programa. Para los modos y estados del sistemas he utilizado enumeradores para poder controlar mejor las acciones de la máquina.

También he añadido el mecanismo de seguridad de **watchdog**, el cual reinicia el sistema si nadie refresca su variable, evitando que el programa se quede en un bucle eterno.
Para probar este mecanismo he puesto estas lineas de código (luego se deben borrar para que todo funcione correctamente):

if (evSelection && modo == ADMIN){  
        while (true);  
}  

Indica que cuando el sistema está en modo admin y pulsamos el botón del joystick, entra en un bucle infinito, pero esto se evita con este mecanismo y se reinicia todo el programa.

## Esquema del circuito (con Fritzing):

<img width="1355" height="962" alt="cap_esquema_empotrados_practica3" src="https://github.com/user-attachments/assets/545f8b44-b82f-4ad4-a416-063e3f367774" />

Cada color del cable representa un sensor.  
El sensor de temperatura está colocado distinto en el circuito real por falta de cables.

## Videos del montaje y su funcionamiento:

(El modo arranque se ve en los demás modos).

### -Modo servicio:

https://github.com/user-attachments/assets/ecd14f1d-a8a1-4b4f-b513-dcffa7cab693

### -Modo admin:
**Navegamos por el menú y vemos las funciones simples:**



**Modificamos los precios de los productos:**



### -Prueba con el Watchdog:

https://github.com/user-attachments/assets/42d172bd-e657-4a76-a225-714c3d2fc7b2

Vemos que cuando activamos el modo admin y pulsamos cualquier selección, el programa se queda congelado por poquito, pero gracias al watchdog, se reinicia. 
