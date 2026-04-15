# Smart Autonomous Car · Vehículo Autónomo Embebido

<div align="center">

![Platform](https://img.shields.io/badge/Plataforma-mbed-000000?style=for-the-badge&logo=arm&logoColor=white)
![Language](https://img.shields.io/badge/Lenguaje-C%2B%2B-00599C?style=for-the-badge&logo=cplusplus&logoColor=white)
![Comms](https://img.shields.io/badge/Comunicación-USART%20%2F%20WiFi-FF6600?style=for-the-badge)
![Status](https://img.shields.io/badge/Estado-Funcional-brightgreen?style=for-the-badge)

<br/>

> **Vehículo autónomo completo sobre plataforma mbed:**
> seguimiento de línea, evasión de obstáculos, radar con servo,
> navegación con selección de ruta y telemetría WiFi/serial —
> **sin RTOS · sin delays · sin intervención humana.**

<br/>

[![LinkedIn](https://img.shields.io/badge/Tadeo_Mendelevich-0A66C2?style=for-the-badge&logo=data:image/svg%2bxml;base64,PHN2ZyByb2xlPSJpbWciIHZpZXdCb3g9IjAgMCAyNCAyNCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBmaWxsPSJ3aGl0ZSIgZD0iTTIwLjQ0NyAyMC40NTJoLTMuNTU0di01LjU2OWMwLTEuMzI4LS4wMjctMy4wMzctMS44NTItMy4wMzctMS44NTMgMC0yLjEzNiAxLjQ0NS0yLjEzNiAyLjkzOXY1LjY2N0g5LjM1MVY5aDMuNDE0djEuNTYxaC4wNDZjLjQ3Ny0uOSAxLjYzNy0xLjg1IDMuMzctMS44NSAzLjYwMSAwIDQuMjY3IDIuMzcgNC4yNjcgNS40NTV2Ni4yODZ6TTUuMzM3IDcuNDMzYTIuMDYyIDIuMDYyIDAgMCAxLTIuMDYzLTIuMDY1IDIuMDY0IDIuMDY0IDAgMSAxIDIuMDYzIDIuMDY1em0xLjc4MiAxMy4wMTlIMy41NTVWOWgzLjU2NHYxMS40NTJ6TTIyLjIyNSAwSDEuNzcxQy43OTIgMCAwIC43NzQgMCAxLjcyOXYyMC41NDJDMCAyMy4yMjcuNzkyIDI0IDEuNzcxIDI0aDIwLjQ1MUMyMy4yIDI0IDI0IDIzLjIyNyAyNCAyMi4yNzFWMS43MjlDMjQgLjc3NCAyMy4yIDAgMjIuMjIyIDBoLjAwM3oiLz48L3N2Zz4=&logoColor=white)](https://www.linkedin.com/in/tadeo-mendelevich/)
[![GitHub](https://img.shields.io/badge/tadeomendelevich-181717?style=for-the-badge&logo=github&logoColor=white)](https://github.com/tadeomendelevich)

</div>

---

## ¿Qué hace este proyecto?

Vehículo autónomo con percepción multisensor y cuatro modos de operación independientes. La lógica completa corre en un microcontrolador compatible con mbed, coordinando motores, servo, sensores y comunicación dentro de una arquitectura de tareas sin sistema operativo.

<div align="center">

### 🚗 Sistema en funcionamiento

<img src="car.gif" width="400"/>

### 📸 Vista del sistema — versión 2

<img src="setup.png" width="700"/>

</div>

| Módulo | Descripción |
|--------|-------------|
| 📏 **HC-SR04** | Medición de distancia por ultrasonido — disparo y captura por interrupciones, sin busy-wait |
| 🔦 **IR ×3** | Sensores izquierda, centro y derecha para seguimiento de línea negra y detección de bifurcaciones |
| 🔁 **Encoders** | Cálculo de velocidad real por interrupción en flancos — odometría para giros controlados |
| ⚙️ **Servo radar** | Escaneo angular del entorno para detectar el objeto más cercano antes de moverse |
| 📡 **Protocolo UNER** | Trama binaria estructurada sobre USART y UDP — ALIVE, FIRMWARE, sensores, motores, servo |
| 🧠 **Máquina de estados** | Cuatro modos de operación con transiciones limpias: IDLE, línea, distancia y exploración |

---

## Arquitectura

```
Loop principal — arquitectura de tareas cooperativas
│
├── serialTask()    ──► USART / WiFi ──► Parser UNER ──► comandos entrantes
│
├── distanceTask()  ──► HC-SR04 ──► ISR eco ──► distancia_cm
│
├── irSensorsTask() ──► IR ×3 ──► posición sobre línea ──► error lateral
│
├── speedTask()     ──► encoders por INT ──► velocidad_rpm ──► odometría
│
├── servoTask()     ──► PWM servo ──► ángulo ──► mapa de distancias (radar)
│
└── Máquina de estados
        ├── Modo 0 — IDLE       ──► heartbeat LED
        ├── Modo 1 — Línea      ──► followLine() + corrección dinámica
        ├── Modo 3 — Distancia  ──► radar + control P ──► mantiene separación
        └── Modo 4 — Exploración──► navegación + lectura de códigos IR + ruta óptima
```

> Todas las tareas son no bloqueantes. La sincronización se resuelve con flags de interrupción
> y timestamps — sin `wait()`, sin `Thread::wait()`.

---

## Lo más interesante del código

#### 🧠 Máquina de estados con cuatro personalidades
Cada modo tiene su propia lógica de percepción y actuación. Las transiciones son limpias: entrar a un modo reinicia su estado interno, salir lo deja en condición segura. El vehículo nunca queda en un estado indefinido.

#### 📡 Radar con servo
Antes de moverse en Modo 3, el servo barre el arco frontal tomando muestras de distancia cada pocos grados. El firmware construye un mapa angular mínimo en memoria y apunta el vehículo al objeto más cercano — todo en enteros, sin floats.

#### 🔁 Seguidor de línea con corrección dinámica
Los tres sensores IR generan un error lateral de −1, 0 o +1 (y combinaciones). La corrección se aplica directamente al PWM diferencial de los motores — simple, rápido y sin overshooting en curvas lentas.

#### 🗺️ Navegación con selección de ruta
El Modo 4 lee "códigos" en el piso mediante patrones de los sensores IR en las bifurcaciones. El firmware acumula el recorrido y elige la rama más corta al llegar al destino — navegación reactiva con memoria local.

#### 🔌 Telemetría dual USART + WiFi
El mismo parser UNER atiende el puerto serial USB y el módulo ESP simultáneamente. El comando recibido por cualquiera de los dos canales tiene el mismo efecto — la capa de transporte es transparente para la lógica de control.

---

## Hardware

| Componente | Detalle |
|------------|---------|
| MCU | mbed compatible (Cortex-M) |
| Tracción | 2× motor DC con control PWM diferencial |
| Dirección | Servo motor — PWM 50 Hz |
| Distancia | HC-SR04 — 2 cm a 400 cm por Input Capture |
| Línea | IR × 3 — izquierda, centro, derecha |
| Velocidad | Encoders de horquilla por interrupción externa |
| Comunicación | USART (USB) + WiFi UDP (módulo ESP) |
| Protocolo | UNER binario — ALIVE, FIRMWARE, sensores, actuadores |

---

## Estructura del proyecto

```
├── main.cpp          — Máquina de estados, inicialización, loop de tareas
├── serialTask()      — Parser UNER, USART y WiFi
├── distanceTask()    — Driver HC-SR04 con ISR
├── irSensorsTask()   — Lectura IR y cálculo de error lateral
├── speedTask()       — Encoders y odometría
├── servoTask()       — Control angular y radar
├── followLine()      — Lógica de seguimiento de línea
├── PID()             — Control proporcional de distancia
└── turn()            — Giros controlados por odometría
```

---

<div align="center">

**Tadeo Mendelevich** · Ingeniería en Sistemas · UNER — Concordia, Entre Ríos

[![LinkedIn](https://img.shields.io/badge/LinkedIn-Conectar-0A66C2?style=flat&logo=data:image/svg%2bxml;base64,PHN2ZyByb2xlPSJpbWciIHZpZXdCb3g9IjAgMCAyNCAyNCIgeG1sbnM9Imh0dHA6Ly93d3cudzMub3JnLzIwMDAvc3ZnIj48cGF0aCBmaWxsPSJ3aGl0ZSIgZD0iTTIwLjQ0NyAyMC40NTJoLTMuNTU0di01LjU2OWMwLTEuMzI4LS4wMjctMy4wMzctMS44NTItMy4wMzctMS44NTMgMC0yLjEzNiAxLjQ0NS0yLjEzNiAyLjkzOXY1LjY2N0g5LjM1MVY5aDMuNDE0djEuNTYxaC4wNDZjLjQ3Ny0uOSAxLjYzNy0xLjg1IDMuMzctMS44NSAzLjYwMSAwIDQuMjY3IDIuMzcgNC4yNjcgNS40NTV2Ni4yODZ6TTUuMzM3IDcuNDMzYTIuMDYyIDIuMDYyIDAgMCAxLTIuMDYzLTIuMDY1IDIuMDY0IDIuMDY0IDAgMSAxIDIuMDYzIDIuMDY1em0xLjc4MiAxMy4wMTlIMy41NTVWOWgzLjU2NHYxMS40NTJ6TTIyLjIyNSAwSDEuNzcxQy43OTIgMCAwIC43NzQgMCAxLjcyOXYyMC41NDJDMCAyMy4yMjcuNzkyIDI0IDEuNzcxIDI0aDIwLjQ1MUMyMy4yIDI0IDI0IDIzLjIyNyAyNCAyMi4yNzFWMS43MjlDMjQgLjc3NCAyMy4yIDAgMjIuMjIyIDBoLjAwM3oiLz48L3N2Zz4=&logoColor=white)](https://www.linkedin.com/in/tadeo-mendelevich/)
[![GitHub](https://img.shields.io/badge/GitHub-tadeomendelevich-181717?style=flat&logo=github)](https://github.com/tadeomendelevich)

</div>
