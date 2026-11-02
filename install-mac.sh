#!/bin/bash
# Chef Riser — installazione/aggiornamento automatico su Mac
# Scarica l'ultima versione da GitHub e la installa in Macintosh HD → Libreria → Audio → Plug-Ins.
# Scaricando dal Terminale macOS non applica la quarantena: nessun avviso in Privacy e sicurezza.
set -e

URL="https://github.com/chefcatta/Vocal-Riser/releases/latest/download/ChefRiser-mac.zip"
TMP="$(mktemp -d /tmp/chefriser.XXXXXX)"
COMP="/Library/Audio/Plug-Ins/Components"
VST3="/Library/Audio/Plug-Ins/VST3"

echo ""
echo "  CHEF RISER — CHEF CATTA dsp"
echo "  Scarico l'ultima versione..."
curl -fsSL "$URL" -o "$TMP/ChefRiser-mac.zip"
ditto -x -k "$TMP/ChefRiser-mac.zip" "$TMP"

echo "  Installo (ti verra chiesta la password del Mac: mentre la scrivi non compare nulla)..."
sudo mkdir -p "$COMP" "$VST3"
sudo rm -rf "$COMP/Vocal Riser.component" "$VST3/Vocal Riser.vst3" \
            "$COMP/Chef Riser.component"  "$VST3/Chef Riser.vst3"
sudo cp -R "$TMP/Chef Riser.component" "$COMP/"
sudo cp -R "$TMP/Chef Riser.vst3" "$VST3/"
sudo xattr -cr "$COMP/Chef Riser.component" "$VST3/Chef Riser.vst3"
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true
rm -rf "$TMP"

echo ""
echo "  Fatto! Chef Riser e installato in:"
echo "   - Macintosh HD > Libreria > Audio > Plug-Ins > Components (Logic, GarageBand)"
echo "   - Macintosh HD > Libreria > Audio > Plug-Ins > VST3 (altre DAW)"
echo ""
echo "  Apri la DAW. In Logic: Plug-in Manager > cerca \"Chef\" > Reset & Rescan Selection."
echo ""
