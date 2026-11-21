#include <Wire.h>
#include <Adafruit_VL53L0X.h>

// Definición de Pines y Direcciones XSHUT para los VL53L0X
const int xShutPins[4] = {38, 8, 36, 21};               // [0]: Front, [1]: Back, [2]: Izquierda, [3]: Derecha
uint8_t sensorAddresses[4] = {0x30, 0x31, 0x32, 0x33};  // Nuevas direcciones I2C para evitar colisiones (por defecto es 0x29)

Adafruit_VL53L0X lox[4];

// Definición de Pines para los QTR-1A
const int QTRPins[4] = {10, 6, 2, 13};                  // [0]: Front-Left, [1]: Front-Right, [2]: Back-Left, [3]: Back-Right

// Definición de Pines para el Controlador TB6612FNG y Botón de Inicio
const int PWMA = 1;
const int AIN1 = 3;
const int AIN2 = 5;
const int STBY = 7;
const int BIN1 = 9;
const int BIN2 = 11;
const int PWMB = 12;
const int BTN = 15;

// Banderas, Contadores, Funciones y Otras cosas
const unsigned long fiveSeconds = 5000;
unsigned long currentTime = 0;
unsigned long lastTime = 0;
unsigned long distance[4];    // [0]:Front, [1]:Back, [2]:Izquierda, [3]:Derecha
unsigned int IRstate[4];      // [0]:Front-Left, [1]:Front-Right, [2]:Back-Left, [3]:Back-Right
bool started = false;
bool waiting = true;

void initSensors();
void waitingTheButton();
void readSensors();

void moveMotorA(int speed, bool forward);
void moveMotorB(int speed, bool forward);
void stopMotors();

void setup() {
  Serial.begin(115200);     // Velocidad establecida por Esteban y Sebastian

  // Configuración del I2C en SDA=33 y SCL=36 para evitar contradicciones con la librería
  Wire.begin(33, 35);
  
  // Inicio de sensores VL53L0X y QTR y Configuración de pines TB6612FNG
  initSensors();
  pinMode(PWMA, OUTPUT);
  pinMode(AIN1, OUTPUT);
  pinMode(AIN2, OUTPUT);
  pinMode(STBY, OUTPUT);
  pinMode(BIN1, OUTPUT);
  pinMode(BIN2, OUTPUT);
  pinMode(PWMB, OUTPUT);
  pinMode(BTN, INPUT_PULLUP);

  digitalWrite(STBY, HIGH); // Encendido del driver de motores
  lastTime = millis();
}

// Ahora si comienza lo chido
void loop() {
  currentTime = millis(); // Actualizamos el tiempo global en cada ciclo

  if (waiting) { 
    waitingTheButton();
  }

  if (started) {
    readSensors();

    // --- ZONA DE PRUEBAS TEMPORAL ---
    for (int i = 0; i < 4; i++) {
      Serial.print("Dist["); Serial.print(i); Serial.print("]: "); Serial.print(distance[i]); Serial.print(" cm  |  ");
      Serial.print("QTR["); Serial.print(i); Serial.print("]: "); Serial.println(IRstate[i]);
    }
    Serial.println("-----------------------------------");
    
  // Primer Caso: Condiciones Superficie Negra
  if (started && IRstate[0] >= 6 && IRstate[1] >= 15 && IRstate[2] >= 4000 && IRstate[3] >= 4000) {

    // Condicion 1: Distancias mayores a 60 girar una llanta
    if (distance[0] > 60 && distance[1] > 60 && distance[2] > 60 && distance[4] > 60) {
        moveMotorA(220, true);
        moveMotorB(220, false);
        Serial.println("giro1");
        delay(10);
    }

    // Condicion 2: Distancia enfrente menor a 60 
    else if (distance[0] <= 30 && distance[0] > 1) {
      moveMotorA(250, true);
      moveMotorB(250, true);
      Serial.println("giro2");
      delay(10);
    }

    // Condicion 3: Distancia izquierda menor a 60
    else if (distance[2] <= 30 && distance[2] > 1) {
      moveMotorA(225, true);
      moveMotorB(225, false);
      Serial.println("giro3");
      delay(10);
    }

    // Condicion 4: Distancia derecha menor a 60
    else if (distance[3] <= 30 && distance[3] > 1) {
      moveMotorA(225, true);
      moveMotorB(225, false);
      Serial.println("giro4");
      delay(10);
    }

    // Condicion 5: Distancia atrás menor a 60 después de descartar lo demás
    else if (distance[1] <= 30 && distance[1] > 1) {
      moveMotorA(250, true);
      moveMotorB(250, false);
      Serial.println("giro5");
      delay(50);
    }
  }

  /* Segundo Caso: Condicion Superficie Blanca
  else if (started && (IRstate[0] <= 6 || IRstate[1] <= 15 || IRstate[2] <= 4000 || IRstate[4] <= 4000) &&
        (distance[0] <= 30 || distance[1] <= 30 || distance[2] <= 30 || distance[3] <= 20)) {
    
    if (IRstate[0] <= 6 && (distance[1] <= 30 || distance[2] <= 30)) {

    }
  } */

  // Tercer Caso: Condicion Superficie Blanca Total
  // Superficie blanca detras
  else if (started && (IRstate[2] < 4000 || IRstate[3] < 4000)) {
    moveMotorA(250, true);
    moveMotorB(250, true);
    Serial.println("giro6");
    delay(15);
  }
  
  // Superficie blanca adelante
  else if (started && (IRstate[0] < 6 || IRstate[1] < 15) && (IRstate[2] > 4000 && IRstate[3] > 4000)) {
    moveMotorA(250, false);
    moveMotorB(250, false);
    Serial.println("giro7");
    delay(15);
  }
  
  delay(1); // Por si acaso
  }

  Serial.println("Llegue?");
}

void waitingTheButton () {
  while (started == false) {
    currentTime = millis();       

    // Condicional para comprobar la bandera working y más abajo la diferencia de tiempo de 5 o más segundos
    if (waiting && digitalRead(BTN) == LOW) {
      waiting = false;
      delay(25);                         // Antirrebote pro que el profe pablo nos enseñó
      Serial.println("Pushed");          // Ahí como para poder leer bien en el monitor serial
      Serial.println("Pushed");
      Serial.println("Pushed");
    } else if (waiting && digitalRead(BTN) == HIGH) {
      lastTime = currentTime;            // Guarda el tiempo anterior para que respeten los 5 seg en cualquier momento
    }

    if (!started && !waiting && currentTime - lastTime >= fiveSeconds) { // Importante los ! para las negaciones de los boleanos
      started = true;
    }
    delay(20);
  }
}

void initSensors() {
  // Declara todos los XSHUT de sensores láser como salidas poniendo y los apaga
  for (int i = 0; i < 4; i++) {
    pinMode(xShutPins[i], OUTPUT);
    digitalWrite(xShutPins[i], LOW);
  }
  // Declara todos los pines de señal de sensores QTR como salidas
  for (int i = 0; i < 4; i++) {
    pinMode(QTRPins[i], INPUT);
  }
  delay(10);

  // Exclusivo para VL53L0X: Encendido y reconfiguración dirección de uno en uno
  for (int i = 0; i < 4; i++) {
    digitalWrite(xShutPins[i], HIGH);     // Encender sensor actual
    delay(20);
    
    if (!lox[i].begin(sensorAddresses[i])) {    // Solo si llega a ocurrir algún error al inicializar un sensor
      Serial.print("Error al inicializar VL53L0X número: ");
      Serial.println(i);
    }
    delay(10);
  }
}

void readSensors () {
  // Lectura de Distancias (VL53L0X)
  for (int i = 0; i < 4; i++) {

    VL53L0X_RangingMeasurementData_t measure;
    lox[i].rangingTest(&measure, false);
    
    // Estado 4 significa que hay problemas y nos tenemos que dar de baja
    if (measure.RangeStatus != 4) {
      distance[i] = measure.RangeMilliMeter / 10; // Guardado en cm
    } else {
      distance[i] = 555; // Valor si llega a haber algun problema
    }
  }

  // Lectura de la Refractancia del Suelo (QTR-1A)
  for (int i = 0; i < 4; i++) {  
    IRstate[i] = analogRead(QTRPins[i]); // Lectura ADC (12 bits) de 0 a 4095
  }
}

void moveMotorA(int speed, bool forward) {
  if (forward) {
    digitalWrite(AIN1, LOW);
    digitalWrite(AIN2, HIGH);
  } else {
    digitalWrite(AIN1, HIGH);
    digitalWrite(AIN2, LOW);
  }
  analogWrite(PWMA, speed);
}

void moveMotorB(int speed, bool forward) {
  if (forward) {
    digitalWrite(BIN1, HIGH);
    digitalWrite(BIN2, LOW);
  } else {
    digitalWrite(BIN1, LOW);
    digitalWrite(BIN2, HIGH);
  }
  analogWrite(PWMB, speed);
}

void stopMotors() {
  digitalWrite(AIN1, LOW);
  digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW);
  digitalWrite(BIN2, LOW);
  analogWrite(PWMA, 0);
  analogWrite(PWMB, 0);
}