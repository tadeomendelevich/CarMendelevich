/*! \mainpage Archivo para <gregar variables/definiciones/constantes etc a un programa>
 * \date 01/01/2023
 * \author Nombre
 * \section genDesc Descripcion general
 * [Complete aqui con su descripcion]
 *
 * \section desarrollos Observaciones generales
 * [Complete aqui con sus observaciones]
 *
 * \section changelog Registro de cambios
 *
 * |   Fecha    | Descripcion                                    |
 * |:----------:|:-----------------------------------------------|
 * |10/09/2023 | Creacion del documento                         |
 *
 */


#ifndef UTIL_H_
#define UTIL_H_

/* Includes ------------------------------------------------------------------*/
#include "myDelay.h"
#include <stdlib.h>
/* END Includes --------------------------------------------------------------*/


/* typedef -------------------------------------------------------------------*/
/**
 * @brief Mapa de bits para declarar banderas
 * 
 */
typedef union{
    struct{
        uint8_t bit7 : 1;
        uint8_t bit6 : 1;
        uint8_t bit5 : 1;
        uint8_t bit4 : 1;
        uint8_t bit3 : 1;
        uint8_t bit2 : 1;
        uint8_t bit1 : 1;
        uint8_t bit0 : 1;
    }bits;
    uint8_t bytes;
}_uFlag;

typedef union{                          //union para definir la bandera, y no ocupar una variable booleana
    struct{
        uint8_t b0:1;
        uint8_t b1:1;
        uint8_t b2:1;
        uint8_t b3:1;
        uint8_t b4:1;
        uint8_t b5:1;
        uint8_t b6:1;
        uint8_t b7:1;
    }bit;
    uint8_t byte;
}_flag;

/**
 * 
 * @brief Unión ara la descomposición/composición de números mayores a 1 byte
 * 
 */
typedef union{
    uint32_t    ui32;
    int32_t     i32;
    uint16_t    ui16[2];
    int16_t     i16[2];
    uint8_t     ui8[4];
    int8_t      i8[4];
}_uWord;

/**
 * @brief estructura para la recepción de datos por puerto serie
 * 
 */
typedef struct{
    uint8_t *buff;      /*!< Puntero para el buffer de recepción*/
    uint8_t indexR;     /*!< indice de lectura del buffer circular*/
    uint8_t indexW;     /*!< indice de escritura del buffer circular*/
    uint8_t indexData;  /*!< indice para identificar la posición del dato*/
    uint8_t mask;       /*!< máscara para controlar el tamaño del buffer*/
    uint8_t chk;        /*!< variable para calcular el checksum*/
    uint8_t nBytes;     /*!< variable para almacenar el número de bytes recibidos*/
    uint8_t header;     /*!< variable para mantener el estado dela MEF del protocolo*/
    uint8_t timeOut;    /*!< variable para resetear la MEF si no llegan más caracteres luego de cierto tiempo*/
    uint8_t isComannd;
}_sRx;

/**
 * @brief Estructura para la transmisión de datos por el puerto serie
 * 
 */
typedef struct{
    uint8_t *buff;      /*!< Puntero para el buffer de transmisión*/
    uint8_t indexR;     /*!<indice de lectura del buffer circular*/
    uint8_t indexW;     /*!<indice de escritura del buffer circular*/
    uint8_t mask;       /*!<máscara para controlar el tamaño del buffer*/
    uint8_t chk;        /*!< variable para calcular el checksum*/
}_sTx;


/**
 * @brief estructura para el manejo de sensores
 * 
 */
typedef struct{
    uint16_t    currentValue;
    uint16_t    maxValue;
    uint16_t    minValue;
    uint16_t    blackValue;
    uint16_t    whiteValue;
}_sSensor;

typedef struct{
    uint32_t    currentValue;
    uint32_t    intervalValue;
    int16_t    X2;
    int16_t    X1;
    int16_t    Y2;
    int16_t    Y1;
}_sServo;

/**
 * @brief Struct que posee las variables necesarias para el conteo
 * 
 */
typedef struct{
    uint8_t state = 0;
    uint8_t counterStart[4];
    uint8_t counterEnd[4];
    uint8_t firstNumberCounted = 0;
    uint8_t i = 0;
    uint8_t counter = 0;
}_sCounter;

/**
 * @brief Enum de los estados de mi contador
 * 
 */
typedef enum{
    START, 
    ADD, 
    WAITBLACK,
    WAITWHITE,
    END
}_eCounter;





/**
 * @brief Enumeración para la maquina de estados
 * que se encarga de decodificar el protocolo
 * de comunicación
 *  
 */
typedef enum{
    HEADER_U,
    HEADER_N,
    HEADER_E,
    HEADER_R,
    NBYTES,
    TOKEN,
    PAYLOAD
}_eDecode;


/**
 * @brief Enumeración de los comandos del protocolo
 * 
 */
typedef enum{
    ALIVE = 0xF0,
    FIRMWARE= 0xF1,
    LEDSTATUS = 0x10,
    BUTTONSTATUS = 0x12,
    ANALOGSENSORS = 0xA0,
    SETBLACKCOLOR = 0xA6,
    SETWHITECOLOR = 0xB1,
    SETLINESPEED = 0xA7,
    MOTORTEST = 0xA1,
    SERVOANGLE = 0xA2,
    CONFIGSERVO = 0xA5,
    SERVOFINISHMOVE = 0x0A,
    GETDISTANCE = 0xA3,
    GETSPEED = 0xA4,
    SENDALLSENSORS = 0xA9,
    RADAR = 0xA8,
    SW0 = 0xB2,
    ACK = 0x0D,
    UNKNOWN = 0xFF
}_eCmd;

typedef union{ 
    uint8_t     u8[4];
    int8_t      i8[4];
    uint16_t    u16[2];
    int16_t     i16[2];
    uint32_t    u32;
    int32_t     i32;
    float       f;
}_work;

/**
 * @brief Enumeracion de los estados de seguimiento de linea
 * 
 */
typedef enum{
    FOLLOWING,
    OBSTACLE 
}_eLine;

/**
 * @brief Enumeracion de los estados para esquivar el obstaculo
 * 
 */
typedef enum{
    TURNCAR,
    CORRECTANGLE, 
    DODGE,
    TURNOPPOSITE, 
    RETURNLINE 
}_eObstacle;

int32_t slowSpeedValue = 6500; // Valor de seguimiento de linea de los motores

/**
 * @brief Enumeracion de los estados del angulo de escaneo del servo
 * 
 */
typedef enum{
    ANGLEADVANCING,
    ANGLEBACKING, 
    SCANNED
}_eAngle;

/**
 * @brief Enumeracion de los estados de mi mascota
 * 
 */
typedef enum{
    SCANNING,
    TURNINGTOOBJECT,
    MOVING,
    TURNING180
}_ePet;

/**
 * @brief Enumeracion de los estados de la maquina de estado
 * para encontrar el camino mas corto.
 * 
 */
typedef enum{
    AXISTURN,
    FINDCIRCLE,
    ANTICLOCKWISE,
    LEAVINGBOX,
    FINDPATH,
    CHANGESIDE, 
    CHECKOPENDOOR,
    MOVEALITTLE,
    MOVETOPATH,
    ADVANCEALITTLE,
    GOTONUMBERPATH,
    FOLLOWINGPATH,
    TAKEFIRSTNUMBER,
    GOINGOUTFIRSTPATH,
    TAKELASTNUMBER,
    GOOUTFROMSECONDPATH,
    FINDINGOTHERPATH,
    TURNTOLARGELANE,
    NEXTPATH, 
    ANOTHERPATH, 
    NOTCRASH,
    TURNINGTOPATH,
    TURNTOANOTHERPATH,
    TAKEANOTHERNUMBER,
    ADVANCE,
    RETURNTOCIRCLE,
    TAKEFINALNUMBER,
    GETOUTFINALNUMBER,
    GETINGINCIRLCE,
    INTOCIRCLE
}_ePath;


/**
 * @brief Enumeracion de los casos de moviimento de los motores
 * 
 */
typedef enum{
    adelante, 
    atras,
    derechasuave,
    derecha,
    izquierdasuave,
    izquierda,
    stop
}_eDirections;


/* END typedef ---------------------------------------------------------------*/

/* define --------------------------------------------------------------------*/

/* END define ----------------------------------------------------------------*/

/* hardware configuration ----------------------------------------------------*/


/* END hardware configuration ------------------------------------------------*/


/* Function prototypes -------------------------------------------------------*/

/* END Function prototypes ---------------------------------------------------*/


/* Global variables ----------------------------------------------------------*/

/* END Global variables ------------------------------------------------------*/

#endif