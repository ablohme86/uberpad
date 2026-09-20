# UberPad

**UberPad** er en moderne, lynrask og funksjonsrik Notepad++ ekvivalent applikasjon skrevet i **C++20** og **Qt6**, med **dobbel frontend** (både et komplett grafisk grensesnitt og en dedikert **Terminal UI (TUI)**-modus via ncurses), samt full **syntax highlighting med auto-deteksjon av over 460 programmeringsspråk** levert av KDEs KF6SyntaxHighlighting.

---

## Egenskaper

### 1. Notepad++ ekvivalent Editor (GUI)
- **Fanebasert grensesnitt (Multi-Document Tabs):**
  - Åpne ubegrenset antall filer i faner.
  - Endringsindikator (`*`) for filer med ulagrede endringer.
  - Høyreklikk-kontekstmeny på faner: Lukk fane, Lukk andre faner, Lukk faner til høyre, Lukk alle, Kopier filsti, Åpne inneholdende mappe.
  - Dra-og-slipp (Drag & Drop) av filer rett inn i editoren.
- **Avansert kodeeditor:**
  - Linjenummer-marg med markering av aktiv linje og klikk-for-å-velge linje.
  - Utheving av gjeldende linje (Current Line Highlight).
  - Parentes- og klammematching (`()`, `{}`, `[]`).
  - Smart automatisk innrykk (Auto-indentation) ved linjeskift (`{`, `:` osv.).
  - Tab / Shift+Tab for innrykk og utrykk av markerte blokker (støtter mellomrom eller ekte tabulator).
  - Dupliser linje eller utvalg med **Ctrl+D** (kjent Notepad++ snarvei).
  - Flytt linje opp/ned med **Alt+Up** / **Alt+Down**.
  - Slå av/på kommentarer med **Ctrl+/** (detekterer automatisk kommentarstil som `//`, `#`, `--`).
  - Linjeskiftkonvertering: Deteksjon og konvertering mellom Unix (`LF`) og Windows (`CRLF`).
  - Zoom inn/ut med **Ctrl++**, **Ctrl+-**, **Ctrl+0** eller **Ctrl + Musehjul**.
  - Word Wrap av/på (**Ctrl+Alt+W**).
- **Søk og Erstatt (Notepad++ stil):**
  - Dokk/panel nederst i vinduet for søk og erstatting.
  - Søk forover (**Enter** / **Find Next**) og bakover (**Find Prev**).
  - Erstatt gjeldende og **Replace All** med statistikk.
  - Match Case (skill store/små bokstaver), Whole Word (hele ord) og Regular Expressions (Regex).
  - Sanntids teller: F.eks. `"3 of 15 matches"` eller `"No match"`.
  - Utheving av alle treff i hele dokumentet.
- **Hurtighopp til linje (Ctrl+G):**
  - Rask dialog for å hoppe direkte til linjenummer.
- **Arbeidsområde og filtre (Folder as Workspace):**
  - Sidemeny (venstre dokk) for å utforske prosjektmapper.
  - Åpne filer med dobbeltklikk.
  - Kontekstmeny for å opprette nye filer/mapper, slette, omdøpe eller vise i filbehandler.
- **Detaljert statuslinje:**
  - Posisjon: `Ln : X   Col : Y   Sel : Z`
  - Statistikk: `Lines : N   Length : M`
  - Linjeskift: `Unix (LF)` / `Windows (CRLF)` (klikkbar for direkte konvertering).
  - Koding: `UTF-8`.
  - Språk: Viser aktivt språk (klikkbar for hurtigmeny over alle 460+ språk).
  - Modus: `INS` / `OVR`.
- **Temaer:**
  - Innebygde mørke og lyse temaer: Dracula, Breeze Dark, Monokai, Nord, Catppuccin Mocha, GitHub Dark, Solarized Dark/Light, Breeze Light m.fl.

---

### 2. Full Terminal UI (TUI) Støtte
UberPad har tosidig terminalstøtte:

1. **Integrert interaktiv PTY-terminal i GUI:**
   - Egen dokkbar terminal nederst i editoren (**F12** eller **Ctrl+`**).
   - Kjører et ekte pseudo-terminal (PTY) underskall (`$SHELL`, `/bin/bash` eller `/bin/zsh`).
   - Full ANSI/VT100 fargestøtte (16 standard farger, 256 farger og 24-bit Truecolor).
   - Håndterer dynamisk endring av størrelse (`TIOCSWINSZ` / `SIGWINCH`).
   - Hurtigknapp for å skifte mappe direkte til den aktive filens mappe (`cd "<dir>"`).
2. **Dedikert frittstående TUI-modus (`uberpad --tui [filer...]`):**
   - Kjører direkte i terminalen (via `ncursesw`) uten behov for grafisk visning (Wayland/X11).
   - Designet for å matche Qt GUI-layouten mest mulig:
     - **Topp-menylinje (F10):** Interaktive rullegardinsmenyer (*File, Edit, Search, View, Language, Help*).
     - **Dokument-fanelinje (Row 1):** Viser åpne dokumenter (`[ 1: main.cpp * ] [ 2: CMakeLists.txt ]`), veksle med `F7`/`F8` eller `Ctrl+B`/`Ctrl+T`.
     - **Filutforsker-sidemeny (F9):** Venstre sidepanel som viser mapper og filer i prosjektet (*Folder as Workspace*). Naviger med piltastene, trykk `Enter` for å åpne fil eller utvide/lukke mappe.
     - **Fokus-veksling (Tab):** `Tab`-tasten bytter aktivt fokus direkte mellom filutforsker-sidemenyen og editoren.
     - **Linjenummer-kolonne:** Viser linjenumre med fargeindikasjon for aktiv linje.
     - **Full syntaksfarging i terminalen** via samme KDE KF6-motor (460+ språk).
     - **Statuslinje:** Matcher GUI-statuslinjen (`Ln X, Col Y │ Lines: N, Length: M │ Unix (LF) │ UTF-8 │ C++ │ INS`).
     - **Snarveier:** `Ctrl+N` (ny), `Ctrl+O` (åpne), `Ctrl+S` (lagre), `Ctrl+W` (lukk fane), `Ctrl+F` (søk), `Ctrl+G` (hopp til linje), `Ctrl+D` (dupliser linje), `Ctrl+/` (kommenter), `Ctrl+Q` (avslutt).

---

### 3. Syntaksfarging & Auto-deteksjon (460+ språk)
UberPad benytter **KDE KF6SyntaxHighlighting**, som gir dekning for mer enn **460 programmerings-, skript- og markeringsspråk**:
- **System og applikasjoner:** C, C++, Rust, Go, Zig, D, Fortran, Ada, C#, Java, Kotlin, Swift, Objective-C.
- **Skript og dynamiske språk:** Python, JavaScript, TypeScript, Ruby, PHP, Perl, Lua, R, Julia, Dart, Tcl.
- **Skall og terminal:** Bash, Zsh, Fish, POSIX Shell, PowerShell, Batch.
- **Web og data:** HTML, CSS, SCSS, XML, JSON, YAML, TOML, INI, SQL, GraphQL, Markdown, LaTeX.
- **Bygg og DevOps:** CMake, Makefile, Dockerfile, Meson, QMake, Nix, Terraform, Kubernetes.
- **Auto-deteksjon:** Kjenner automatisk igjen filtype basert på filendelse, filnavn, MIME-type eller innhold.
- **Manuell overstyring:** Språkmenyen har kategoriserte undergrupper (A-C, D-H, I-P, Q-Z) for direkte valg.

---

## Bygging og Installasjon

### Forutsetninger (Arch / CachyOS / Fedora / Ubuntu)
- C++20-kompatibel kompilator (`g++` eller `clang++`)
- `cmake` (>= 3.20) og `ninja`
- `qt6-base` (Qt6 Widgets, Gui, Core)
- `syntax-highlighting` (KDE KF6SyntaxHighlighting)
- `ncurses`

### Kompiler
```bash
cd /home/alexander/Prosjekter/uberpad
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

Den kjørbare filen vil ligge i `build/uberpad`.

---

## Bruk

### 1. Start i Grafisk modus (GUI)
```bash
./build/uberpad [fil1 fil2 ...]
```

### 2. Start i Terminal UI-modus (TUI)
```bash
./build/uberpad --tui [fil1 fil2 ...]
```
*(Eller bare `./build/uberpad` når man er på en maskin uten grafisk visning).*

### 3. Kommandolinjeflagg
```text
UberPad - Modern Notepad++ equivalent in C++/Qt6 with TUI support

Usage: uberpad [options] [files...]

Options:
  -t, --tui      Run in Terminal User Interface (TUI) mode
  -g, --gui      Force Graphical User Interface (GUI) mode
  -v, --version  Show version information
  -h, --help     Show this help message
```

---

## Tastatursnarveier

| Hurtigtast | Funksjon |
|---|---|
| **Ctrl + N** | Nytt dokument |
| **Ctrl + O** | Åpne fil |
| **Ctrl + K** | Åpne mappe som arbeidsområde (Workspace) |
| **Ctrl + S** | Lagre fil |
| **Ctrl + Shift + S** | Lagre som... |
| **Ctrl + Shift + Alt + S** | Lagre alle filer |
| **Ctrl + W** | Lukk aktiv fane |
| **Ctrl + Shift + W** | Lukk alle faner |
| **Ctrl + F** | Åpne Søk-panelet |
| **Ctrl + H** | Åpne Erstatt-panelet |
| **Ctrl + G** | Gå til linje... |
| **Ctrl + D** | Dupliser linje eller markering |
| **Ctrl + /** | Slå av/på kommentar for linje |
| **Alt + Up / Down** | Flytt linje opp / ned |
| **Tab / Shift+Tab** | Innrykk / utrykk av markert blokk |
| **Ctrl + Alt + W** | Slå av/på linjebryting (Word Wrap) |
| **Ctrl + Plus / Minus** | Zoom inn / ut |
| **Ctrl + 0** | Tilbakestill zoom |
| **F12** / **Ctrl + `** | Slå av/på integrert terminal |
| **Ctrl + Q** | Avslutt programmet |
# uberpad
