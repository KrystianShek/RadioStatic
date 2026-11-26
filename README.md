<div align="center">

# 📻 RadioStatic

### Knacken · Rauschen · Pfeifen · Fading · Funkstimmen
**Das Mittelwellen- & Kurzwellen-Feeling als VST3-Effekt-Plugin**

[![Made with Claude](https://img.shields.io/badge/Made%20with-Claude-D97757?style=for-the-badge&logo=anthropic&logoColor=white)](https://claude.ai)
[![Format](https://img.shields.io/badge/Format-VST3-FF9F1C?style=for-the-badge)](#)
[![Framework](https://img.shields.io/badge/Framework-JUCE%208-1F6FEB?style=for-the-badge)](https://juce.com)
[![Built with](https://img.shields.io/badge/Build-CMake-064F8C?style=for-the-badge&logo=cmake&logoColor=white)](https://cmake.org)

*Designed by **Krystian Shek***

</div>

---

## ✨ Was ist RadioStatic?

RadioStatic schickt dein Signal durch ein altes Radio – oder besser gesagt: mitten hinein in
die Störungen zwischen zwei Sendern. Du schaltest zwischen **Mittelwelle (MW)** und
**Kurzwelle (SW)** um, drehst am **Mix**-Regler und bestimmst selbst, wie viel Äther-Chaos
zu deinem Sound hinzukommt.

Perfekt für Lo-Fi-Intros, Film- und Hörspiel-Atmosphäre, Spionage-Vibes, „Zahlensender“-Stimmung
oder einfach, um einen zu sauberen Mix mit etwas Patina zu versehen.

```
┌──────────────────────────────────────────────────────────────┐
│                        R A D I O S T A T I C                 │
│  ┌────────────────────────────────────────────────────────┐  │
│  │ MW  520   600   700   800  │ 1000  1200  1400  1600 kHz│  │
│  │ SW   3     5     7     9   │ 11   13    16   19   22 MHz│  │
│  │ ~~~~~~∿∿~~~~∿~~~/\~~∿∿~~~~~~~∿~~~~~/\/~~~~~∿∿~~~~~~~~~ │  │
│  └────────────────────────────────────────────────────────┘  │
│  ┌──────────────┐   ◯ NOISE  ◯ CRACKLE  ◯ WHISTLE  ◯ FADING │
│  │ MEDIUM  SHORT│   ◯ VOICES                       ┌───────┐ │
│  │ ●━━━━━━○     │                                  │  MIX  │ │
│  └──────────────┘                                  └───────┘ │
│                       Designed by Krystian Shek              │
└──────────────────────────────────────────────────────────────┘
```
<sub>Schematische Darstellung – die echte Oberfläche ist eine leuchtende Glasskala mit
Metallreglern und LED-Ringen im Retro-Look.</sub>

---

## 🎛️ Bedienung

| Regler | Standard | Was er macht |
|---|:---:|---|
| **MEDIUM WAVE / SHORTWAVE** | MW | Schiebeschalter. Wechselt den Charakter der Störungen (siehe unten). |
| **MIX** | 35 % | Wie viel Radio-Klang samt Störungen zu deinem Signal gemischt wird. 0 % = sauber, 100 % = voll im Äther. |
| **NOISE** | 50 % | Rauschen – bei Kurzwelle mit „wuschelndem“, wanderndem Filter. |
| **CRACKLE** | 50 % | Knacken und atmosphärische Entladungen, in Schüben wie bei einem fernen Gewitter. |
| **WHISTLE** | 30 % | Heterodyn-Pfeiftöne von Nachbarsendern, die ein- und ausblenden und gleiten. |
| **FADING** | 40 % | Lautstärke-Schwund des Signals, wie es bei weit entfernten Sendern typisch ist. |
| **VOICES** | 25 % | Ab und zu zerhackte Funkstimmen. Höher = häufigere und stärker verzerrte Durchsagen. |

> 💡 **Tipp:** Doppelklick auf einen Regler setzt ihn auf den Standardwert zurück.
> Das Fenster lässt sich frei skalieren.

### MW oder SW?

| | 📡 Mittelwelle | 🌍 Kurzwelle |
|---|---|---|
| **Klang** | breiter, gleichmäßiger | schmalbandiger, „weiter weg“ |
| **Rauschen** | konstant | wandernd, wuschelig |
| **Knacken** | häufig, in Schüben | seltener |
| **Pfeifen** | stabile Töne | wild gleitende Töne |
| **Fading** | langsam und sanft | schnell und unregelmäßig |

### Die Anzeige lebt

Die Nadel auf der Skala wandert wie bei einem Sendersuchlauf, und darunter läuft live die
Kurve der Störungen mit. Je mehr Mix, desto wilder der Ausschlag.

---

## 🎙️ Funkstimmen

Der Regler **VOICES** spielt zufällig kurze „Durchsagen“ ab, die aus einer **eingebetteten
Sprachaufnahme** stammen (`Resources/voice.wav`). Das Plugin zerlegt die Aufnahme automatisch
an den Sprechpausen in Sätze und spielt pro Durchsage 1–3 davon ab. Sie werden dabei wie echter
Funkverkehr behandelt:

- 📞 Bandbegrenzung auf Sprechfunk-Bereich (ca. 380–2600 Hz)
- 🔥 starke Übersteuerung für den harten, komprimierten Funk-Sound
- 🧱 Sample-Rate-Crunch für körnige, digitale Übertragungsartefakte
- 🔔 *Squelch*-Klick am Anfang und „kschhk“ am Ende jeder Durchsage
- 🌫️ leise rauschender, offener Träger während der Durchsage
- 🌊 Fading blendet die Stimmen ein und aus

**Eigene Stimme verwenden:** Ersetze einfach `Resources/voice.wav` durch eine beliebige
WAV-Datei mit demselben Namen und baue das Plugin neu. Die Sätze werden automatisch neu erkannt.

---

## 🔧 Selbst bauen

### Voraussetzungen

- [CMake](https://cmake.org) 3.22 oder neuer
- Ein C++-Compiler (Windows: Visual Studio mit „Desktopentwicklung mit C++“, macOS: Xcode,
  Linux: GCC/Clang)
- [JUCE 8](https://github.com/juce-framework/JUCE) als Unterordner `JUCE/`

### Schritte

```bash
# 1) JUCE neben die CMakeLists.txt klonen
git clone https://github.com/juce-framework/JUCE.git

# 2) Konfigurieren
cmake -B build

# 3) Bauen
cmake --build build --config Release
```

Das fertige Plugin liegt danach unter:

```
build/RadioStatic_artefacts/Release/VST3/RadioStatic.vst3
```

Kopiere die `.vst3` in deinen VST3-Ordner und lass die DAW neu nach Plugins suchen:

| System | VST3-Ordner |
|---|---|
| Windows | `C:\Program Files\Common Files\VST3` |
| macOS | `/Library/Audio/Plug-Ins/VST3` |
| Linux | `~/.vst3` |

Zusätzlich entsteht ein **Standalone**-Programm, mit dem du den Effekt auch ohne DAW
ausprobieren kannst.

---

## 🗂️ Projektstruktur

```
RadioStatic/
├── CMakeLists.txt            Build-Konfiguration (inkl. eingebetteter Sprachaufnahme)
├── JUCE/                     JUCE-Framework (selbst klonen)
├── Resources/
│   └── voice.wav             eingebettete Funkstimme
└── Source/
    ├── PluginProcessor.h/.cpp   Klang: Rauschen, Knacken, Pfeifen, Fading, Funkstimmen
    ├── PluginEditor.h/.cpp      Oberfläche: Layout und Metall-Hintergrund
    └── UiComponents.h           Glasskala, Schiebeschalter, Regler mit LED-Ring
```

### Klang justieren

Die wichtigsten Stellschrauben stehen direkt im Code in `PluginProcessor.cpp`:

- **Lautstärke der Funkstimmen:** Zeile mit `chatLevel`
- **Charakter von MW und SW** (Bandbreite, Knack-Dichte, Fading-Tempo): Block am Anfang von `processBlock`
- **Pausen zwischen den Durchsagen:** Funktion `next` in `RadioVoice` (`PluginProcessor.h`)

---

## 🛠️ Technik in Kürze

- **Rauschen:** gefiltertes Weißrauschen, bei SW mit wanderndem Tiefpass
- **Knacken:** zufällige Impulse mit kurzer Abklingzeit, in Gewitter-artigen Schüben
- **Pfeifen:** zwei driftende Sinus-Oszillatoren mit zufälligem Ein- und Ausblenden
- **Fading:** drei überlagerte langsame Modulatoren (unregelmäßiger Schwund)
- **Radio-Klang:** Band-Begrenzung aus je zwei Hoch- und Tiefpässen plus sanfte Sättigung
- **Anzeige:** Oszilloskop-Daten und Nadelposition werden thread-sicher vom Audio-Thread an die
  Oberfläche übergeben

---

## 🤖 Entstehung

Dieses Plugin wurde gemeinsam mit **[Claude](https://claude.ai)** von Anthropic entwickelt:
Der gesamte Code (Klangerzeugung, Oberfläche und Build-System) entstand im Dialog mit Claude.
Das Design stammt von **Krystian Shek**.

[![Made with Claude](https://img.shields.io/badge/Code%20written%20with-Claude-D97757?style=flat-square&logo=anthropic&logoColor=white)](https://claude.ai)

---

## ⚠️ Hinweise

- Die eingebettete Sprachaufnahme wurde mit einem KI-Stimmengenerator erzeugt. Prüfe vor einer
  Veröffentlichung oder einem Verkauf des Plugins, ob die Lizenz deines Tarifs die kommerzielle
  Nutzung der Stimme erlaubt – oder tausche die Aufnahme gegen eine eigene aus.
- Das Plugin ist ein Effekt zur kreativen Klanggestaltung. Beginne mit niedrigen Pegeln, denn
  Knackser und Funkstimmen können kurzzeitig laut werden.
- Eine Lizenz für den Quellcode ist noch festzulegen.

---

<div align="center">

**RadioStatic** · Designed by Krystian Shek · Code written with Claude

*„Between the stations, that's where the magic is.“*

</div>
