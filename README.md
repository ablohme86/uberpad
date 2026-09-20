# UberPad

**UberPad** er en moderne, lynrask og funksjonsrik Notepad++ ekvivalent applikasjon skrevet i **C++20** og **Qt6**, med **dobbel frontend** (både et komplett grafisk grensesnitt og en dedikert **Terminal UI (TUI)**-modus via ncurses), full **syntax highlighting med auto-deteksjon av over 460 programmeringsspråk** levert av KDEs KF6SyntaxHighlighting, samt innebygd **SFTP / FTP / FTPS Remote Workspace** for direkte redigering mot eksterne servere (tilsvarende NppFTP).

---

## Egenskaper

### 1. Notepad++ ekvivalent Editor (GUI)
- **Fanebasert grensesnitt (Multi-Document Tabs):**
  - Åpne ubegrenset antall filer i faner med ikon og server/fil-tooltips.
  - Endringsindikator (`*`) for filer med ulagrede endringer.
  - Høyreklikk-kontekstmeny på faner: Lukk fane, Lukk andre faner, Lukk faner til høyre, Lukk alle, Kopier filsti, Kopier Remote URL, Åpne inneholdende mappe.
  - Dra-og-slipp (Drag & Drop) av filer rett inn i editoren.
- **Avansert kodeeditor:**
  - Linjenummer-marg med markering av aktiv linje og klikk-for-å-velge linje.
  - Utheving av gjeldende linje (Current Line Highlight).
  - Parentes- og klammematching (`()`, `{}`, `[]`).
  - Smart automatisk innrykk (Auto-indentation) ved linjeskift (`{`, `:` osv.).
  - Tab / Shift+Tab for innrykk og utrykk av markerte blokker.
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
- **Lokalt arbeidsområde (Folder as Workspace):**
  - Sidemeny (venstre dokk) for å utforske lokale prosjektmapper.
  - Åpne filer med dobbeltklikk.
  - Kontekstmeny for å opprette nye filer/mapper, slette, omdøpe eller vise i filbehandler.
- **Remote Workspace over SFTP / FTP / FTPS (NppFTP-stil):**
  - Integrert Remote Workspace dokk i sidepanelet (**Ctrl+Shift+R**).
  - Støtter SSH File Transfer Protocol (**SFTP**), standard **FTP** og **FTPS** (FTP over TLS/SSL) via libcurl.
  - Utforsk eksterne serverkataloger med dynamisk/lazy loading.
  - Dobbeltklikk på ekstern fil for å laste ned og åpne direkte i tabbed editor som `[SFTP] filnavn`.
  - Direkte lagring tilbake til fjernserver ved **Ctrl+S** eller **Save All**.
  - Verktøylinje og kontekstmeny: Opprett ny fil/mappe på server, slett fil/mappe, last opp lokal fil direkte til ekstern mappe, kopier remote path / URL.
- **Detaljert statuslinje:**
  - Posisjon: `Ln : X   Col : Y   Sel : Z`
  - Statistikk: `Lines : N   Length : M`
  - Linjeskift: `Unix (LF)` / `Windows (CRLF)` (klikkbar for direkte konvertering).
  - Koding: `UTF-8`.
  - Språk: Viser aktivt språk (klikkbar for hurtigmeny over alle 460+ språk).
  - Modus: `INS` / `OVR`.
- **Temaer & Ikon:**
  - Tilpasset neon origami-kameleon app-ikon i flere oppløsninger (256x256, 64x64, 32x32) innebygd i Qt-ressurser og `.desktop`-fil.
  - Innebygde mørke og lyse temaer: Dracula, Breeze Dark, Monokai, Nord, Catppuccin Mocha, GitHub Dark, Solarized Dark/Light, Breeze Light m.fl.

---

### 2. Full Terminal UI (TUI) Støtte
UberPad har tosidig terminalstøtte:

1. **Integrert interaktiv PTY-terminal i GUI:**
   - Egen dokkbar terminal nederst i editoren (**F12** eller **Ctrl+`**).
   - Kjører et ekte pseudo-terminal (PTY) underskall (`$SHELL`, `/bin/bash` eller `/bin/zsh`).
   - Full ANSI/VT100 fargestøtte (16 standard farger, 256 farger og 24-bit Truecolor).
   - Håndterer dynamisk endring av størrelse (`TIOCSWINSZ` / `SIGWINCH`).
2. **Dedikert frittstående TUI-modus (`uberpad --tui [filer...]`):**
   - Kjører direkte i terminalen (via `ncursesw`) uten behov for grafisk visning (Wayland/X11).
   - **DOS `edit.com` / Windows-stil menynavigasjon:**
     - Direkte hurtigåpning av menyer med **Alt+F** (File), **Alt+E** (Edit), **Alt+S** (Search), **Alt+V** (View), **Alt+L** (Language), **Alt+H** (Help).
     - Hurtigtast-bokstaver i menyene: Trykk f.eks. `x` for Exit, `n` for New, `o` for Open, `s` for Save.
     - Naviger mellom menyer med venstre/høyre piltaster og elementer med opp/ned, `Enter` for å velge, `Esc` for å lukke.
   - **Dokument-fanelinje (Row 1):** Viser åpne dokumenter (`[ 1: main.cpp * ] [ 2: CMakeLists.txt ]`), veksle med `F7`/`F8` eller `Ctrl+B`/`Ctrl+T`.
   - **Filutforsker-sidemeny (F9):** Venstre sidepanel som viser mapper og filer i prosjektet (*Folder as Workspace*). Naviger med piltastene, trykk `Enter` for å åpne fil eller utvide/lukke mappe.
   - **Fokus-veksling (Tab):** `Tab`-tasten bytter aktivt fokus direkte mellom filutforsker-sidemenyen og editoren.
   - **Linjenummer-kolonne:** Viser linjenumre med fargeindikasjon for aktiv linje.
   - **Full syntaksfarging i terminalen** via samme KDE KF6-motor (460+ språk).
   - **Statuslinje:** Matcher GUI-statuslinjen (`Ln X, Col Y │ Lines: N, Length: M │ Unix (LF) │ UTF-8 │ C++ │ INS`).

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
- `cmake` (>= 3.20) og `ninja` eller `make`
- `qt6-base` (Qt6 Widgets, Gui, Core)
- `syntax-highlighting` (KDE KF6SyntaxHighlighting)
- `ncurses`
- `curl` (med SFTP/libssh2-støtte)

### Bygging med Makefile
```bash
make            # Bygger optimalisert release-versjon og lager symlink ./uberpad
make run        # Bygger og starter i GUI-modus
make tui        # Bygger og starter i TUI-modus
make debug      # Bygger med debug-symboler
make clean      # Fjerner kompileringsfiler
make help       # Viser alle tilgjengelige mål
```

### Installasjon

Begge målene installerer binærfila, `.desktop`-oppføringa og hele ikonsettet,
og oppdaterer deretter meny- og ikoncachene slik at UberPad dukker opp i
programmenyen til KDE, GNOME, XFCE, Cinnamon m.fl. uten utlogging.

```bash
make install-user      # Kun for innlogget bruker -> ~/.local (ingen root)
sudo make install      # Systemomfattende -> /usr/local

make uninstall-user    # Fjerner brukerinstallasjonen
sudo make uninstall    # Fjerner systeminstallasjonen
```

Velg et annet mål med `PREFIX`, f.eks. `make install PREFIX=/opt/uberpad`.
`DESTDIR` støttes for pakkebygging (cache-oppdateringen hoppes da over).

Merk: ved `make install-user` skrives den absolutte stien inn i `.desktop`-fila,
så menyoppføringa virker uansett om `~/.local/bin` ligger i `PATH`. For å kunne
kjøre `uberpad` fra skallet legger du til stien – for fish:
```fish
fish_add_path ~/.local/bin
```

### Alternativ bygging med CMake
```bash
cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build
cmake --install build --prefix ~/.local
```

### Programikonet

Ikonet er en vektortegnet kameleon og genereres fra
`resources/icons/generate-icons.py`, som skriver `uberpad.svg` og PNG-settet
(16–512 px). Kjør `make icons` etter endringer i skriptet.
`--dark`-flagget gir en mørk variant med fargeovergang i selve kameleonen.

---

## Bruk

### 1. Start i Grafisk modus (GUI)
```bash
./uberpad [fil1 fil2 ...]
```

### 2. Start i Terminal UI-modus (TUI)
```bash
./uberpad --tui [fil1 fil2 ...]
```
*(Eller bare `./uberpad` når man er på en SSH-økt eller terminal uten grafisk visning).*

---

## Tastatursnarveier

| Hurtigtast | Funksjon |
|---|---|
| **Ctrl + N** | Nytt dokument |
| **Ctrl + O** | Åpne fil |
| **Ctrl + K** | Åpne lokal mappe som arbeidsområde (Workspace) |
| **Ctrl + Shift + R** | Koble til Remote Workspace (SFTP / FTP) |
| **Ctrl + S** | Lagre fil (lokalt eller direkte over SFTP) |
| **Ctrl + Shift + S** | Lagre som... |
| **Ctrl + Shift + Alt + S** | Lagre alle åpne filer |
| **Ctrl + W** | Lukk aktiv fane |
| **Ctrl + Shift + W** | Lukk alle faner |
| **Ctrl + F** | Åpne Søk-panelet |
| **Ctrl + H** | Åpne Erstatt-panelet |
| **Ctrl + G** | Gå til linje... |
| **Ctrl + D** | Dupliser linje eller markering |
| **Ctrl + /** | Slå av/på kommentar for linje |
| **Alt + Up / Down** | Flytt linje opp / ned |
| **Tab / Shift+Tab** | Innrykk / utrykk av markert blokk (eller bytt fokus i TUI) |
| **Ctrl + Alt + W** | Slå av/på linjebryting (Word Wrap) |
| **Ctrl + Plus / Minus** | Zoom inn / ut |
| **Ctrl + 0** | Tilbakestill zoom |
| **F12** / **Ctrl + `** | Slå av/på integrert terminal i GUI |
| **Alt + F / E / S / V / L / H** | DOS-stil menystyring i TUI |
| **F9** | Slå av/på filutforsker i TUI |
| **F7 / F8** | Forrige / neste fane i TUI |
| **Ctrl + Q** | Avslutt programmet |
