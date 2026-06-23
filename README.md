








# 🏰 Medieval Tavern - Computer Graphics Project (2026)

## Team components:

* Ernesto Meneghini - [Matricola] - ernesto.meneghini@mail.polimi.it
* Edoardo Bergamo - [Matricola] - edoardo.bergamo@mail.polimi.it
* Simone Boscain - 10861057 - simone.boscain@mail.polimi.it

## Other info:
* 090958 - COMPUTER GRAPHICS
* A.Y. 2025/2026, 2nd semester
* Professor Gribaudo Marco

## Setup del Progetto & Compilazione (CLion)

Per evitare errori di compilazione con CMake e far funzionare correttamente Git sia per il codice che per il materiale di studio (`ispirational`), segui questa procedura esatta la prima volta che apri il progetto su CLion.

### 🛠️ Come aprire il progetto su CLion:
1. Fai il `git clone` del repository sul tuo PC.
2. Apri CLion e clicca su **Open**.
3. **⚠️ ATTENZIONE:** Seleziona la cartella radice principale del repository (`cgproject` / `CGProject2026`), **NON** la sottocartella `medievaltavern`.
4. CLion rileverà il file `CMakeLists.txt` globale che reindirizza la compilazione alla sottocartella, configurando tutto in automatico in un unico spazio di lavoro ordinato.

### 🎯 Vantaggi di questo setup:
- **Zero conflitti:** La cartella `cmake-build-debug` verrà generata solo nella root, senza doppioni fastidiosi.
- **Git sincronizzato:** Premendo `Ctrl + K`, vedrai sia le modifiche al codice sia i file dentro `ispirational`.
- **Assets funzionanti:** I modelli 3D e le texture verranno copiati automaticamente nella build senza errori di percorso.


## 🎮 Input Mapping (Controls)

The project utilizes standard GLFW input callbacks to handle camera movement and interaction. The default control mapping is structured as follows:

| Key / Input | Action | Description |
| :--- | :--- | :--- |
| **W** | Move Forward | Moves the camera forward along the look vector |
| **S** | Move Backward | Moves the camera backward along the look vector |
| **A** | Strafe Left | Moves the camera left, perpendicular to the look vector |
| **D** | Strafe Right | Moves the camera right, perpendicular to the look vector |
| **Mouse Move** | Look Around | Controls Orientation (Pitch and Yaw angles) |


