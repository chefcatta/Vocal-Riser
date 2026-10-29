# Vocal Riser — fra catta dsp

Plugin (AU / VST3) per creare riser sulle voci in pochi secondi, direttamente in sessione.
Il plugin **non modifica la traccia**: ascolta la voce, genera il riser e te lo fa trascinare nella timeline.

## Come si usa

1. Metti **Vocal Riser** come insert sulla traccia della voce.
2. Premi **CATTURA**, porta il playhead poco prima della frase e premi **play**: il plugin ascolta la frase.
3. Gira la manopola **INTENSITÀ** e scegli la **DURATA** (1/4, 1/2, 1 o 2 battute, a tempo col progetto).
4. Accendi **LOOP**: quando la DAW è ferma, il plugin suona il riser in loop così lo regoli a orecchio. In Logic tieni selezionata la traccia, altrimenti a trasporto fermo Logic non fa lavorare il plugin.
5. **Trascina la forma d'onda** su una traccia audio: il riser finisce esattamente dove attacca la voce, quindi allinea la **fine** della clip all'inizio della frase.

**RESET** riporta tutti i controlli ai valori iniziali (il riser catturato resta).

**SALVA** scrive il WAV (24 bit) in `Documenti/Vocal Riser` e apre la cartella: utile se la tua DAW non accetta il trascinamento dal plugin.

### Cosa fanno i pulsanti

| Pulsante | Effetto |
|---|---|
| FILTRO | Passa-alto che si apre durante il riser |
| STUTTER | Ripetizioni costanti su tutto il riser, allineate all'attacco della voce. Il **TIMING** si sceglie tra 1/4, 1/8 e 1/16 |
| PHONE | Suono "telefono" (banda stretta + saturazione) |
| CHORUS | Allarga e rende più morbida la coda |
| RIVERBERO | **HALL** (sala ampia, il suono originale), **PLATE** (denso e brillante), **SPACE** (coda enorme, scura e morbida) |
| IMMAGINE | Voce mono: scegli **MONO** o **STEREO** (allarga il riser). Voce stereo: il riser resta stereo e l'immagine della sorgente viene mantenuta |

Il plugin ascolta tutta la frase ma usa **solo la prima parte della prima parola** (l'attacco): trova da solo dove finisce la prima parola e ne prende il primo 40%, tra 80 e 250 ms.

Nel riser **non c'è voce dry**: l'attacco viene rovesciato e mandato in eco + riverbero al 100% wet. Si tiene solo la coda, che viene riversata e poi allungata (time-stretch, senza cambiare l'intonazione) fino alla durata scelta, con crescendo di volume e normalizzazione a −1 dBFS.

## Compilare il plugin

### Opzione A — senza installare niente (consigliata)
1. Crea un repository su GitHub e carica tutta questa cartella (compresa `.github`).
2. Vai su **Actions**: la build parte da sola (circa 10 minuti).
3. Scarica `VocalRiser-macOS` e/o `VocalRiser-Windows` dalla pagina della build.

**Mac:** copia `Vocal Riser.component` in `~/Library/Audio/Plug-Ins/Components/` e `Vocal Riser.vst3` in `~/Library/Audio/Plug-Ins/VST3/`. Il plugin non è firmato, quindi la prima volta lancia nel Terminale:

```
xattr -cr ~/Library/Audio/Plug-Ins/Components/"Vocal Riser.component"
```

Poi riapri Logic (se non compare: *Logic Pro → Impostazioni → Plug-in Manager → Reimposta e riesegui la scansione*).

**Windows (FL Studio / Cubase):** copia `Vocal Riser.vst3` in `C:\Program Files\Common Files\VST3\` e fai una nuova scansione dei plugin.

### Opzione B — sul tuo computer
Serve CMake 3.22+ e Xcode (Mac) o Visual Studio 2022 (Windows).

```
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release
```

Su Mac i plugin vengono copiati automaticamente nelle cartelle giuste. JUCE viene scaricato in automatico al primo `cmake`.

## Note
- La durata in battute assume il 4/4; il tempo è quello del progetto al momento della cattura.
- Ogni volta che premi play con AUTO acceso il plugin ricattura: spegni AUTO per tenere quella che hai.
