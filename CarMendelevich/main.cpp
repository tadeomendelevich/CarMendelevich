/*! \mainpage Ejercicio Titulo
 * \date 10/09/2023
 * \author Alejandro Rougier
 * \section Ejemplo comunicación USART
 * [Complete aqui con su descripcion]
 *
 * \section desarrollos Observaciones generales
 * [Complete aqui con sus observaciones]
 *
 * \section changelog Registro de cambios
 *
 * |   Fecha    | Descripcion                                    |
 * |:----------:|:-----------------------------------------------|
 * | 10/09/2023 | Creacion del documento                         |
 *
 */



/* Includes ------------------------------------------------------------------*/
#include "mbed.h"
#include "util.h"
#include "myDelay.h"
#include "debounce.h"
#include "config.h"
#include "wifi.h"
/* END Includes --------------------------------------------------------------*/


/* typedef -------------------------------------------------------------------*/

/* END typedef ---------------------------------------------------------------*/

/* define --------------------------------------------------------------------*/
#define RXBUFSIZE           256
#define TXBUFSIZE           256
#define DEBOUNCE            20
#define HEARBEATIME         100
#define GENERALTIME         10
#define NUMBUTTONS          4
#define DISTANCEINTERVAL    300

#define FORWARD             2
#define BACKWARD            1
#define STOP                0
#define MAXSPEED            25000
#define MEDIUMSPEED         12500
#define ALMOSTNOSPEED       6800
#define NOSPEED             0

#define SERIE               0
#define WIFI                1

#define RESETFLAGS          flags.bytes 
#define ISCOMAND            flags.bits.bit0
#define SERVOMOVING         flags.bits.bit1
#define SERVODIRECT         flags.bits.bit2
#define MEDIRDISTANCIA      flags.bits.bit3

#define IS100MS             flag1.bit.b0
#define IS20MS              flag1.bit.b1
#define DIRECCIONSERVO      flag1.bit.b5
#define ECHORISE            flag1.bit.b6
#define ECHOPULSE           flag1.bit.b7

#define IDLE                0
#define MODE_1              1
#define MODE_2              2
#define MODE_3              3
#define MODE_4              4
#define SENDINFOTIME        1500
#define CHECKLINETIME       10
#define LEFT                1
#define MID                 2
#define RIGHT               3
#define DISTANCETIME        10
#define TURNTIME            2000
#define TURNINGOBJECTTIME   500
#define FINDOBJECTTIME      150

#define BOXSPEED            6250
#define FIFTY               50
#define ONESECOND           1000
#define TWOSECONDS          2000
#define FIVESECONDS         5000
#define TENSECONDS          10000


/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/

DigitalOut LED(PC_13);//!< Led de Hearbet

BusOut leds(PB_6,PB_7,PB_14,PB_15);//!< leds de la placa

BusIn pulsadores(PA_4,PA_5,PA_6,PA_7);//!< Botones de la placa

RawSerial PC(PA_9,PA_10);//!< Configuración del puerto serie, la velocidad (115200) tiene que ser la misma en QT

DigitalOut TRIGGER(PB_13);//!< Salida para el trigger del sensor Ultrasonico 

InterruptIn ECHO(PB_12); //!<pin de eco del sensor Ultrasonico definido como interrupción 

PwmOut  servo(PA_8);//!< Pin del Servo debe ser PWM para poder modular el ancho del pulso

AnalogIn irLeft(PA_2); //!<Sensor infrarrojo para detección de linea 

AnalogIn irCenter(PA_1);//!<Sensor infrarrojo para detección de linea 

AnalogIn irRight(PA_0);//!<Sensor infrarrojo para detección de linea 

InterruptIn speedLeft(PB_9);//!<Sensor de Horquilla para medir velocidad

InterruptIn speedRight(PB_8);//!<Sensor de Horquilla para medir velocidad

BusOut dirMLeft(PB_14,PB_15);//!< Pines para determinara la dirección de giro del motor

BusOut dirMRight(PB_6,PB_7);//!< Pines para determinara la dirección de giro del motor

PwmOut speedMLeft(PB_1);//!< Pin de habilitación del giro del motor, se usa para controlar velocidad del mismo

PwmOut speedMRight(PB_0);//!< Pin de habilitación del giro del motor, se usa para controlar velocidad del mismo

/* END hardware configuration ------------------------------------------------*/


/* Function prototypes -------------------------------------------------------*/

/**
 * @brief Hearbeat, indica el funcionamiento del sistema
 * 
 * @param timeHearbeat Variable para el intervalo de tiempo
 * @param mask Secuencia de encendido/apagado del led de Hearbeat
 */
void hearbeatTask(_delay_t *timeHearbeat, uint8_t mask[]);

/**
 * @brief Ejecuta las tareas del puerto serie Decodificación/trasnmisión
 * 
 * @param dataRx Estructura de datos de la recepción
 * @param dataTx Estructura de datos de la trasnmisión
 * @param source Identifica la fuente desde donde se enviaron los datos
 */
void serialTask(_sRx *dataRx, _sTx *dataTx, uint8_t source);


/**
 * @brief Rutina oara medir distancia
 * 
 * @param medicionTime variable que contiene el intervalo de medición de distancia
 * @param triggerTime Variable que contiene el valor de tiempo del Trigger
 */
void distanceTask(_delay_t *medicionTime, int32_t *triggerTime);


/**
 * @brief Rutina para realizar la verificación de mocimiento del servo
 * en caso de que no se mueva envia la respuesta automática a la PC
 * 
 * @param servoTime almacena el tiempo del timer
 * @param intervalServo posee el tiempo del ointervalo para responder en función del ángulo a mover
 */
void servoTask(_delay_t *servoTime, uint32_t *intervalServo);

/**
 * @brief Rutina para medir la velocidad
 * 
 */
void speedTask();

/**
 * @brief Rutina para hacer la medición de los sensores IR
 * 
 */
void irSensorsTask();


/**
 * @brief Recepción de datos por el puerto serie
 * 
 */
void onRxData();

/**
 * @brief Pone el encabezado del protocolo, el ID y la cantidad de bytes a enviar
 * 
 * @param dataTx Estructura para la trasnmisión de datos
 * @param ID Identificación del comando que se envía
 * @param frameLength Longitud de la trama del comando
 * @return uint8_t devuelve el Checksum de los datos agregados al buffer de trasnmisión
 */
uint8_t putHeaderOnTx(_sTx  *dataTx, _eCmd ID, uint8_t frameLength);

/**
 * @brief Agrega un byte al buffer de transmisión
 * 
 * @param dataTx Estructura para la trasnmisión de datos
 * @param byte El elemento que se quiere agregar
 * @return uint8_t devuelve el Checksum del dato agregado al buffer de trasnmisión
 */
uint8_t putByteOnTx(_sTx    *dataTx, uint8_t byte);

/**
 * @brief Agrega un String al buffer de transmisión
 * 
 * @param dataTx Estructura para la trasnmisión de datos
 * @param str String a agregar
 * @return uint8_t devuelve el Checksum del dato agregado al buffer de trasnmisión
 */
uint8_t putStrOntx(_sTx *dataTx, const char *str);

uint8_t getByteFromRx(_sRx *dataRx, uint8_t iniPos, uint8_t finalPos);

/**
 * @brief Decodifica la trama recibida
 * 
 * @param dataRx Estructura para la recepción de datos
 */
void decodeHeader(_sRx *dataRx);

/**
 * @brief Decodifica el comando recibido en la transmisión y ejecuita las tareas asociadas a dicho comando
 * 
 * @param dataRx Estructura para la recepción de datos
 * @param dataTx Estructura para la trasnmisión de datos
 */
void decodeCommand(_sRx *dataRx, _sTx *dataTx);


/**
 * @brief Función del Sensor de Horquilla Izquierdo
 * cuenta los pulsos del sensor para medir velocidad luego
 */
void speedCountLeft(void);

/**
 * @brief Función del Sensor de Horquilla Derecho
 * cuenta los pulsos del sensor para medir velocidad luego
 */
void speedCountRight(void);

/**
 * @brief Función del Sensor de Horquilla Izquierdo
 * cuenta los pulsos del sensor para medir los giros luego
 */
void turnCountLeft(void);

/**
 * @brief Función del Sensor de Horquilla Derecho
 * cuenta los pulsos del sensor para medir los giros luego
 */
void turnCountRight(void);


/**
 * @brief Resteo el valor de mi variable de conteo para lograr 
 * girar de manera precisa
 */
void resetCountRight();

/**
 * @brief Resteo el valor de mi variable de conteo para lograr 
 * girar de manera precisa
 */
void resetCountLeft();

/**
 * @brief Función para realizar la autoconexión de los datos de Wifi
 * 
 */
void autoConnectWifi();

/**
 * @brief envía de manera automática el alive
 * 
 */
void aliveAutoTask(_delay_t *aliveAutoTime);

void FillPayload(uint8_t header);

/**
 * @brief Envia el contenido y datos para control de variables
 * 
 * @param buf 
 * @param length 
 */
void SendUDP(uint8_t *buf, uint8_t length);


/**
 * @brief Se encarga de mover el servo a determinada posicion
 * 
 * @param servoPrevio Posicion del servo anterior
 * @param angle Angulo en el que se va a llevar el servo
 */
void moveServo(uint32_t servoPrevio, int angle);

void OnTimeTriggerPulse();     // Apaga la señal de trigger despues de 10us

void ECHOInt();          // Interrupcion para detectar flancos de echo

void On10ms();

void Do100ms();

/**
 * @brief Se encarga de pasar los pulsos necesarios para mover los motores en determinada direccion y velocidad
 * 
 * @param direccion 
 * @param speed 
 */
void motores(_eDirections direccion, int speed);

/**
 * @brief Se encarga de girar el auto hasta cierta posisicion, controlado por horquillas
 * 
 * @param angle Anulgo de giro
 * @param lado Direccion de giro
 * @return uint8_t 
 */
uint8_t turn(int angle, _eDirections lado);

/**
 * @brief Se encarga de seguir la linea y controlar los motores para este objetivo
 * 
 * @param leftSensor Valor de sensor Ir izquierdo
 * @param midSensor Valor de sensor Ir central
 * @param rightSensor Valor de sensor Ir derecho
 */
void followLine(int32_t leftSensor, int32_t midSensor, int32_t rightSensor);

/**
 * @brief Se encarga de mantener el auto a cierta distancia de un objetivo
 * 
 * @param distanceWanted Distancia de mantencion deseada
 * @param distance Distancia medida
 * @param velProm Velocidad promedio de movimiento de los motores
 */
void PID(uint8_t distanceWanted, uint16_t distance, int16_t velProm);

/**
 * @brief Se encarga de revisar si todos los sensores Ir estan viendo negro
 * 
 * @param leftSensor Valor de sensor Ir izquierdo
 * @param midSensor Valor de sensor Ir central
 * @param rightSensor Valor de sensor Ir derecho
 * @return uint8_t 
 */
uint8_t allIrBlack(int32_t leftSensor, int32_t midSensor, int32_t rightSensor);

/**
 * @brief Revisa si todos los Ir se encuentran sobre blanco
 * 
 * @param leftSensor Valor de sensor Ir izquierdo
 * @param midSensor Valor de sensor Ir central
 * @param rightSensor Valor de sensor Ir derecho
 * @return uint8_t 
 */
uint8_t allIrWhite(int32_t leftSensor, int32_t midSensor, int32_t rightSensor);


/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/

Timer TimeHCSR04;

Timeout TimeTriggerPulse; 

Ticker Ticker10ms;

//DISTANCIAHCSR04 = 0xA3
uint32_t distancia_us;  // Measured distance  
uint8_t timeOutDistancia;
uint8_t time100ms;

const char firmware[] = "EX100923v01\n";

volatile _sRx dataRx;

uint32_t countLeftValue, countRightValue;

uint32_t turnLeftValue, turnRightValue;

uint32_t servoPrevio;

uint32_t speedleftValue, speedRightValue;

_sTx dataTx;

wifiData myWifiData;

volatile uint8_t buffRx[RXBUFSIZE];

uint8_t buffTx[TXBUFSIZE];

uint8_t globalIndex, index2;

_uFlag  flags;

_flag flag1;

_sButton myButton[NUMBUTTONS];

_delay_t    generalTime;

_delay_t    checkLineTime;

_delay_t    fiftyMsTime;

Timer myTimer;

_sSensor irSensor[3];

_sServo miServo;

_uWord myWord;

_sTx wifiTx;

_sRx wifiRx;

uint8_t wifiBuffRx[RXBUFSIZE];

uint8_t wifiBuffTx[TXBUFSIZE];

uint8_t buttonMode = 0;

int32_t     leftIrSensor;   // Measured value
int32_t     midIrSensor;
int32_t     rightIrSensor;

uint16_t    leftBlackColor;  // Black value
uint16_t    midBlackColor;
uint16_t    rightBlackColor;

uint16_t    leftWhiteColor;  // White value
uint16_t    midWhiteColor;
uint16_t    rightWhiteColor;

uint8_t     lastLine = 1;

int32_t     SLOWSPEED;
int32_t     *lineSpeed = &slowSpeedValue; // Asigna la dirección de speedValue a lineSpeed

_eDirections directions;

uint8_t     horquillasReseteadas = 0;

uint8_t     blackAgain = 0;

_sCounter myCounter;



/* END Global variables ------------------------------------------------------*/

/* Function prototypes user code ----------------------------------------------*/

/**
 * @brief Instanciación de la clase Wifi, le paso como parametros el buffer de recepción, el indice de 
 * escritura para el buffer de recepción y el tamaño del buffer de recepción
 */
Wifi myWifi(wifiBuffRx, &wifiRx.indexW, RXBUFSIZE);


void hearbeatTask(_delay_t *timeHearbeat, uint8_t mask[])
{   
    static uint8_t indexSequence;
    if (delayRead(timeHearbeat)) {     // REALIZO LA SECUENCIA DE INICIO 
        LED = !(mask[indexSequence]);
        indexSequence++;
        if (indexSequence > 30)
            indexSequence = 0;
    }
    // static uint8_t sec = 0;
    // if(delayRead(timeHearbeat)){
    //     LED = (~mask & (1<<sec));
    //     sec++;
    //     sec &= -(sec<16);
    // }
}

void serialTask(_sRx *dataRx, _sTx *dataTx, uint8_t source)
{
    if(dataRx->isComannd){
        dataRx->isComannd=false;
        decodeCommand(dataRx,dataTx);
    }

    if(delayRead(&generalTime)){
        if(dataRx->header){
            dataRx->timeOut--;
        if(!dataRx->timeOut)
            dataRx->header = HEADER_U;
        }
    }

    if(dataRx->indexR!=dataRx->indexW){
        decodeHeader(dataRx);
       /* CODIGO A EFECTOS DE EVALUAR SI FUNCIONA LA RECEPCIÓN , SE DEBE DESCOMENTAR 
       Y COMENTAR LA LINEA decodeHeader(dataRx); 
       while (dataRx->indexR!=dataRx->indexW){
            dataTx->buff[dataTx->indexW++]=dataRx->buff[dataRx->indexR++];
            dataTx->indexW &= dataTx->mask;
            dataRx->indexR &= dataRx->mask;
        } */
    }
        

    if(dataTx->indexR!=dataTx->indexW){
        if(source){
             myWifi.writeWifiData(&dataTx->buff[dataTx->indexR++],1); 
             dataTx->indexR &=dataTx->mask; 
        }else{
            if(PC.writeable()){
                PC.putc(dataTx->buff[dataTx->indexR++]);
                dataTx->indexR &=dataTx->mask;
            }
        }
    }

}


void onRxData()
{
    while(PC.readable()){
        dataRx.buff[dataRx.indexW++]=PC.getc();
        dataRx.indexW &= dataRx.mask;
    }
}

uint8_t putHeaderOnTx(_sTx  *dataTx, _eCmd ID, uint8_t frameLength)
{
    dataTx->chk = 0;
    dataTx->buff[dataTx->indexW++]='U';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]='N';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]='E';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]='R';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]=frameLength+1;
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]=':';
    dataTx->indexW &= dataTx->mask;
    dataTx->buff[dataTx->indexW++]=ID;
    dataTx->indexW &= dataTx->mask;
    dataTx->chk ^= (frameLength+1);
    dataTx->chk ^= ('U' ^'N' ^'E' ^'R' ^ID ^':') ;
    return  dataTx->chk;
}

uint8_t putByteOnTx(_sTx *dataTx, uint8_t byte)
{
    dataTx->buff[dataTx->indexW++]=byte;
    dataTx->indexW &= dataTx->mask;
    dataTx->chk ^= byte;
    return dataTx->chk;
}


uint8_t putStrOntx(_sTx *dataTx, const char *str)
{
    globalIndex=0;
    while(str[globalIndex]){
        dataTx->buff[dataTx->indexW++]=str[globalIndex];
        dataTx->indexW &= dataTx->mask;
        dataTx->chk ^= str[globalIndex++];
    }
    return dataTx->chk ;
}

uint8_t getByteFromRx(_sRx *dataRx, uint8_t iniPos, uint8_t finalPos){
    uint8_t getByte;
    dataRx->indexData += iniPos;
    dataRx->indexData &=dataRx->mask;
    getByte = dataRx->buff[dataRx->indexData];
    dataRx->indexData += finalPos;
    dataRx->indexData &=dataRx->mask;
    return getByte;
}

void decodeHeader(_sRx *dataRx)
{
    uint8_t auxIndex=dataRx->indexW;
    while(dataRx->indexR != auxIndex){
        switch(dataRx->header)
        {
            case HEADER_U:
                if(dataRx->buff[dataRx->indexR] == 'U'){
                    dataRx->header = HEADER_N;
                    dataRx->timeOut = 5;
                }
            break;
            case HEADER_N:
                if(dataRx->buff[dataRx->indexR] == 'N'){
                    dataRx->header = HEADER_E;
                }else{
                    if(dataRx->buff[dataRx->indexR] != 'U'){
                        dataRx->header = HEADER_U;
                        dataRx->indexR--;
                    }
                }
            break;
            case HEADER_E:
                if(dataRx->buff[dataRx->indexR] == 'E'){
                    dataRx->header = HEADER_R;
                }else{
                    dataRx->header = HEADER_U;
                    dataRx->indexR--;
                }
            break;
            case HEADER_R:
                if(dataRx->buff[dataRx->indexR] == 'R'){
                    dataRx->header = NBYTES;
                }else{
                    dataRx->header = HEADER_U;
                    dataRx->indexR--;
                }
            break;
            case NBYTES:
                dataRx->nBytes=dataRx->buff[dataRx->indexR];
                dataRx->header = TOKEN;
            break;
            case TOKEN:
                if(dataRx->buff[dataRx->indexR] == ':'){
                    dataRx->header = PAYLOAD;
                    dataRx->indexData = dataRx->indexR+1;
                    dataRx->indexData &= dataRx->mask;
                    dataRx->chk = 0;
                    dataRx->chk ^= ('U' ^'N' ^'E' ^'R' ^dataRx->nBytes ^':') ;
                }else{
                    dataRx->header = HEADER_U;
                    dataRx->indexR--;
                }
            break;
            case PAYLOAD:
                dataRx->nBytes--;
                if(dataRx->nBytes>0){
                   dataRx->chk ^= dataRx->buff[dataRx->indexR];
                }else{
                    dataRx->header = HEADER_U;
                    if(dataRx->buff[dataRx->indexR] == dataRx->chk)
                        dataRx->isComannd = true;
                }
            break;
            default:
                dataRx->header = HEADER_U;
            break;
        }
        dataRx->indexR++;
        dataRx->indexR &= dataRx->mask;
    }
}


void decodeCommand(_sRx *dataRx, _sTx *dataTx)
{
    int32_t motorSpeed, auxSpeed;
    int8_t angleSource;

    uint32_t servoPrevio = miServo.currentValue;
    switch(dataRx->buff[dataRx->indexData]){
        case ALIVE:
            putHeaderOnTx(dataTx, ALIVE, 2);
            putByteOnTx(dataTx, ACK );
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case FIRMWARE:
            putHeaderOnTx(dataTx, FIRMWARE, 12);
            putStrOntx(dataTx, firmware);
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case LEDSTATUS:
            putHeaderOnTx(dataTx, LEDSTATUS, 2);
            putByteOnTx(dataTx, ((~((uint8_t)leds.read()))&0x0F));
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case BUTTONSTATUS:
            putHeaderOnTx(dataTx, BUTTONSTATUS, 2);
            putByteOnTx(dataTx, ((~((uint8_t)pulsadores.read()))&0x0F));
            putByteOnTx(dataTx, dataTx->chk);
        break;
        case ANALOGSENSORS:
            myWord.ui16[0] =  irSensor[0].currentValue;
            putHeaderOnTx(dataTx, ANALOGSENSORS, 7);
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            myWord.ui16[0] =  irSensor[1].currentValue;
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            myWord.ui16[0] =  irSensor[2].currentValue;
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] ); 
            putByteOnTx(dataTx, dataTx->chk);       
        break;
        case SETBLACKCOLOR:
            putHeaderOnTx(dataTx, SETBLACKCOLOR, 2);
            putByteOnTx(dataTx, ACK );
            putByteOnTx(dataTx, dataTx->chk);

            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);

            irSensor[0].blackValue = myWord.i32;

            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);

            irSensor[1].blackValue = myWord.i32;

            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);

            irSensor[2].blackValue = myWord.i32;
        break;
        case SETWHITECOLOR:
            putHeaderOnTx(dataTx, SETBLACKCOLOR, 2);
            putByteOnTx(dataTx, ACK );
            putByteOnTx(dataTx, dataTx->chk);

            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);

            irSensor[0].whiteValue = myWord.i32;

            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);

            irSensor[1].whiteValue = myWord.i32;

            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);

            irSensor[2].whiteValue = myWord.i32;
        break;
        case SETLINESPEED:
            putHeaderOnTx(dataTx, SETLINESPEED, 2);
            putByteOnTx(dataTx, ACK );
            putByteOnTx(dataTx, dataTx->chk);

            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);

            motorSpeed = myWord.i32;
            *lineSpeed = (abs(motorSpeed))*250;
        break;
        case MOTORTEST:
            putHeaderOnTx(dataTx, MOTORTEST, 2);
            putByteOnTx(dataTx, ACK );
            putByteOnTx(dataTx, dataTx->chk);
            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);
            motorSpeed = myWord.i32;
            if(motorSpeed>=0)
                dirMLeft.write(FORWARD);
            else
                dirMLeft.write(BACKWARD);
            auxSpeed=(abs(motorSpeed))*250;
            speedMLeft.pulsewidth_us(auxSpeed);
            myWord.ui8[0]=getByteFromRx(dataRx,1,0);
            myWord.ui8[1]=getByteFromRx(dataRx,1,0);
            myWord.ui8[2]=getByteFromRx(dataRx,1,0);
            myWord.ui8[3]=getByteFromRx(dataRx,1,0);
            motorSpeed = myWord.i32;
            if(motorSpeed>=0)
                dirMRight.write(FORWARD);
            else
                dirMRight.write(BACKWARD);
            auxSpeed = (abs(motorSpeed))*250;
            speedMRight.pulsewidth_us (auxSpeed);
        break;
        case SERVOANGLE:
            putHeaderOnTx(dataTx, SERVOANGLE, 2);
            putByteOnTx(dataTx, ACK );
            putByteOnTx(dataTx, dataTx->chk);
            SERVOMOVING = true;
            angleSource = getByteFromRx(dataRx,1,0);

            if (angleSource < -90 || angleSource > 90)
                break;
            else{
                miServo.currentValue = (((angleSource - miServo.Y1) * (miServo.X2-miServo.X1))/(miServo.Y2-miServo.Y1)) + miServo.X1;
                if(miServo.currentValue > (uint16_t)miServo.X2)
                    miServo.currentValue = miServo.X2;
                if(miServo.currentValue < (uint16_t)miServo.X1)
                    miServo.currentValue = miServo.X1;
                servo.pulsewidth_us(miServo.currentValue);
            }
            if(miServo.currentValue > servoPrevio){
                miServo.intervalValue = (miServo.currentValue-servoPrevio);
            }else{
                miServo.intervalValue = (servoPrevio-miServo.currentValue);
            }
            miServo.intervalValue = (miServo.intervalValue*1000) / (miServo.X2-miServo.X1);
            if(miServo.intervalValue > 1000)
                miServo.intervalValue = 1000;
            if(miServo.intervalValue < 50)
                miServo.intervalValue = 50;
        break;
        case CONFIGSERVO:
        break;
        case GETDISTANCE:
            putHeaderOnTx(dataTx, GETDISTANCE, 5);
            myWord.ui32 = distancia_us;
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            putByteOnTx(dataTx, myWord.ui8[2] );
            putByteOnTx(dataTx, myWord.ui8[3] ); 
            putByteOnTx(dataTx, dataTx->chk);  
        break;
        case GETSPEED:
            myWord.ui32 = speedleftValue;
            putHeaderOnTx(dataTx, GETSPEED, 9);
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            putByteOnTx(dataTx, myWord.ui8[2] );
            putByteOnTx(dataTx, myWord.ui8[3] ); 
            myWord.ui32 = speedRightValue;
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            putByteOnTx(dataTx, myWord.ui8[2] );
            putByteOnTx(dataTx, myWord.ui8[3] ); 
            putByteOnTx(dataTx, dataTx->chk);           
        break;
        case SENDALLSENSORS:    // All the sensor's info in just a single send.
            putHeaderOnTx(dataTx, SENDALLSENSORS, 19);

            myWord.ui32 = distancia_us;    // Eye's distance
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            putByteOnTx(dataTx, myWord.ui8[2] );
            putByteOnTx(dataTx, myWord.ui8[3] ); 

            myWord.ui16[0] =  irSensor[0].currentValue; 
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            myWord.ui16[0] =  irSensor[1].currentValue;
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            myWord.ui16[0] =  irSensor[2].currentValue;
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] ); 

            myWord.ui32 = turnLeftValue;
            //myWord.ui32 = speedleftValue;
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            putByteOnTx(dataTx, myWord.ui8[2] );
            putByteOnTx(dataTx, myWord.ui8[3] ); 
            myWord.ui32 = turnRightValue;
            //myWord.ui32 = speedRightValue;
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            putByteOnTx(dataTx, myWord.ui8[2] );
            putByteOnTx(dataTx, myWord.ui8[3] ); 

            putByteOnTx(dataTx, dataTx->chk);      
        break;
        case RADAR:
            putHeaderOnTx(dataTx, RADAR, 6);    
            putByteOnTx(dataTx, ACK );

            SERVOMOVING = true;     // SERVO
            angleSource = getByteFromRx(dataRx,1,0);
            if (angleSource < -90 || angleSource > 90)
                break;
            else{
                miServo.currentValue = (((angleSource - miServo.Y1) * (miServo.X2-miServo.X1))/(miServo.Y2-miServo.Y1)) + miServo.X1;
                if(miServo.currentValue > (uint16_t)miServo.X2)
                    miServo.currentValue = miServo.X2;
                if(miServo.currentValue < (uint16_t)miServo.X1)
                    miServo.currentValue = miServo.X1;
                servo.pulsewidth_us(miServo.currentValue);
            }
            if(miServo.currentValue > servoPrevio){
                miServo.intervalValue = (miServo.currentValue - servoPrevio);
            }else{
                miServo.intervalValue = (servoPrevio - miServo.currentValue);
            }
            miServo.intervalValue = (miServo.intervalValue * 1000) /(miServo.X2-miServo.X1);
            if(miServo.intervalValue > 1000)
                miServo.intervalValue = 1000;
            if(miServo.intervalValue < 50)
                miServo.intervalValue = 50;
            
            myWord.ui32 = distancia_us;    // DISTANCE
            putByteOnTx(dataTx, myWord.ui8[0] );
            putByteOnTx(dataTx, myWord.ui8[1] );
            putByteOnTx(dataTx, myWord.ui8[2] );
            putByteOnTx(dataTx, myWord.ui8[3] ); 
            putByteOnTx(dataTx, dataTx->chk);   // CHECKSUM
        break;
        case SW0:
            putHeaderOnTx(dataTx, SW0, 2);
            putByteOnTx(dataTx, ACK );
            putByteOnTx(dataTx, dataTx->chk);
            buttonMode++;
        break;
        default:
            putHeaderOnTx(dataTx, (_eCmd)dataRx->buff[dataRx->indexData], 2);
            putByteOnTx(dataTx,UNKNOWN );
            putByteOnTx(dataTx, dataTx->chk);
        break;
        
    }
}


void servoTask(_delay_t *servoTime, uint32_t*intervalServo){
     /******************** RESPUESTA AUTOMATICA DEL SERVO ********************/
    if(SERVOMOVING){
        if(delayRead(servoTime)){
            SERVOMOVING=false;
            putHeaderOnTx(&dataTx,SERVOANGLE,2);
            putByteOnTx(&dataTx,SERVOFINISHMOVE);
            putByteOnTx(&dataTx, dataTx.chk);
        }
    }
}

void speedTask(){
    static int32_t timeSpeed = 0;
    #define INTERVAL 1000
    if ((myTimer.read_ms()-timeSpeed) >= INTERVAL){  // Cada un segundo guardo en una variable los valores de la velocidad
            timeSpeed = myTimer.read_ms();       
            speedleftValue = countLeftValue;
            countLeftValue = 0;
            speedRightValue = countRightValue;
            countRightValue = 0;
    } 
}


void irSensorsTask(){
    static int32_t timeSensors=0;
    static uint8_t index=0;
    #define INTERVALO 30
    if ((myTimer.read_ms() - timeSensors) >= INTERVALO){
            timeSensors = myTimer.read_ms(); 
            switch(index){
                case 0:
                    irSensor[index].currentValue = irLeft.read_u16();
                break;
                case 1:
                    irSensor[index].currentValue = irCenter.read_u16();
                break;
                case 2:
                    irSensor[index].currentValue = irRight.read_u16();
                break;
                default:
                    index = 255;
            }
            index++;
            index &=(-(index<=2));
    } 

}


void speedCountLeft(void){
    countLeftValue++;
}

void speedCountRight(void){
    countRightValue++;
}

void turnCountLeft(void){
    turnLeftValue++;
}

void turnCountRight(void){
    turnRightValue++;
}

void resetCountRight(){
    turnRightValue = 0;
}

void resetCountLeft(){
    turnLeftValue = 0;
}

void ECHOInt(){
    if(ECHORISE){ //si el flanco es ascendente
        TimeHCSR04.reset();
        TimeHCSR04.start();
        ECHORISE = 0;
    } else { //si el flanco es descendente
        distancia_us = TimeHCSR04.read_us();    //distancia del objeto en microsegundos 
        TimeHCSR04.stop();
        if(distancia_us > 2000)                 //midio distancia mayor a medida valida 
            distancia_us = 5800;
        ECHOPULSE = 0;                          //indica que vino el flanco descendente
    }
}

void OnTimeTriggerPulse(){
    TRIGGER = 0;
    ECHORISE = 1;
}

void On10ms(){
    time100ms--;
    if(!time100ms){
        IS100MS = 1;
        time100ms = 10;    
    }
}

void Do100ms(){

    IS100MS = 0;

    timeOutDistancia--;
    if(!timeOutDistancia){
        if(ECHOPULSE){ //si nunca vino el flanco descendente hay un error 
            ECHOPULSE = 0;
            TimeHCSR04.stop();
            distancia_us = 0xFFFFFFFF;//distancia no alcanzable
        }
        timeOutDistancia = 2;
        TRIGGER = 1;                                           //dispara trigger (señal)
        TimeTriggerPulse.attach_us(&OnTimeTriggerPulse, 10);   //espero 10us para frenar pulso de trigger
        ECHOPULSE = 1;
    }

}

/**********************************AUTO CONNECT WIF*********************/

void autoConnectWifi(){
    #ifdef AUTOCONNECTWIFI
        memcpy(&myWifiData.cwmode, dataCwmode, sizeof(myWifiData.cwmode));
        memcpy(&myWifiData.cwdhcp,dataCwdhcp, sizeof(myWifiData.cwdhcp) );
        memcpy(&myWifiData.cwjap,dataCwjap, sizeof(myWifiData.cwjap) );
        memcpy(&myWifiData.cipmux,dataCipmux, sizeof(myWifiData.cipmux) );
        memcpy(&myWifiData.cipstart,dataCipstart, sizeof(myWifiData.cipstart) );
        memcpy(&myWifiData.cipmode,dataCipmode, sizeof(myWifiData.cipmode) );
        memcpy(&myWifiData.cipsend,dataCipsend, sizeof(myWifiData.cipsend) );
        myWifi.configWifi(&myWifiData);
    #endif
}


void aliveAutoTask(_delay_t *aliveAutoTime)
{
    if(myWifi.isWifiReady()){
        if(delayRead(aliveAutoTime))
        {
            putHeaderOnTx(&wifiTx, ALIVE, 2);
            putByteOnTx(&wifiTx, ACK );
            putByteOnTx(&wifiTx, wifiTx.chk);
        }
    }
}

void moveServo(uint32_t servoPrevio, int angle)
{    
    miServo.currentValue = (((angle - miServo.Y1) * (miServo.X2-miServo.X1))/(miServo.Y2-miServo.Y1)) + miServo.X1;
    if(miServo.currentValue > (uint16_t)miServo.X2)
        miServo.currentValue = miServo.X2;
    if(miServo.currentValue < (uint16_t)miServo.X1)
        miServo.currentValue = miServo.X1;
    servo.pulsewidth_us(miServo.currentValue);  // Move the servo

    if(miServo.currentValue > servoPrevio){
        miServo.intervalValue = (miServo.currentValue-servoPrevio);
    }else{
        miServo.intervalValue = (servoPrevio-miServo.currentValue);
    }
    miServo.intervalValue = (miServo.intervalValue*1000) / (miServo.X2-miServo.X1);
    if(miServo.intervalValue > 1000)
        miServo.intervalValue = 1000;
    if(miServo.intervalValue < 50)
        miServo.intervalValue = 50;
}

void motores(_eDirections direccion, int speed) {
    switch (direccion) {
        case adelante:
            dirMLeft.write(FORWARD);
            dirMRight.write(FORWARD);
            speedMLeft.pulsewidth_us(speed);
            speedMRight.pulsewidth_us(speed + 1000);
            break;
        case atras:
            dirMLeft.write(BACKWARD);
            dirMRight.write(BACKWARD);
            speedMLeft.pulsewidth_us(speed);
            speedMRight.pulsewidth_us(speed);
            break;
        case derechasuave:
            dirMLeft.write(FORWARD);
            speedMLeft.pulsewidth_us(speed);
            speedMRight.pulsewidth_us(0);
            break;
        case derecha:
            dirMLeft.write(FORWARD);
            dirMRight.write(BACKWARD);
            speedMLeft.pulsewidth_us(speed);
            speedMRight.pulsewidth_us(speed);
            break;
        case izquierdasuave:
            dirMRight.write(FORWARD);
            speedMRight.pulsewidth_us(speed);
            speedMLeft.pulsewidth_us(0);
            break;
        case izquierda:
            dirMLeft.write(BACKWARD);
            dirMRight.write(FORWARD);
            speedMLeft.pulsewidth_us(speed);
            speedMRight.pulsewidth_us(speed);
            break;
        case stop:
            speedMLeft.pulsewidth_us(0);
            speedMRight.pulsewidth_us(0);
            break;
        default:
            speedMLeft.pulsewidth_us(0);
            speedMRight.pulsewidth_us(0);
            break;
    }
}

uint8_t turn(int angle, _eDirections lado) {
    if (horquillasReseteadas == 0) {
        horquillasReseteadas = 1;
        resetCountLeft();
        resetCountRight();
    } 

    switch(angle) {
        case 360:
            motores(lado, 8000);
            if (turnLeftValue >= 25 && turnRightValue >= 25) {
                return 1;
            } else {
                return 0;
            }
            break;
        
        case 90:
            if (lado == derecha || lado == izquierda) {
                motores(lado, 10000);
                if (turnLeftValue >= 4 && turnRightValue >= 4) {
                    return 1;
                } else {
                    return 0;
                }
            } else if (lado == izquierdasuave) {
                motores(lado, 12000);
                if (turnRightValue >= 13) {
                    motores(stop, 0);
                    return 1;
                } else {
                    return 0;
                }
            } else if (lado == derechasuave) {
                motores(lado, 7750);
                if (turnLeftValue >= 10) {
                    motores(stop, 0);
                    return 1;
                } else {
                    return 0;
                } 
            } else {
                return 0;
            }
            break;

        case 30:
            if (lado == derecha || lado == izquierda) {
                motores(lado, 6500);
                if (turnLeftValue >= 2 && turnRightValue >= 2) {
                    return 1;
                } else {
                    return 0;
                }
            } else if (lado == izquierdasuave) {
                motores(lado, 10000);
                if (turnRightValue >= 8) {
                    motores(stop, 0);
                    return 1;
                } else {
                    return 0;
                }
            } else if (lado == derechasuave) {
                motores(lado, 10000);
                if (turnLeftValue >= 8) {
                    motores(stop, 0);
                    return 1;
                } else {
                    return 0;
                } 
            } else {
                return 0;
            }
            break;
        
        case 1:
            if (lado == derecha || lado == izquierda) {
                motores(lado, 6500);
                if (turnLeftValue >= 2 && turnRightValue >= 2) {
                    return 1;
                } else {
                    return 0;
                }
            } else if (lado == izquierdasuave) {
                motores(lado, 10000);
                if (turnRightValue >= 2) {
                    motores(stop, 0);
                    return 1;
                } else {
                    return 0;
                }
            } else if (lado == derechasuave) {
                motores(lado, 10000);
                if (turnLeftValue >= 2) {
                    motores(stop, 0);
                    return 1;
                } else {
                    return 0;
                } 
            } else {
                return 0;
            }
            break;

        default:
            motores(stop, 0);
            return 0;
            break;
    }
}

void followLine(int32_t leftSensor, int32_t midSensor, int32_t rightSensor, int32_t speed) {
    if (delayRead(&checkLineTime)) {
        if (midSensor < midBlackColor && leftSensor > leftWhiteColor && rightSensor > rightWhiteColor) {  // In the line
            motores(adelante, speed);
        } else if (rightSensor < rightBlackColor && midSensor > midWhiteColor && leftSensor > leftWhiteColor) {   // Right sensor on the line
            motores(derechasuave, speed);
            lastLine = RIGHT;
        } else if (leftSensor < leftBlackColor && midSensor > midWhiteColor && rightSensor > rightWhiteColor) {   // Left sensor at line
            motores(izquierdasuave, speed);
            lastLine = LEFT;
        } else if (midSensor > midWhiteColor && leftSensor > leftWhiteColor && rightSensor > rightWhiteColor) {   // No sensor on the line
            switch (lastLine)
            {
            case LEFT:
                motores(izquierda, (speed - 1600));
                break;
            case RIGHT:
                motores(derecha, (speed - 1600));
                break;
            default:
                motores(izquierda, (speed - 1600));
                break;
            }
        }
    }
}

void PID(uint8_t distanceWanted, uint16_t distance, int16_t velProm) {
/*------------ VELOCIDAD PARA MANTER RECTO EL ROBOT -----------*/
    if (delayRead(&fiftyMsTime)) {
        if (distance > 30) {    // Controll the max measure of distance for not to go crazy
            distance = 30;
        }
        
        uint8_t kp = 15;
        
        int16_t errorMeasured = (distanceWanted - distance);
        
        int16_t correction = ((kp * errorMeasured));

        int16_t leftPulse;
        int16_t rightPulse; 

        if (miServo.currentValue > 0) {  // Looking at left
            leftPulse = (velProm + correction + 1350);
            rightPulse = (velProm - correction);
        } else {    // Looking at right
            leftPulse = (velProm - correction + 1350);
            rightPulse = (velProm + correction);
        }

        dirMLeft.write(FORWARD);    // Set engines to the calculated speed 
        dirMRight.write(FORWARD);
        speedMLeft.pulsewidth_us(leftPulse);
        speedMRight.pulsewidth_us(rightPulse); 
    }

    switch(myCounter.state) {
        case START:
            if (allIrWhite(leftIrSensor, midIrSensor, rightIrSensor)) { 
                myCounter.state = ADD;
            }
        
        case ADD:
            myCounter.counter++;
            myCounter.state = WAITBLACK;

            break;

        case WAITBLACK:
            if ((leftIrSensor < leftBlackColor || midIrSensor < midBlackColor || rightIrSensor < rightBlackColor) && !(allIrBlack(leftIrSensor, midIrSensor, rightIrSensor))) {
                myCounter.state = WAITWHITE;
            }   

            if(allIrBlack(leftIrSensor, midIrSensor, rightIrSensor)) {
                myCounter.state = END;
            }
            break;

        case WAITWHITE:
            if (allIrWhite(leftIrSensor, midIrSensor, rightIrSensor)) {
                myCounter.state = ADD;
            }

            if(allIrBlack(leftIrSensor, midIrSensor, rightIrSensor)) {
                myCounter.state = END;
            }
            break;

        case END:
            if (myCounter.firstNumberCounted == 0) {    // Reset values
                myCounter.counterStart[myCounter.i] = myCounter.counter;
                myCounter.counter = 0;
                //myCounter.i++;
            }
            break;

        default:
            myCounter.state = START;
            break;
    }
}

uint8_t allIrBlack(int32_t leftSensor, int32_t midSensor, int32_t rightSensor) {
    if ((midSensor < midBlackColor) && (leftSensor < leftBlackColor) && (rightSensor < rightBlackColor)) {    // All Ir are in black
        return 1;
    } else {
        return 0;
    }
}

uint8_t allIrWhite(int32_t leftSensor, int32_t midSensor, int32_t rightSensor) {
    if ((midIrSensor > midWhiteColor) && (rightIrSensor > rightWhiteColor) && (leftIrSensor > leftWhiteColor)) {    // All Ir are in white
        return 1;
    } else {
        return 0;
    }
}

/* END Function prototypes user code ------------------------------------------*/

int main()
{
    flag1.byte = 0;  // Put all the bits in 0
    time100ms = 10;

    Ticker10ms.attach_us(&On10ms, 10000);

    //ECHO del sensor HC-SR04
    ECHO.rise(&ECHOInt);
    ECHO.fall(&ECHOInt);
    
    //ULTRASONICO
    ECHORISE = 1;
    timeOutDistancia = 2;
    
    dataRx.buff = (uint8_t *)buffRx;
    dataRx.indexR = 0;
    dataRx.indexW = 0;
    dataRx.header = HEADER_U;
    dataRx.mask = RXBUFSIZE - 1;

    dataTx.buff = buffTx;
    dataTx.indexR = 0;
    dataTx.indexW = 0;
    dataTx.mask = TXBUFSIZE -1;

    wifiRx.buff = wifiBuffRx;
    wifiRx.indexR = 0;
    wifiRx.indexW = 0;
    wifiRx.header = HEADER_U;
    wifiRx.mask = RXBUFSIZE - 1;

    wifiTx.buff = wifiBuffTx;
    wifiTx.indexR = 0;
    wifiTx.indexW = 0;
    wifiTx.mask = TXBUFSIZE -1;

/* Local variables -----------------------------------------------------------*/
    uint8_t mask_0[30] = {1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0,1,0};
    uint8_t mask_1[30] = {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0};
    uint8_t mask_2[30] = {1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0,1,0,1,0,0,0,0,0,0,0};
    uint8_t mask_4[30] = {1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,0,0};

    _delay_t    hearbeatTime;
    _delay_t    debounceTime;
    _delay_t    medicionTime;
    _delay_t    servoTime;
    _delay_t    aliveAutoTime;
    _delay_t    sendTime;
    _delay_t    maintainDistanceTime;
    _delay_t    findobjectTime;
    _delay_t    oneSecondTime;
    _delay_t    twoSecondsTime;
    _delay_t    fiveSecondsTime;
    _delay_t    tenSecondsTime;
    _delay_t    advanceTime;

    uint8_t     lineState = 0;
    uint8_t     obstacleState = 0;
    int         turnTime = 0;
    uint8_t     side;

    uint8_t     petState = 0; 
    int         distance;
    int         speed;
    int         minSpeed = 4000;    
    int         angle = 0;
    uint8_t     angleState = 0;
    int         angleFound; 
    int         distanceFound = 300;
    uint8_t     fakeDistance = 0;

    uint8_t     pathState = 0;
    uint8_t     lookingRight = 0;
    uint8_t     distanceToRead = 8;
    uint8_t     servoMoved = 0;
    uint8_t     pathFound = 0;
    uint8_t     findingNewPath = 0;

    
    /* SERVO DEFUALT VALUES*/
    miServo.X2 = 2400;  // This value carries the servo to 90 degrees
    miServo.X1 = 550; // This value carries to -90 degrees
    miServo.Y2 = 90;
    miServo.Y1 = -90;
    miServo.intervalValue = 1500; // Angle cero value
    /* FIN SERVO DEFAULT VALUES*/

/* END Local variables -------------------------------------------------------*/


/* User code -----------------------------------------------------------------*/
    PC.baud(115200);
    myTimer.start();
    speedMLeft.period_ms(25);
    speedMRight.period_ms(25);
    servo.period_ms(20);
   /********** attach de interrupciones ********/
    PC.attach(&onRxData, SerialBase::IrqType::RxIrq);

    speedLeft.rise(&speedCountLeft);    // Cada vez que encuentro un flanco ascendente de [arte de la horquilla, sumo. 

    speedRight.rise(&speedCountRight);
    
    // Conteo de flancos ascendentes para giros controlados
    speedLeft.rise(&turnCountLeft);
    speedRight.rise(&turnCountRight);

     /********** FIN - attach de interrupciones ********/

    miServo.currentValue = 1075; 
    servo.pulsewidth_us(miServo.currentValue);

    speedMLeft.pulsewidth_us(0);
    
    speedMRight.pulsewidth_us(0);

    delayConfig(&hearbeatTime, HEARBEATIME);
    delayConfig(&debounceTime, DEBOUNCE);
    delayConfig(&generalTime,GENERALTIME);
    delayConfig(&medicionTime,DISTANCEINTERVAL);
    delayConfig(&servoTime,miServo.intervalValue);
    delayConfig(&aliveAutoTime, ALIVEAUTOINTERVAL);
    delayConfig(&sendTime, SENDINFOTIME);
    delayConfig(&checkLineTime, CHECKLINETIME);
    delayConfig(&maintainDistanceTime, DISTANCETIME);
    delayConfig(&findobjectTime,FINDOBJECTTIME);
    delayConfig(&fiftyMsTime,FIFTY);
    delayConfig(&oneSecondTime,ONESECOND);
    delayConfig(&twoSecondsTime,TWOSECONDS);
    delayConfig(&fiveSecondsTime,FIVESECONDS);
    delayConfig(&tenSecondsTime,TENSECONDS);
    delayConfig(&advanceTime,200);

    startButon(myButton, NUMBUTTONS);
    
    myWifi.initTask();
    autoConnectWifi();

    side = LEFT;

    irSensor[0].blackValue = 10000;  // Left IR sensor 
    irSensor[1].blackValue = 10000;  // Mid IR sensor
    irSensor[2].blackValue = 16500;  // Right IR sensor 

    irSensor[0].whiteValue = 15000;  // Left IR sensor 
    irSensor[1].whiteValue = 15000;  // Mid IR sensor
    irSensor[2].whiteValue = 20000;  // Right IR sensor 

    leftIrSensor = irSensor[0].currentValue;    // Measured value
    midIrSensor = irSensor[1].currentValue;
    rightIrSensor = irSensor[2].currentValue;

    leftBlackColor = irSensor[0].blackValue;    // Black color value
    midBlackColor = irSensor[1].blackValue;
    rightBlackColor = irSensor[2].blackValue;

    leftWhiteColor = irSensor[0].whiteValue;    // White color value
    midWhiteColor = irSensor[1].whiteValue;
    rightWhiteColor = irSensor[2].whiteValue;
    
    moveServo(servoPrevio, 0);
    
    while(1){
        myWifi.taskWifi();
        serialTask((_sRx *)&dataRx,&dataTx, SERIE);
        serialTask(&wifiRx,&wifiTx, WIFI);
        buttonTask(&debounceTime, myButton, pulsadores.read());
        speedTask();
        irSensorsTask();
        servoTask(&servoTime,&miServo.intervalValue);
        aliveAutoTask(&aliveAutoTime);
        SLOWSPEED = *lineSpeed;     // Motor's value to follow blackline
        servoPrevio = miServo.currentValue;

        if(IS100MS) // Send flag to measure distance
            Do100ms();
        
        distance = distancia_us / 58;  // Distance in "cm"  
        
        leftIrSensor = irSensor[0].currentValue;    // Measured value
        midIrSensor = irSensor[1].currentValue;
        rightIrSensor = irSensor[2].currentValue;

        leftBlackColor = irSensor[0].blackValue;    // Black color value
        midBlackColor = irSensor[1].blackValue;
        rightBlackColor = irSensor[2].blackValue;

        leftWhiteColor = irSensor[0].whiteValue;    // White color value
        midWhiteColor = irSensor[1].whiteValue;
        rightWhiteColor = irSensor[2].whiteValue;


        switch (buttonMode)
        {
        case IDLE:
            hearbeatTask(&hearbeatTime, mask_0);
            motores(stop, 0);
            break;
        case MODE_1:
            buttonMode = MODE_4;
            hearbeatTask(&hearbeatTime, mask_1);
            switch (lineState)
            {
            case FOLLOWING:
                followLine(leftIrSensor, midIrSensor, rightIrSensor, SLOWSPEED);

                // if (distance < 25 && distance != 0) {    // If there´s an object
                //     lineState = OBSTACLE;
                //     obstacleState = TURNCAR;
                //     speedMLeft.pulsewidth_us(NOSPEED);
                //     speedMRight.pulsewidth_us(NOSPEED);
                //     wait_ms(500);
                //     resetCountLeft();
                //     resetCountRight();
                //     turnTime = myTimer.read_ms();
                // }
                break;
            case OBSTACLE:
                switch (obstacleState)
                {
                case TURNCAR:
                    if (turnLeftValue >= 12 && turnRightValue >= 12) {  // Once I have turned I get out
                        obstacleState = DODGE;
                        turnTime = myTimer.read_ms();
                    } else {
                        if (side == LEFT) {
                            dirMLeft.write(BACKWARD);
                            dirMRight.write(FORWARD);
                            speedMLeft.pulsewidth_us(ALMOSTNOSPEED);
                            speedMRight.pulsewidth_us(ALMOSTNOSPEED);
                        } else {
                            dirMLeft.write(FORWARD);
                            dirMRight.write(BACKWARD);
                            speedMLeft.pulsewidth_us(ALMOSTNOSPEED);
                            speedMRight.pulsewidth_us(ALMOSTNOSPEED);
                        }
                    }
                    break;

                case DODGE: 
                    if (side == LEFT) {    // DODGE BY LEFT (LOOKING AT RIGHT)
                        moveServo(servoPrevio, -65);    // Servo looking at right
                        if (distance > 20) {   // If I'm near the object
                            dirMLeft.write(FORWARD);
                            dirMRight.write(FORWARD);
                            speedMLeft.pulsewidth_us(7000);
                            speedMRight.pulsewidth_us(4500);
                        } else if (distance <= 20) {    // If I'm so near 
                            dirMLeft.write(FORWARD);
                            dirMRight.write(FORWARD);
                            speedMLeft.pulsewidth_us(4950);
                            speedMRight.pulsewidth_us(6500);
                        }
                    } else {   // DODGE BY RIGHT (LOOKING AT LEFT)
                        moveServo(servoPrevio, 65);    // Servo looking at left
                        if (distance > 20) {   // If I'm near the object
                            dirMLeft.write(FORWARD);
                            dirMRight.write(FORWARD);
                            speedMLeft.pulsewidth_us(4500);
                            speedMRight.pulsewidth_us(7000);
                        } else if (distance <= 20) {    // If I'm so near 
                            dirMLeft.write(FORWARD);
                            dirMRight.write(FORWARD);
                            speedMLeft.pulsewidth_us(6500);
                            speedMRight.pulsewidth_us(4950);
                        }
                    }   

                    if (midIrSensor > midBlackColor && leftIrSensor < leftWhiteColor && rightIrSensor < rightWhiteColor) { // I find the line again
                        lineState = FOLLOWING; 
                        if (side == LEFT) { // Change the side where we're going to dodge next obstacle
                            dirMLeft.write(BACKWARD);
                            dirMRight.write(FORWARD);
                            speedMLeft.pulsewidth_us(SLOWSPEED);
                            speedMRight.pulsewidth_us(SLOWSPEED);
                            side = RIGHT;
                        } else {
                            dirMLeft.write(FORWARD);
                            dirMRight.write(BACKWARD);
                            speedMLeft.pulsewidth_us(SLOWSPEED);
                            speedMRight.pulsewidth_us(SLOWSPEED);
                            side = LEFT;
                        }
                        moveServo(servoPrevio, 0);
                    }

                    break;
                default:
                    obstacleState = TURNCAR;
                    break;
                }
                break;
            default:
                lineState = FOLLOWING;
                break;
            }
            break;
            
        case MODE_2:
            buttonMode = MODE_4;
            break;

        case MODE_3:    // Stay at a certain distance from an object
            hearbeatTask(&hearbeatTime, mask_2);
            buttonMode = MODE_4;
            
            if (delayRead(&maintainDistanceTime)) {
                switch (petState)
                {
                case SCANNING:
                    speedMLeft.pulsewidth_us(NOSPEED);  // Stay quiet
                    speedMRight.pulsewidth_us(NOSPEED);

                    switch (angleState) 
                    {
                        case ANGLEADVANCING:
                            angle++;
                            moveServo(servoPrevio, angle);

                            if (angle >= 90) {  // Once reached the limit, change the direction
                                angleState = ANGLEBACKING;
                            }
                            break;
                        case ANGLEBACKING:
                            angle--;
                            moveServo(servoPrevio, angle);

                            if (angle <= -90) {  // Once reached the limit, change the direction
                                angleState = SCANNED;
                            }
                            break;
                        case SCANNED:
                            resetCountLeft();   // Reset to control the turn to find the object 
                            resetCountRight();
                            if(distanceFound > 100) { // If no object was found, we turn 180 degrees
                                petState = TURNING180;
                                angleState = ANGLEADVANCING;
                            } else {
                                turnTime = myTimer.read_ms();
                                petState = TURNINGTOOBJECT;
                                angleState = ANGLEADVANCING;
                            }
                            angle = 0;
                            moveServo(servoPrevio, angle);
                            break;
                        default:
                            angleState = ANGLEADVANCING;
                            break;
                    }

                    if ((distance < distanceFound) && (distance <= 100) && (distance != 0)) {  // I find an object at 1 meter range, (distance is being measured everytime)
                        distanceFound = distance;
                        angleFound = angle;
                    }
                    break;

                case TURNINGTOOBJECT:

                    if ((myTimer.read_ms() - turnTime) >= TURNINGOBJECTTIME){
                        if (angleFound >= 0) {  // If object is at left                           
                            if (turnLeftValue >= 2 && turnRightValue >= 2) {     // When I turn, I wait a little for check distance
                                speedMLeft.pulsewidth_us(NOSPEED);
                                speedMRight.pulsewidth_us(NOSPEED);
                                resetCountLeft();
                                resetCountRight();
                                turnTime = myTimer.read_ms();
                            } else {
                                dirMLeft.write(BACKWARD);    // Turn left
                                dirMRight.write(FORWARD);
                                speedMLeft.pulsewidth_us(ALMOSTNOSPEED);
                                speedMRight.pulsewidth_us(ALMOSTNOSPEED);
                            }
                        } else {    // If object is at right
                            if (turnLeftValue >= 2 && turnRightValue >= 2) {     // When I turn, I wait a little for check distance
                                speedMLeft.pulsewidth_us(NOSPEED);
                                speedMRight.pulsewidth_us(NOSPEED);
                                resetCountLeft();
                                resetCountRight();
                                turnTime = myTimer.read_ms();
                            } else {
                                dirMLeft.write(FORWARD);    // Turn right
                                dirMRight.write(BACKWARD);
                                speedMLeft.pulsewidth_us(ALMOSTNOSPEED);
                                speedMRight.pulsewidth_us(ALMOSTNOSPEED);
                            }
                        }
                    }  

                    if ((distance >= (distanceFound - 30)) && (distance <= (distanceFound + 30))) { // I am in the correct rank of distance I go out
                        angleFound = 0;
                        distanceFound = 300;    // Reset distanceFound
                        petState = MOVING;
                    }
                    break;
                
                case MOVING:
                    if (distance > 100) {   // If it has lost the object in sight I add one to fakeDistance
                        fakeDistance++;
                    } else if (distance > 15 && distance <= 100) {    // It's far but still on the range (100cm)
                        speed = minSpeed + ((distance / 200.0) * (MAXSPEED - minSpeed));    // Calculate the speed according to distance
                        dirMLeft.write(FORWARD);
                        dirMRight.write(FORWARD);
                        speedMLeft.pulsewidth_us(speed);    // Turn on motors at moderate speed
                        speedMRight.pulsewidth_us(speed);
                    } else if (distance < 9) {  // Too near
                        dirMLeft.write(BACKWARD);
                        dirMRight.write(BACKWARD);
                        speedMLeft.pulsewidth_us(ALMOSTNOSPEED);    // Turn on motors backwards
                        speedMRight.pulsewidth_us(ALMOSTNOSPEED);
                    } else {    // Right in pointttt
                        speedMLeft.pulsewidth_us(NOSPEED);    // Turn off motors
                        speedMRight.pulsewidth_us(NOSPEED);
                    }

                    if (fakeDistance >= 10) {   // If It has lost the object in sight
                        petState = TURNING180;
                        resetCountLeft();   // Reset both values to control de turn 
                        resetCountRight();
                    }
                    break;

                case TURNING180:
                    if (turnLeftValue >= 18 && turnRightValue >= 18) {  // Once sensors have measured the right values
                        distanceFound = 300;
                        angleState = ANGLEADVANCING;
                        angle = 0;
                        fakeDistance = 0;
                        petState = SCANNING;
                    } else {
                        dirMLeft.write(FORWARD);
                        dirMRight.write(BACKWARD);
                        speedMLeft.pulsewidth_us(ALMOSTNOSPEED);    
                        speedMRight.pulsewidth_us(ALMOSTNOSPEED);
                    }
                    break;
                default:
                    petState = SCANNING;
                    break;
                }
            }
            break;

        case MODE_4:    // Find the shorter path
            hearbeatTask(&hearbeatTime, mask_4);
            switch (pathState)
            {
            case AXISTURN:
                if (turn(360, derecha)) {  // Once I have turned in my axis
                    pathState = FINDCIRCLE;
                }
                break;
            
            case FINDCIRCLE:    // Advance to stop in line of circle
                motores(adelante, SLOWSPEED);
                if (midIrSensor < midBlackColor || leftIrSensor < leftBlackColor || rightIrSensor < rightBlackColor) {
                    pathState = ANTICLOCKWISE;
                    horquillasReseteadas = 0;
                }
                break;
            
            case ANTICLOCKWISE:     // Turn to take the circle anticlockwise
                if (turn(90, derechasuave)) {
                    if(delayRead(&twoSecondsTime)) { // Wait a little to turn completely
                        moveServo(servoPrevio, -85);    // Servo looking at right
                        pathState = FINDPATH;
                    } 
                    followLine(leftIrSensor, midIrSensor, rightIrSensor, 7500);
                }
                break;
            
            case FINDPATH:
                followLine(leftIrSensor, midIrSensor, rightIrSensor, BOXSPEED);
                
                if (distance >= 8 && distance <= 17) {    // I search the best distance from the walls 
                    motores(stop, 0);
                    moveServo(servoPrevio, 10);  // Move Servo to check if there's an open door
                    pathState = CHECKOPENDOOR;
                }

                if (delayRead(&fiveSecondsTime)) {   // If  haven't found a path in a ten seconds row
                    horquillasReseteadas = 0;
                    pathState = CHANGESIDE;
                }
                break;

            case CHANGESIDE: 
                if (turn(1, izquierda)) {   // After turning 180 degrees go ahead to find lane
                    motores(adelante, 5000);
                    if (!allIrWhite(leftIrSensor, midIrSensor, rightIrSensor)) {
                        lookingRight = 0;
                        pathState = FINDPATH;
                    }
                }
                break;
            
            case CHECKOPENDOOR:
                motores(stop, 0);

                if (delayRead(&oneSecondTime)) {
                    if (distance > 60) {   // If there's no wall
                        moveServo(servoPrevio, 0);
                        horquillasReseteadas = 0;
                        pathState = MOVETOPATH;
                    } else {
                        moveServo(servoPrevio, -85);
                        pathState = MOVEALITTLE;
                    }
                }
                break;

            case MOVEALITTLE:
                followLine(leftIrSensor, midIrSensor, rightIrSensor, BOXSPEED);
                
                if (delayRead(&twoSecondsTime)) {
                    pathState = FINDPATH;
                } 
                break;            

            case MOVETOPATH:                
                if (turn(90, izquierda)) {    // When I have turned or I have found the line
                    resetCountLeft();
                    resetCountRight();
                    pathState = ADVANCEALITTLE;
                }
                break;

            case ADVANCEALITTLE:
                if (turnLeftValue > 3 && turnRightValue > 3 && allIrWhite(leftIrSensor, midIrSensor, rightIrSensor)) {
                    lastLine = LEFT;    // Take this value to make robot go left
                    pathState = GOTONUMBERPATH;
                } else {
                    motores(adelante, 6500);
                }
                break;

            case GOTONUMBERPATH:
                followLine(leftIrSensor, midIrSensor, rightIrSensor, SLOWSPEED);

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor)) {   // If I should take the path number
                    if (servoMoved == 0) {
                        motores(stop, 0);
                        moveServo(servoPrevio, -85);
                        pathFound = 0;
                        servoMoved = 1;
                    }
                    if (delayRead(&twoSecondsTime)) {   // Wait two seconds to take the distance well
                        pathState = TAKEFIRSTNUMBER;
                        servoMoved = 0;
                    }
                }
                break;

            case TAKEFIRSTNUMBER:   // First three lines in the circle
                PID(distanceToRead, distance, 5000);

                if ((midIrSensor > midWhiteColor) && (rightIrSensor > rightWhiteColor) && (leftIrSensor > leftWhiteColor)) {
                    pathFound = 1;
                }

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor) && pathFound == 1) {  // If all the Ir are on black
                    pathFound = 0;
                    motores(stop, 0);
                    pathState = GOINGOUTFIRSTPATH;
                }
                break;

            case GOINGOUTFIRSTPATH: // See the last first line
                if (!allIrBlack(leftIrSensor, midIrSensor, rightIrSensor)) {  // If I don't see all Ir's in black anymore
                    pathState = FOLLOWINGPATH;
                } else {
                    motores(adelante, 6000);
                }
                break;

            case FOLLOWINGPATH: // After three first lines 
                followLine(leftIrSensor, midIrSensor, rightIrSensor, SLOWSPEED);

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor) && distance > 2 && distance < 20) {  // I am in a number path again
                    pathFound = 0;
                    motores(stop, 0);
                    if (delayRead(&oneSecondTime)) {   // Wait two seconds to take the distance well
                        pathState = TAKELASTNUMBER;
                    }
                }
                break;

            case TAKELASTNUMBER:    // Reading first line of the last path's number
                PID(distanceToRead, distance, 5000);

                if (allIrWhite(leftIrSensor, midIrSensor, rightIrSensor)) { 
                    pathFound = 1;
                }

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor) && pathFound == 1) {  // If all the Ir are on black
                    if (delayRead(&oneSecondTime)) {
                        pathFound = 0;
                        pathState = GOOUTFROMSECONDPATH;
                    } else {
                        if (lookingRight == 0) {
                            moveServo(servoPrevio, 0);
                            lookingRight = 1;
                        }
                        motores(stop, 0);
                    }
                }
                break;

            case GOOUTFROMSECONDPATH:
                if (!allIrBlack(leftIrSensor, midIrSensor, rightIrSensor) && distance > 20) {  // If I don't see all Ir's in black anymore
                    pathState = FINDINGOTHERPATH;
                } else {
                    motores(adelante, 6000);
                }
                break;

            case FINDINGOTHERPATH:  // After the last three last lines of the path
                followLine(leftIrSensor, midIrSensor, rightIrSensor, SLOWSPEED);
                myCounter.firstNumberCounted = 1;

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor) && (distance <= 25)) {  // If all the Ir are on black
                    if (delayRead(&oneSecondTime)) {
                        horquillasReseteadas = 0;
                        pathState = TURNTOLARGELANE;               
                    }
                    motores(stop, 0);
                }                
                break;
            
            case TURNTOLARGELANE:
                if ((!allIrBlack(leftIrSensor, midIrSensor, rightIrSensor)) && (distance <= 25)) {  // After reaching nearer the wall to turn left
                    turn(90, derecha);
                    moveServo(servoPrevio, -85);
                    lastLine = LEFT;
                    pathState = NEXTPATH;
                } else {
                    motores(adelante, 6000);
                }
                break;

            case NEXTPATH:
                if (delayRead(&twoSecondsTime)) {  // Boleano for not making a mistake in new path sreaching
                    findingNewPath = 1;
                }

                if (distance > 2 && distance < 15 && findingNewPath == 1) {    // I have found another path
                    motores(stop, 0);
                    if(delayRead(&twoSecondsTime)) { // Stop for the whole seconds
                        findingNewPath = 0;
                        moveServo(servoPrevio, 90);
                        resetCountLeft();   // Reset horquillas
                        resetCountRight();
                        pathState = NOTCRASH;
                    }
                } else {
                    followLine(leftIrSensor, midIrSensor, rightIrSensor, SLOWSPEED);
                }
                break;
            
            case NOTCRASH:
                if (turnRightValue >= 13 && turnLeftValue >= 13) {
                    motores(stop, 0);
                    if (delayRead(&twoSecondsTime)) {
                        horquillasReseteadas = 0;
                        pathState = TURNINGTOPATH;
                    }
                } else {
                    motores(adelante, 5000);
                }
                break;

            case TURNINGTOPATH:
                if (turn(30, derecha)) {
                    horquillasReseteadas = 0;
                    pathState = ANOTHERPATH;
                }
                break;

            case ANOTHERPATH:
                if (leftIrSensor < leftBlackColor || midIrSensor < midBlackColor || rightIrSensor < rightBlackColor) {
                    motores(stop, 0);
                    if(delayRead(&oneSecondTime)) {
                        pathState = TURNTOANOTHERPATH;
                    }
                } else {
                    motores(izquierda, 5500);
                }
                break;

            case TURNTOANOTHERPATH:             
                followLine(leftIrSensor, midIrSensor, rightIrSensor, SLOWSPEED);

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor) && distance > 2 && distance < 20) {  // I am in a number path again
                    pathFound = 0;
                    motores(stop, 0);
                    if (delayRead(&oneSecondTime)) {   // Wait two seconds to take the distance well
                        pathState = TAKEANOTHERNUMBER;
                    }
                }
                break;

            case TAKEANOTHERNUMBER:     // First number of another path
                PID(distanceToRead, distance, 5000);
                
                if (allIrWhite(leftIrSensor, midIrSensor, rightIrSensor)) {
                    pathFound = 1;
                }

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor) && pathFound == 1) {  // If all the Ir are on black
                    pathFound = 0;
                    motores(stop, 0);
                    pathState = ADVANCE;
                }
                break;

            case ADVANCE:
                motores(adelante, 6000);
                
                if (!allIrBlack(leftIrSensor, midIrSensor, rightIrSensor)) {
                    moveServo(servoPrevio, 0);
                    pathState = RETURNTOCIRCLE;
                }
                break;

            case RETURNTOCIRCLE:    // Keep following
                followLine(leftIrSensor, midIrSensor, rightIrSensor, SLOWSPEED);

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor)) {   // If I should take the path number
                    if (servoMoved == 0) {
                        motores(stop, 0);
                        moveServo(servoPrevio, 90);
                        pathFound = 0;
                        servoMoved = 1;
                    }
                    if (delayRead(&twoSecondsTime)) {   // Wait two seconds to take the distance well
                        pathState = TAKEFINALNUMBER;
                    }
                }   
                break;

            case TAKEFINALNUMBER:   // Final number
                PID(distanceToRead, distance, 5000);
                
                if (allIrWhite(leftIrSensor, midIrSensor, rightIrSensor)) { 
                    pathFound = 1;
                }

                if (allIrBlack(leftIrSensor, midIrSensor, rightIrSensor) && pathFound == 1) {  // If all the Ir are on black
                    motores(stop, 0);
                    if (delayRead(&oneSecondTime)) {
                        pathFound = 0;
                        pathState = GETOUTFINALNUMBER;
                    }
                } 
                break;
            
            case GETOUTFINALNUMBER:
                motores(adelante, 6000);

                if (!allIrBlack(leftIrSensor, midIrSensor, rightIrSensor)) {
                    moveServo(servoPrevio, 0);
                    pathState = GETINGINCIRLCE;
                } 
                break;

            case GETINGINCIRLCE:
                followLine(leftIrSensor, midIrSensor, rightIrSensor, SLOWSPEED);
                
                if (allIrWhite(leftIrSensor, midIrSensor, rightIrSensor)) { // I have reached the end
                    horquillasReseteadas = 0;
                    pathState = INTOCIRCLE;
                } 
                break;

            case INTOCIRCLE:
                motores(derechasuave, 8000);
                myCounter.firstNumberCounted = 0;
                
                if (leftIrSensor < leftBlackColor || midIrSensor < midBlackColor || rightIrSensor < rightBlackColor) {
                    pathFound = 0;
                    horquillasReseteadas = 0;
                    lookingRight = 0;
                    servoMoved = 0;
                    moveServo(servoPrevio, -85);
                    pathState = FINDPATH;
                }
                break;

            default:
                pathState = AXISTURN;
                break;
            }

            break;

        default:
            buttonMode = IDLE;
            break;
        }
    
        if(myButton[0].flagDetected == RISINGFLAG) {    // If button is pressed
            buttonMode++;
            moveServo(servoPrevio, 0);

            if (buttonMode == MODE_1) {
                buttonMode++;
                lineState = FOLLOWING;
            }

            if (buttonMode == MODE_2) { // Reset parameters for Mode_2
                moveServo(servoPrevio, -85);
                buttonMode++;
                angleFound = 0;
                distanceFound = 300;
                petState = SCANNING;
            }

            if (buttonMode == MODE_4) {  // Reset parameters for MODE_4 find shorter path
                pathState = AXISTURN;
                lookingRight = 0;
                horquillasReseteadas = 0;
            }

            if (buttonMode == 5) 
                buttonMode = IDLE;
        }

        if(myButton[0].timeDiff > 3000 && buttonMode != IDLE) {   // If I get SW0 
            buttonMode = IDLE;
            moveServo(servoPrevio, 0);
        }
    }

/* END User code -------------------------------------------------------------*/
}
