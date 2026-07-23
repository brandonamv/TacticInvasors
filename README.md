# TacticInvasors

Este documento explica las reglas del juego, las variables principales y la configuración inicial, la cual se controla a través del archivo `matrix.txt`.

## ¿De qué trata el juego?

`TacticInvasors` es una simulación en la que interactúan distintos tipos de seres o entidades (por ejemplo, Agresivos y Pasivos) dentro de un mismo entorno. El juego hace un seguimiento de los recursos, el nivel de energía (aptitud o *fitness*) y cómo crece o disminuye la población según las interacciones y reglas establecidas.

## Archivo de Configuración: `matrix.txt`

El archivo `matrix.txt` define los parámetros principales del juego y las reglas de interacción. Cada línea representa una variable o una regla específica.

### Variables Globales

Estas variables controlan la economía general del juego y las características de las entidades:

* **`v` (Ganancia de recursos):** La cantidad de recursos que obtiene una entidad tras una interacción exitosa.
  * **Valor actual:** `1`
* **`c` (Costo del recurso):** El costo o penalización por intentar obtener un recurso (se puede entender como gasto de energía).
  * **Valor actual:** `2`
* **`m` (Pérdida constante de energía):** La velocidad a la que disminuye la energía de una entidad con el paso del tiempo.
  * **Valor actual:** `0.2`
* **`i` (Energía inicial):** La cantidad de energía con la que nace o aparece una nueva entidad.
  * **Valor actual:** `0.5`
* **`r` (Reservado):** Variable no disponible o sin uso actualmente.
  * **Valor actual:** `0`
* **`u` (Límite para reproducirse):** El nivel de energía que debe alcanzar o superar una entidad para poder multiplicarse.
  * **Valor actual:** `2.0`
* **`s` (Velocidad del juego):** Multiplicador de la velocidad de la simulación. Un valor de `1.0` equivale a velocidad normal.
  * **Valor actual:** `1.0` (Velocidad 1x)
* **`p` (Iteraciones):** El número total de pasos o turnos que durará la simulación.
  * **Valor actual:** `10`

### Reglas de Interacción entre Entidades

Las líneas que comienzan con números (`0=`, `1=`, etc.) definen la cantidad inicial y el comportamiento de cada tipo de entidad. Siguen este formato:

`<ID_Tipo_Entidad>=<Resultado_vs_Agresivo>;<Resultado_vs_Pasivo>;<Cantidad_Inicial>`

Donde:
* `<ID_Tipo_Entidad>`: Código único para identificar el tipo de entidad (por ejemplo, `0` para Agresivo, `1` para Pasivo).
* `<Resultado_vs_Agresivo>`: La fórmula o resultado obtenido al interactuar con una entidad "Agresiva".
* `<Resultado_vs_Pasivo>`: La fórmula o resultado obtenido al interactuar con una entidad "Pasiva".
* `<Cantidad_Inicial>`: La cantidad de entidades de este tipo al iniciar la simulación.

Según el archivo `matrix.txt`:

#### Tipo de Entidad `0`: Agresivo

* **Comportamiento:** Entidades que compiten activamente por los recursos.
* **Al interactuar con otro Agresivo (Tipo 0):** `0.5 * (v - c)`
  * Obtiene la mitad de la diferencia entre la ganancia y el costo del recurso.
* **Al interactuar con un Pasivo (Tipo 1):** `v`
  * Se queda con la ganancia total del recurso.
* **Población inicial:** `1`
  * La simulación comienza con 1 entidad Agresiva.

#### Tipo de Entidad `1`: Pasivo

* **Comportamiento:** Entidades que evitan el conflicto.
* **Al interactuar con un Agresivo (Tipo 0):** `0`
  * No obtiene ningún beneficio al enfrentarse a un Agresivo.
* **Al interactuar con otro Pasivo (Tipo 1):** `v / 2`
  * Comparte el recurso, obteniendo la mitad de la ganancia.
* **Población inicial:** `10`
  * La simulación comienza con 10 entidades Pasivas.

## Cómo Empezar

Para ejecutar la simulación, asegúrate de colocar el archivo `matrix.txt` en la siguiente ruta: `TacticInvasors/Saved/matrix.txt`. El juego iniciará automáticamente con los parámetros y poblaciones configurados.

Puedes presionar la tecla **Escape** en cualquier momento para salir del juego.