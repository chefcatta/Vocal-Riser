# Vocal Riser — Installazione

**CHEF CATTA dsp**

Scarica il file giusto per il tuo computer dalla pagina **Releases** del progetto:
<https://github.com/chefcatta/Vocal-Riser/releases/latest>

| Computer | File da scaricare | Cosa contiene |
|---|---|---|
| Mac (Apple Silicon e Intel, macOS 11 o successivo) | `VocalRiser-mac.zip` | `Vocal Riser.component` (Audio Unit) e `Vocal Riser.vst3` |
| Windows 10/11 a 64 bit | `VocalRiser-windows.zip` | `Vocal Riser.vst3` |

Quale formato usa la tua DAW:

| DAW | Mac | Windows |
|---|---|---|
| Logic Pro, GarageBand | Audio Unit (`.component`) | — |
| Ableton Live | VST3 (oppure Audio Unit) | VST3 |
| FL Studio | VST3 | VST3 |
| Cubase, Nuendo | VST3 | VST3 |
| Studio One | VST3 (oppure Audio Unit) | VST3 |
| Reaper | VST3 (oppure Audio Unit) | VST3 |
| Bitwig Studio | VST3 | VST3 |
| Pro Tools | non ancora disponibile, vedi in fondo | non ancora disponibile |

---

## 1. Copiare il plugin nella cartella giusta

### Mac

1. Fai doppio clic su `VocalRiser-mac.zip` per decomprimerlo.
2. Nel Finder premi **Cmd + Shift + G** e incolla il percorso della cartella:
   - `Vocal Riser.component` → `/Library/Audio/Plug-Ins/Components`
   - `Vocal Riser.vst3` → `/Library/Audio/Plug-Ins/VST3`

   Copia tutti e due: così il plugin funziona in qualsiasi DAW. Se il Finder chiede la password o se vuoi sostituire una versione precedente, conferma.
3. **Sblocca il plugin.** Il plugin non è firmato da Apple, quindi la prima volta macOS lo blocca. Apri il **Terminale** (Cmd + Spazio, scrivi «Terminale»), incolla queste due righe e premi Invio. Ti chiederà la password del Mac: mentre la scrivi non compare nulla, è normale.
   ```
   sudo xattr -cr "/Library/Audio/Plug-Ins/Components/Vocal Riser.component"
   sudo xattr -cr "/Library/Audio/Plug-Ins/VST3/Vocal Riser.vst3"
   ```
   In alternativa, se all'apertura della DAW compare l'avviso *«Vocal Riser… non è stata aperta»*, premi **Fine** (non «Sposta nel Cestino»). Poi vai su **Impostazioni di Sistema → Privacy e sicurezza**, scorri fino a **Sicurezza** e premi **«Apri comunque»**.

### Windows

1. **Prima di decomprimere** fai clic destro su `VocalRiser-windows.zip` → **Proprietà**. Se in basso c'è la casella **«Sblocca»**, spuntala e premi OK.
2. Clic destro sullo zip → **Estrai tutto**.
3. Copia la cartella `Vocal Riser.vst3` in:
   ```
   C:\Program Files\Common Files\VST3
   ```
   Windows chiederà il permesso di amministratore: conferma.

---

## 2. Far trovare il plugin alla DAW

Dopo aver copiato il plugin, apri (o riavvia) la DAW e fai una nuova scansione dei plugin. Lo sviluppatore è **Chef Catta** e il plugin si chiama **Vocal Riser**.

### Logic Pro (Mac)
1. Apri Logic. Al primo avvio scansiona da solo i nuovi plugin.
2. Se non compare, vai su **Logic Pro → Impostazioni → Plug-in Manager**. Cerca «Vocal», seleziona la riga e premi **Reset & Rescan Selection** (in basso a sinistra). Controlla che la casella **Usa** sia spuntata.
3. Su una traccia audio: slot **Audio FX → Audio Units → Chef Catta → Vocal Riser**.

### GarageBand (Mac)
1. Apri GarageBand e seleziona la traccia della voce.
2. Apri gli **Smart Controls** (tasto B), poi la sezione **Plug-in**. In uno slot vuoto scegli **Audio Units → Chef Catta → Vocal Riser**.

### Ableton Live (Mac e Windows)
1. **Impostazioni** (Mac: *Live → Impostazioni*; Windows: *Options → Preferences*) → scheda **Plug-in**.
2. Attiva **Usa cartelle di sistema per plug-in VST3** («Use VST3 Plug-In System Folders»). Su Mac puoi attivare anche **Usa Audio Units**.
3. Premi **Ripeti scansione** (Rescan).
4. Nel browser, sotto **Plug-in**, apri **Chef Catta** e trascina **Vocal Riser** sulla traccia della voce.

### FL Studio (Mac e Windows)
1. **Options → Manage plugins**.
2. Premi **Find installed plugins** (oppure *Start scan*) e attendi la fine della scansione.
3. Nel Mixer, sulla traccia della voce, apri uno slot vuoto e scegli **Vocal Riser**. Lo trovi anche nel *Plugin database* sotto *Effects*.

### Cubase / Nuendo (Mac e Windows)
1. Cubase scansiona i plugin VST3 all'avvio: riavvialo dopo l'installazione.
2. Se non lo vedi: **Studio → VST Plug-in Manager**. Se è nella lista dei plug-in bloccati, selezionalo e riattivalo.
3. Sulla traccia della voce: **Insert → Chef Catta → Vocal Riser**.

### Studio One (Mac e Windows)
1. Studio One scansiona i plugin all'avvio: riavvialo.
2. Se non compare: **Studio One → Opzioni** (Windows: *Studio One → Options*) → **Posizioni → Plug-in VST**. Controlla che ci sia la cartella VST3 di sistema, premi **Reimposta blocklist** e riavvia.
3. Nel browser **Effetti** trovi **Vocal Riser** sotto **Chef Catta**: trascinalo sulla traccia.

### Reaper (Mac e Windows)
1. **Options → Preferences → Plug-ins → VST**.
2. Controlla che il percorso dei VST3 sia incluso e premi **Re-scan** (oppure *Clear cache/re-scan*).
3. Sulla traccia: pulsante **FX → Add → All plugins → Vocal Riser (Chef Catta)**. Su Mac lo trovi anche tra gli **AU**.

### Bitwig Studio (Mac e Windows)
1. **Settings → Locations**: controlla che la posizione VST3 predefinita sia attiva. Bitwig scansiona da solo.
2. Nel browser dei dispositivi cerca **Vocal Riser** e trascinalo sulla traccia.

---

## 3. Come si usa (in breve)

1. Metti **Vocal Riser** come effetto (insert) sulla traccia della voce.
2. Premi **CATTURA**, porta il playhead poco prima della frase e premi **play**: il plugin ascolta la frase e crea il riser dalla prima parte della prima parola.
3. Regola **INTENSITÀ**, **DURATA**, **AMBIENTE**, **IMMAGINE** e gli effetti.
4. Accendi **LOOP** per ascoltare il riser in loop a trasporto fermo. In Logic tieni selezionata la traccia.
5. **Trascina la forma d'onda** su una traccia audio e allinea la **fine** della clip all'attacco della voce. Se la tua DAW non accetta il trascinamento, premi **SALVA**: il WAV finisce in `Documenti/Vocal Riser`.

---

## Pro Tools

Pro Tools usa solo il formato **AAX** di Avid, che per ora non è disponibile per Vocal Riser. Per crearlo servono il kit di sviluppo di Avid e una firma digitale (PACE/iLok) senza cui Pro Tools non carica il plugin.

Nel frattempo puoi:
- usare Vocal Riser in un'altra DAW, anche gratuita (per esempio Reaper in prova), salvare il riser con **SALVA** e importare il WAV in Pro Tools;
- oppure usare un plugin "contenitore" AAX che carica i VST3 dentro Pro Tools, come **Blue Cat's PatchWork** (prodotto a pagamento di terze parti).

---

## Problemi frequenti

- **Mac: «non è stata aperta» / «è danneggiato»** → esegui i comandi del passo 1.3 oppure premi «Apri comunque» in Privacy e sicurezza, poi rifai la scansione nella DAW.
- **La DAW non trova il plugin** → controlla di averlo copiato nella cartella esatta indicata sopra (su Mac è `/Library`, non la cartella *Libreria* del tuo utente) e fai una nuova scansione.
- **Windows: il plugin non compare** → controlla che la cartella copiata si chiami `Vocal Riser.vst3` e sia dentro `C:\Program Files\Common Files\VST3`. Le DAW a 32 bit non sono supportate.
- **Il loop non si sente in Logic** → seleziona la traccia su cui c'è il plugin: a trasporto fermo Logic fa lavorare solo la traccia selezionata. Se sul display compare «LOOP: SELEZIONA LA TRACCIA», è proprio questo il motivo.
- **Aggiornare a una nuova versione** → chiudi la DAW, sostituisci i file nelle stesse cartelle, sbloccali di nuovo (passo 1.3) e riapri la DAW. I riser catturati nelle sessioni salvate restano.
