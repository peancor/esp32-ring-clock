# ESP32 RING CLOCK — Reloj LED WS2812B

Reloj analógico basado en un anillo de **60 LEDs WS2812B** controlado por un **ESP32**, sincronizado por NTP con zona horaria configurable.

Cada canal de color representa una manecilla:

| Canal | Manecilla | Estilo |
|-------|-----------|--------|
| 🔴 Rojo | Horas | Arco sólido de 5 LEDs con centro intenso |
| 🟢 Verde | Minutos | Respiración sinusoidal suave |
| 🔵 Azul | Segundos | Cometa con estela exponencial (modo por defecto) |

---

## Hardware

| Componente | Detalle |
|------------|---------|
| MCU | ESP32 DevKit v1 |
| LEDs | Anillo WS2812B × 60 |
| Pin data | GPIO 16 |
| LED estado | GPIO 4 (LED integrado) |
| Botón modo | GPIO 0 (BOOT) |

### Esquema de conexión

```
ESP32 GPIO 16 ──── DIN (WS2812B)
ESP32 GND    ──── GND (WS2812B)
ESP32 5V/VIN ──── 5V  (WS2812B)
```

> **Nota:** Para más de 30 LEDs encendidos a brillo alto, alimenta el anillo directamente desde una fuente de 5V, no solo desde el USB del ESP32.

---

## Configuración

Parámetros base del reloj en [`include/app_config.h`](include/app_config.h) y configuración de efectos cuco en [`src/main.cpp`](src/main.cpp) (bloque `setup()`).

| Define | Valor por defecto | Descripción |
|--------|:-----------------:|-------------|
| `PIN_WS2812B` | `16` | Pin de datos del anillo |
| `NUM_LEDS` | `60` | Número de LEDs en el anillo |
| `LED_OFFSET` | `30` | LED físico que corresponde a las "12 en punto" (0–59). Ajustar según orientación del anillo |
| `LED_PIN` | `4` | LED de estado de la placa |
| `BUTTON_PIN` | `0` | Botón para cambiar de modo (BOOT) |
| `DAY_BRIGHTNESS` | `90` | Brillo diurno (0–255) |
| `NIGHT_BRIGHTNESS` | `15` | Brillo nocturno (0–255) |
| `NIGHT_START` | `22` | Hora inicio modo noche (formato 24h) |
| `NIGHT_END` | `7` | Hora fin modo noche (formato 24h) |
| `NTP_RESYNC_INTERVAL_MS` | `86400000` | Periodo entre resincronizaciones NTP. Por defecto, una vez al dia |
| `NTP_RETRY_INTERVAL_MS` | `300000` | Espera entre reintentos si falla la sincronizacion WiFi o NTP |

### WiFi y zona horaria

Las credenciales WiFi viven en un archivo local no versionado: [include/wifi_secrets.h](include/wifi_secrets.h).

1. Copia [include/wifi_secrets.example.h](include/wifi_secrets.example.h) a [include/wifi_secrets.h](include/wifi_secrets.h).
2. Rellena `WifiSecrets::SSID` y `WifiSecrets::PASSWORD` con tus valores locales.
3. Compila normalmente con PlatformIO.

El archivo real queda fuera del repositorio mediante [.gitignore](.gitignore), así evitas subir secretos a GitHub.

```cpp
namespace WifiSecrets {
constexpr const char* SSID     = "tu-ssid";
constexpr const char* PASSWORD = "tu-password";
}

const char* TZ_INFO    = "CET-1CEST,M3.5.0,M10.5.0/3"; // España peninsular
```

Si este proyecto ya se compartió con credenciales reales, cambia la clave del WiFi. Quitarla del árbol actual no la borra del historial previo.

El reloj no mantiene una conexión WiFi permanente. Se conecta al arrancar para obtener la hora por NTP, se desconecta al terminar y vuelve a conectarse solo cuando vence `NTP_RESYNC_INTERVAL_MS`.

Si una sincronizacion falla, el reloj sigue funcionando con la hora disponible y reintenta pasado `NTP_RETRY_INTERVAL_MS`, en lugar de quedarse reconectando continuamente.

La cadena `TZ_INFO` sigue el formato POSIX. Ejemplos para otras zonas:

| Zona | TZ_INFO |
|------|---------|
| España peninsular | `CET-1CEST,M3.5.0,M10.5.0/3` |
| Canarias / UK | `GMT0BST,M3.5.0/1,M10.5.0` |
| Este de EE.UU. | `EST5EDT,M3.2.0,M11.1.0` |
| Argentina | `ART3` |

---

## Efectos cuco (minuto / cuarto / hora)

El reloj incorpora efectos luminosos configurables por frecuencia:

- **Minuto**: ligeros y rápidos
- **Cuarto de hora**: intermedios
- **Hora en punto**: más avanzados y llamativos

La selección es **aleatoria** entre los efectos activos de cada grupo.

Configuración actual en `setup()` de [`src/main.cpp`](src/main.cpp):

- `cfg.minuteEnabled`, `cfg.quarterEnabled`, `cfg.hourEnabled` para activar/desactivar cada frecuencia.
- `cfg.minuteEffectEnabled[...]`, `cfg.quarterEffectEnabled[...]`, `cfg.hourEffectEnabled[...]` para activar/desactivar efectos individuales.
- `cfg.minuteDurationMs`, `cfg.quarterDurationMs`, `cfg.hourDurationMs` para duración de cada categoría.

Implementación modular:

- Motor de reloj base: [`src/clock_renderer.cpp`](src/clock_renderer.cpp)
- Gestor de efectos cuco: [`src/chime_effects.cpp`](src/chime_effects.cpp)
- Interfaces: [`include/clock_renderer.h`](include/clock_renderer.h), [`include/chime_effects.h`](include/chime_effects.h)

---

## LED Offset

El define `LED_OFFSET` permite compensar la posición física del LED 0 del anillo respecto a donde quieres que estén las "12 en punto" del reloj.

```
          12 (segundo 0)
           ↑
     LED_OFFSET = 30
           ↑
   LED 30 del anillo
```

Si el LED 0 de tu anillo apunta a las 6, necesitas `LED_OFFSET = 30`.
Si apunta a las 3, usa `LED_OFFSET = 45`. Ajusta según montaje.

---

## Modos de visualización

Se cambian pulsando el botón **BOOT** (GPIO 0). Cada pulsación avanza al siguiente modo con un flash de confirmación.

### 1. Classic
Manecillas sólidas sin estelas. Horas como arco rojo, minutos como punto verde, segundos como punto azul. Limpio y directo.

### 2. Comet _(por defecto)_
El segundero se convierte en un cometa azul con estela de 5 LEDs y cabeza interpolada sub-píxel para movimiento suave. Los minutos respiran con pulso sinusoidal. Barra de progreso de segundos azul tenue de fondo.

### 3. Ambient
Segundos con halo suave y respiración rápida. Barras de progreso dobles (segundos en azul tenue + minutos en verde tenue). Minutos con LEDs laterales. Sensación orgánica y calmada.

### 4. Night
Modo mínimo: brillo muy bajo, sin estelas, sin barras de progreso, sin tick. Solo las tres manecillas como puntos individuales con respiración lenta en los minutos. Se activa **automáticamente** entre las 22:00 y las 07:00.

### 5. Sci-Fi
Grid de fondo con marcadores cada 5 LEDs y malla pulsante. Segundero con estela corta y limpia tipo terminal. Minutos con respiración. Estética tipo Iain M. Banks.

---

## Efectos y capas

El renderizado funciona con un **sistema de capas sobre un buffer aditivo RGB de 16 bits**. Las capas se acumulan y se fusionan antes de enviar a la tira.

### Capas (orden de dibujo)

1. **Fondo** — Grid Sci-Fi (solo en modo Sci-Fi)
2. **Barras de progreso** — Segundos (azul, valor 6/255) y minutos (verde, valor 3/255)
3. **Manecillas** — Horas (arco rojo), minutos (verde), segundos (azul, estilo según modo)
4. **Crossfade** — Decaimiento ease-out del LED anterior al cambiar posición
5. **Tick** — Flash de 30ms al cambiar segundo (+8R, +8G, +25B)

### Fusión inteligente de colores

Cuando dos o más manecillas coinciden en el mismo LED:

| Combinación | Resultado |
|-------------|-----------|
| Horas + Minutos | Amarillo cálido (R+G aditivo) |
| Minutos + Segundos | Cian (G+B aditivo) |
| Horas + Segundos | Magenta (R+B aditivo) |
| Horas + Minutos + Segundos | Blanco suave al 65% del pico |

Se evita saturar a 255 (máximo clampeado a 230).

### Transiciones suaves

- **Segundos:** ease-in cuadrático de 150ms para el LED nuevo, ease-out de 150ms en el anterior
- **Minutos:** ease-in de 500ms, ease-out de 500ms
- **Horas:** ease-out de 1000ms en la posición anterior
- **Sub-píxel:** en modos Comet y Sci-Fi, la cabeza del segundero se interpola entre dos LEDs para movimiento fluido

---

## Modo noche automático

Entre `NIGHT_START` (22:00) y `NIGHT_END` (07:00):

- Brillo global reducido a `NIGHT_BRIGHTNESS` (15/255)
- Forzado a comportamiento modo Night independientemente del modo seleccionado
- Sin estelas, sin barras de progreso, sin tick luminoso
- Solo las tres manecillas como puntos tenues

Funciona aunque `NIGHT_START > NIGHT_END` (cruce de medianoche).

---

## Build

Proyecto PlatformIO para ESP32:

```bash
# Compilar
pio run

# Compilar y subir
pio run --target upload

# Monitor serie
pio device monitor
```

### Dependencias

| Librería | Versión |
|----------|---------|
| Adafruit NeoPixel | ≥ 1.10.4 |
| WiFi (ESP32 built-in) | — |

---

## Uso de recursos

| Recurso | Uso |
|---------|-----|
| RAM | ~14% (45 KB / 328 KB) |
| Flash | ~58% (766 KB / 1.3 MB) |
| CPU | ~60 fps (loop de 16ms sin bloqueo) |

---

## Estructura del proyecto

```
ESP32-RING-CLOCK/
├── platformio.ini          # Configuración PlatformIO
├── README.md               # Este archivo
├── include/
│   ├── app_config.h        # Configuración global
│   ├── clock_renderer.h    # API del render del reloj
│   └── chime_effects.h     # API de efectos cuco
├── src/
│   ├── main.cpp            # Orquestación (setup/loop)
│   ├── clock_renderer.cpp  # Render y transiciones del reloj
│   └── chime_effects.cpp   # Efectos por minuto/cuarto/hora
├── lib/
└── test/
```

---

## Licencia

Proyecto personal. Uso libre.
