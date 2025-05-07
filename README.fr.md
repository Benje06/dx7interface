## 🌐 Autres Languages
[English](README.md)

- [Introduction](#introduction)
- [Description](#about)
- [HOWTO](#howto)
    - [Windows](#windows)
    - [Linux](#linux)
    - [Sources](#build-it-from-sources)
- [Historique](#historique)
- [Remerciements](#remerciements)
- [Réferences](#references)
- [Licences](#licences)
- [Auteurs et Contributeurs](#auteurs-et-contributeurs)

# Introduction
**Dx7interface** est une **interface graphique** complète pour **piloter** les synthetiseurs physiques Yamaha **Dx7**\
ainsi que ses derivés tel les **Tx816** et **Tx216**, et aussi les emulateurs comme [Hexter](https://github.com/theabolton/hexter) ou [Dexed](https://asb2m10.github.io/dexed/) du moment qu'ils acceptent les **messages SysEx**.\
Elle permet aussi de gerer les **banques de sons**.

<ins>Dx7interface et un Tx216 sous Manjaro par Jean-Michel</ins>:
<table>
  <tr>
    <td align="center"><b>Maiden Voyage (avec un TX216 Yamaha)</b></td>
    <td align="center"><b>Test de Dx7interface sur un TX216</b></td>
  </tr>
  <tr>
    <td align="center">
    <a href="https://youtu.be/x17MPC53TIk">
        <img src="https://img.youtube.com/vi/x17MPC53TIk/maxresdefault.jpg" width="400">
    </a>
    </td>
    <td align="center">
      <a href="https://youtu.be/rE62dQA1REk">
        <img src="https://img.youtube.com/vi/rE62dQA1REk/maxresdefault.jpg" width="400">
      </a>
    </td>
  </tr>
</table>

<b>Juste pour le plaisir Jean-Michel sur un Dx7 ^^</b>\
<a href="https://youtu.be/IQYbie4J-yw">
  <img src="https://img.youtube.com/vi/IQYbie4J-yw/maxresdefault.jpg" width="400">
</a>

# Description
Dx7interface est une **interface graphique** permettant d'éditer des banques de sons et de piloter les synthétiseurs physiques **Dx7** / **Tx216** / **Tx816** ainsi que leurs **émulateurs**.\
Elle utilise des **messages SysEx** et est basée sur le séquenceur **Alsa** (pour **Linux**) et **RtMidi** + loopMIDI (pour **Windows**).\
L'interface est entièrement pilotable par **Control Change** (CC) et **Program Change** (PC).\
Elle dispose d'une fonction **MIDI Learn** permettant le **chargement** et la **sauvegarde** de la configuration depuis et vers un fichier.\
Les **courbes** ADSR et Pitch sont modifiables à la **souris**.\
Vous pouvez le **demarrer** avec une **couleur** spécifique pour l'**identifier** rapidement.

Elle prend en charge :
- **Les paramètres des contrôleurs** :
    - par son, en **mode TF1** (Tx816/216)
    - par banque, en **mode natif** du Dx7.
- **1**, **32** et jusqu'à **128** sons par banque.
- L'**edition les banques de sons** via les fonctions de menu (Insérer/Remplacer/Supprimer).
- La **lecture** et la **sauvegarde** de son ou des banques depuis et vers les fichiers **Raw** (sans en-têtes SysEx) ou **SysEx**,\
aux formats **Bulk 1** et **Bulk 32**.
- La **comparaison**, la **réstoration** d'un son ou d'une banque et **l'envoi** d'une banque.\
L'envoi du son se fait lorsque vous sélectionnez un son.
- Des **Canaux indépendants** pour l'**Entrée** et la **Sortie** MIDI.
- Un **indicateur** dans la liste des pour les **sons modifiés**.
- Une fonction **panique Midi**.
- La **journalisation** dans un **fichier** et dans la **console**.

Elle est ecrite en **C++** et utilise **GLIB** (glibmm-2.68) et **GTK 4.0** (gtkmm-4.0).\
Avec **Cairo** pour le dessin (cairomm 1.16) et **Pango** pour la prise en charge des polices (pangomm 2.68).\
Elle est **personnalisable** à condition de **préserver** le **type d'objet** et son **nom**, car elle est basée sur :
- un **XML** pour l'**interface utilisateur**
- un **CSS** pour le **thème**
- et la prise en charge du chargement de **polices personnalisées**

Étant donné que **Dx7interface** est un **plugin/module** (Glib::Gmodule), elle s'appuie sur **GxInterface** comme **chargeur de modules**.\
Il s'agit d'une **dérivée** de :
- **GxModule**, venant de Gxinterface
- **Synth**, qui contient toutes les fonctions Midi

**GxInterface/Gxmodule** fournit ses **fonctions de base** telles que :
- **Créer/Charger** des modules
- **Présenter** des boîtes de dialogue/fenêtres pour le chargement/l'enregistrement/les messages
- **Lecture/Écriture** par octets via DataStream
- **Obtentions** des objets graphique de l'interface utilisateur
- **Définir le style** (police/thème)
- **Options de ligne de commande**
- La gestion des **journaux**
- **Threads**

**Dx7interface** fournit :
- la définition du format sysex pour le Dx7 et Tx816/Tx216
- la définition des messages
- la définition des structures de lecture, d'analyse et d'écriture
- l'analyse elle-même
- le lien entre l'interface utilisateur et les fonctions utilisant les signaux et les événements
- la gestion des banques de sons
- les fonctions de dessin (doit être déplacée pour être réutilisable)
- la fonction d'apprentissage MIDI (doit être déplacée pour être réutilisable)

# HOWTO
## Windows
La version Windows n'implémente pas de pilote Midi, elle s'appuie pour cela sur [loopMidi](https://www.tobias-erichsen.de/software/loopmidi.html) de Tobias Erichsen.\
Une fois loopMidi installé, démarrez-le et créez **deux ports** nommés **Dx7interface_in** et **Dx7interface_out**.\
Au démarrage, Dx7interface se connectera automatiquement à ces ports.\
Vous devrez définir la taille maximale du sysex à au moins 4 096 kilo-octets dans la page avancée, ainsi les banques de 32 sons pouront être envoyées.\
Une fois démarré, tout logiciel prenant en charge Direct Music le verra.

Téelecargement de la version Windows:
- Installateur [Dx7interface sans console](https://github.com/Benje06/dx7interface/blob/gtk4/Dx7interface.exe?raw=true)

- Installateur [Dx7interface avec console](https://github.com/Benje06/dx7interface/blob/gtk4/Dx7interface_debug_console.exe?raw=true)

![dx7interface avec console de debug](https://github.com/user-attachments/assets/78956483-d185-4516-9482-534118454dd0)

## Linux
**Distribution supportées**:\
Il devrait fonctionner sur n'importe quelle distribution du moment que les versions spécifiées sont respectées.\
Les distributions testées sont :
- Arch Like: **Archlinux** / **Manjaro** avec les PKGBUILD pour
[Gxinterface](https://github.com/Benje06/dx7interface/blob/gtk4/gxinterface/PKGBUILD?raw=true) et
[Dx7interface](https://github.com/Benje06/dx7interface/blob/gtk4dx7interface/PKGBUILD?raw=true)
- Debian like: **Debian 12** / **Ubuntu 24.04** en utilisant [Checkinstall](#debian--ubuntu)

**Run**:\
Une fois installé, vous pouvez l'exécuter via le menu ou la démarrer depuis le terminal ainsi:
```sh
$ gxinterface -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so
```

<ins>Les options de démarrage de Gxinterface / Dx7interface sont :</ins>
- **Coloriser le titre de la section**\
Utilisé lorsque vous démarrez plusieurs instances du programme.\
Elle colore la section du titre avec la couleur spécifiée pour l'identifier visuellement.\
<code>-c "color"</code> où color est le code couleur HTML.

```sh
$ gxinterface -c "red" -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so
$ gxinterface -c "#e15a46" -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so
```

- **Log level**
pour spécifier le niveau de journalisation.\
<code>-l [0|1|2]</code>\
<code>0=aucun log, 1=log dans un fichier, 2=log dans un fichier et la console (default)</code>

```sh
gxinterface -l 0 -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so
gxinterface -l 1 -m /usr/share/dx7interface/1.0.0/dx7interface-1.0.0.so
```

- **Charger un élément**\
<code>-m nom_fichier.[so|la|dll]</code> \
<code>-m *.so</code> La méthode habituelle pour charger un module. Vous pouvez également charger le fichier .la\
<code>-m *.dll</code> comme .so pour la version Windows

- **Charger l'interface principale alternative**\
<code>-i *.ui</code> Pour charger un fichier XML comme **interface principale**, le code rattaché est celui de gxinterface.\
Ainsi l'interface utilisateur nécessite au minimum :
    - une fenêtre Gtk::Window nommée "main_window"
    - un bouton Gtk::Button nommé "module_select"
    - un bouton Gtk::CheckButton nommé "checkbutton_standalone"
    - un bloc-notes Gtk::Notebook nommé "notebook_main"

## Construction à partir des sources

Vous aurez besoin: 
- **Commun** de DEV:
    - **base-devel** (Arch: 1-2 , UCRT64: >=2024.11-1) ou **build-essential** >= 12.9 ( gcc make ... )
    - GNU autotool: **autogen** >= 5.18 / **autoconf** >= 2.71 / **automake** >= 1.16
    - **intltool** >= 0.51 et **gettext** >= 0.21 
    - **glib-gettextize** => 2.74:
        - Archlinux: glib2 >= 2.82.4
        - Debian: libglib2.0-dev
        - Windows: ucrt64/mingw-w64-ucrt-x86_64-glib2
    - **libtool** >= 2.4.7
    - **aclocal** >= 1.16
        - Archlinux: inclus dans automake
        - Debian: inclus dans automake
        - Windows: msys/automake-wrapper 20240607-1
    - **m4** >= 1.4.19
    - **gtkmm-4.0** >= 4.8.0 / **glibmm-2.68** >= 2.68     (lib...-dev for Debian like)
    - **cairomm-1.16** >= 1.16 / **pangomm-2.48** >= 2.48  (lib...-dev for Debian like)
    - **libsigc++-3.0** >= 3.4.0  					       (lib...-dev for Debian like)
    - **pthread**:
        - Windows ucrt64: ucrt64/mingw-w64-ucrt-x86_64-winpthreads-git >=  12.0.0.r679
        - Debian: inclus dans glibc (optionnel: libpthread-stubs0-dev >= 0.4.1)
        - Archlinux: glibc >= 2.41
    - **Packager**:
        - Archlinux: **makepkg** git (optionnel: devtools)
        - Manjaro:   **manjaro-tools-base** **manjaro-tools-pkg**
        - Debian/Ubuntu:  **checkinstall** >= 1.6.2
        - Windows: **NSIS** mingw-w64-ucrt-x86_64-nsis >= 3.11.1
- **Spécifique** Dx7interface:
    - **gxinterface** >= 1.0.0
    - **MIDI**:
        - Archlinux: **alsa-lib** >= 1.2.14
        - Debian/Ubuntu: **libasound2-dev** >= 1.2.8
        - Windows: **RtMidi** mingw-w64-ucrt-x86_64-rtmidi >= 6.0.0-3


### Environments de dévellopement</ins>:
- <ins>Kdevelop pour Linux</ins> :\
Les fichiers fournissent des variables d'environnement (globales pour Kdev4) et des lanceurs (spécifique à Kdev4)\
pour les actions courantes, celles commençant par D ou DEBUG sont marquées pour le débogage avec gdb dans Kdevelop, pour les projets gxinterface et dx7interface.\
[kdev4 global]()\
<ins>Vous devez modifier le chemin dans tous les fichiers .kdev4</ins> :
    - "file:///home/jerome/dev/git/gtk4" vers votre répertoire de clonage Git
    - "/home/jerome/dev/build/" vers votre répertoire de compilation makepkg ArchLinux
    - gxinterface : [kdev4 spécifique](https://github.com/Benje06/dx7interface/blob/gtk4/gxinterface/.kdev4/gxinterface.kdev4)
    - dx7interface: [kdev4 spécifique](https://github.com/Benje06/dx7interface/blob/gtk4/dx7interface/.kdev4/dx7interface.kdev4)

- <ins>VS Code pour Windows compilé sous ucrt64</ins> :\
Pour compiler sous Windows, vous avez besoin de [MSYS2/UCRT64](https://www.msys2.org/]).\
Vous pouvez obtenir [l'installateur](https://www.msys2.org/docs/installer/) et la [documentation](https://www.msys2.org/docs/what-is-msys2/]) pour MSYS2/UCRT64.\
J'ai dressé la liste des paquets installés sur UCRT64 pour compiler le paquet Windows.\
Cette [liste des paquets UCRT64](https://github.com/Benje06/dx7interface/blob/gtk4/ucrt64_pkg_list.txt) contient plus de données que nécessaire pour compiler le paquet. Veuillez la prendre à titre informatif.\
Après avoir cloné le dépôt, dans les fichiers du [répertoir .vscode](https://github.com/Benje06/dx7interface/blob/gtk4/.vscode), changez le chemin (D:\\crosscompile\\msys2) vers votre répertoire d'installation msys2 et chargez le répertoire racine git clone dans vscode.

### <ins>A la main</ins>
**Il est recommandé d'utiliser makepkg ou checkinstall**\
Il existe aussi un [**PKGBUILD-gx**](https://github.com/Benje06/dx7interface/blob/gtk4/gxinterface/PKGBUILD-gx) et un [**PKGBUILD-dx**](https://github.com/Benje06/dx7interface/blob/gtk4/dx7interface/PKGBUILD-dx) qui utilisent votre répertoire Git local pour la compilation.

#### Construction
```sh
$ cd to_git_clone_directory/[gxinterface|dx7interface]/
```
##### REMISE A ZERO DES SOURCES
```sh
$ make clean || true
$ make disclean || true
```
##### AUTOGEN
- for gxinterface:
```sh
$ ./autogen.sh --prefix=/usr --enable-log=2 --enable-console=0 --enable-maintainer-mode
```
- for dx7interface:
```sh
$ ./autogen.sh --prefix=/usr --enable-log=2 --enable-console=0 --enable-maintainer-mode --enable-alsa
```
"--enable-console=1" active pour windows, le démarrage de l'application avec une console. Non utilisé sous Linux.\
"--enable-log=2" définit les détails du journal des erreurs : 0 = aucun détail, 1 = afficher le nom de la fonction, 2 = nom de la fonction + ligne + fichier. "--enable-log" diffère de la commande de démarrage -l car il ne contient que les détails du niveau de journalisation, et non les messages communs à consigner leurs emplacement.\
"--enable-alsa ou --enable-rtmidi" utilisez alsa sous Linux et rtmidi sous Windows. Inutile de spécifier --disable-alsa, ne passez pas l'indicateur, omettez-le simplement.\
"--[enable|disable]-maintainer-mode" est une macro [mode mainteneur Automake](https://www.gnu.org/software/automake/manual/html_node/maintainer_002dmode.html) qui permet la reconstruction de certains fichiers, comme le script configure.

##### MAKE
```sh
$ make -j3
```
ou bien remplacez 3 par le nombre de CPU moins 1 ou sous Linux <code>$(($(cat /proc/cpuinfo | grep -c ^processor) - 1))</code>
#### INSTALLATION
##### <ins>Par packager</ins>:
###### - **Archlinux** / **Manjaro**:
- <ins>pour gxinterface</ins>:
```sh
$ cd to_build_directory/
$ cp from_git_directory/gxinterface/PKGBUILD PKGBUILD-GX
$ makepkg -sfip PKGBUILD-GX
```
- <ins>pour dx7interface:</ins>
```sh
$ cd to_build_directory/
$ cp from_git_directory/dx7interface/PKGBUILD PKGBUILD-DX
$ makepkg -sfip PKGBUILD-DX
```
###### - <ins>**Debian** / **Ubuntu**:</ins>
- <ins>pour gxinterface</ins>:
```sh
$ cd to_git_clone_directory/gxinterface/
$ sudo checkinstall -D --fstrans=no --install=yes --pkgversion="1.0.0" -y
```
- <ins>pour Dx7interface</ins>:
```sh
$ cd cd to_git_clone_directory/dx7interface/
$ sudo checkinstall -D --fstrans=no --install=yes --pkgversion="1.0.0" -y
```

##### <ins>Par Make install</ins>:
vous pouvez exécuter à la fois gxinterface et dx7interface sans avoir besoin de les installer, mais dx7interface a besoin des bibliothèques et des en-têtes de gxinterface pour être construit.
```sh
$ make install
```

#### EXÉCUTER à partir des sources
- **Gxinterface** seul:
```sh
$ cd to_git_directory/gxinterface/src/
$ ./gxinterface
```
- **Dx7interface**:
```sh
$ cd to_git_directory/dx7interface/src/
$ ../../gxinterface/src/gxinterface -l 2 -m src/.libs/dx7interface-0.0.1.so
```
or if gxinterface is installed
```sh
$ gxinterface -l 2 -m src/.libs/dx7interface-0.0.1.so
```

# History
Ce projet a débuté en 2006 pour créer une interface de configuration pour Linux.\
En commencant par le réseau, il s'appelait [GNetAdm](https://sourceforge.net/projects/gnetadm/) et a été développé en C avec Glade et GTK2.\
L'objectif premier était de charger des modules avec une interface personnalisable pour chaque type de « service » afin de les configurer.\
Utiliser le C avec le multithreading, les modules et l'interface, tout cela de manière dynamique, était fastidieux, alors je l'ai migré vers C++.\
Ce noyau a donné naissance à Gxinterface, le chargeur de modules.\

Au même moment, un ami musicien, Jean-Michel, utilisateur d'un synthétiseur Dx7, cherchait à éditer simplement les banque de sons.\
Nous avons donc eu l'idée de créer une interface pour ce synthétiseur avec gnetadm. C'est ainsi que Dx7interface est né.\
Nous l'avons réalisé en C++ avec GTK2 et Glade. Le projet s'est terminé en 2011 avec une version fonctionnelle et une version non fonctionnelle pour le mode autonome (démarrage de dx7interface séparé de gxinterface). Il était hébergé sur [Savannah](https://savannah.nongnu.org/bzr/?group=gxinterface) avec bzr et permettait des tâches basiques comme le pilotage du synthétiseur via l'interface utilisateur.

[Hexter](https://github.com/theabolton/hexter) est l'émulateur de référence pour ce synthétiseur, mais l'éditeur intégré était en mode rétro. Nous avons donc décidé, comme il acceptait SysEx pour le piloter et qu'il pouvait écrire des fichiers, que cela nous suffirait pour obtenir quelque chose de fonctionnel.\
Début 2025, j'ai rouvert le code et essayé de l'exécuter, mais GTK2 et tout le reste étaient tellement obsolètes que j'ai décidé de le mettre à jour vers GTK3, dans un premier temps pour voir à quel point cela pouvait être douloureux, en vue de GTK4.

Étonnamment, ce n'était pas le cas, ce qui m'a fait penser que le cœur et le concept étaient suffisamment robuste.\
Après avoir migré vers gtk3 ([dernier commit bzr](https://bzr.savannah.gnu.org/lh/gxinterface/changes/105?start_revid=105)), nous avons migré le dépôt vers la version Git et ajouté une nouvelle [branche gtk4](https://git.savannah.nongnu.org/cgit/gxinterface.git).\

Nous avons ajouté des fonctionnalités étape par étape, comme le dessin, la modification à la souris, l'enregistrement de fichiers, l'apprentissage MIDI, la comparaison et le menu d'édition. Parallèlement, nous corrigeons les bugs et mettons à jour le coeur. Nous y avons déplacé les fonctions génériques, placé la partie MIDI dans une classe distincte, en essayant de créer des classes plus spécialisées, et dissocié l'interface utilisateur des fonctionnalités autant que possible.

Et enfin, nous avons déplacé tout le code ici. ^^

Nous espérons que vous apprécierez ce logiciel et ses fonctionnalités, pour piloter ce synthétiseur mythique des années 80.

# Remerciement
Un grand merci à:
- [Thea Bolton](https://github.com/theabolton) pour avoir fait l'incroyable [Hexter](https://github.com/theabolton/hexter).
- [Dave Benson](https://homepages.abdn.ac.uk/d.j.benson/pages/index.html) pour ses [pages de référence du synthétiseur physique](https://homepages.abdn.ac.uk/d.j.benson/pages/html/dx7.html)
- [Tim Conrardy](http://tim-conrardy.last-memories.com/) pour sa page Atari Dx7 [tim's atari world](https://web.archive.org/web/20160308015913/http://tamw.atari-users.net/dx7.htm)

# References
- Présentation\
[Yamaha design Dx7](https://www.yamaha.com/en/tech-design/design/synapses/id_009)

- Manuels\
[Yamaha Dx7 manuels (fr)](https://fr.yamaha.com/fr/support/manuals/index.html?l=fr&c=&k=dx7)\
[Yamaha Dx7 manuals (en)](https://uk.yamaha.com/en/support/manuals/index.html?l=en&c=&k=dx7)\
[Yamaha Tx816 Tx216 manuels (fr)](https://fr.yamaha.com/fr/products/contents/music_production/downloads/manuals/index.html?l=en&c=music_production&k=Tx816)\
[Yamaha Tx816 Tx216 manuels (en)](https://usa.yamaha.com/products/contents/music_production/downloads/manuals/index.html?l=en&c=music_production&k=tx816)
- books\
The complete Dx7 by Howard Massey.
- historic\
[Yamaha Chapitre 2 la synthese fm (fr)](https://fr.yamaha.com/fr/products/contents/music_production/synth_40th/history/chapter02/index.html)

# Licenses
## Gxinterface and Dx7interface
Copyright (C) 2010-2025 sous [GPL-v3](https://www.gnu.org/licenses/gpl-3.0.html)\
A l'exception de la police et des images, voir ci-dessous.
## Images and Font
- Toutes les images (.png, .ico .svg) sont sous [Creative Common CC-By-SA](http://creativecommons.org/licenses/by-sa/3.0/)
    - Par Jean-Michel Thiémonge et Jérome Benhaïm:
        - "icon_dx7interface.svg","dx7interface.png", "dx7interface.ico"
        - "32_algo_dx732_algo_dx7_150*105px.svg", "algo[1-32].png"
        - "lfo_wave_142*48px.svg", "SAW+.png", "SAW-.png", "SIN.png", "SQU.png", "S_HOLD.png", "TRI.png"
        - "PianoKeyboard.svg", "keyboard.png", "keyboard_background.png", touche_b.png", "touche_w.png"
- Police:
    - "Araster-fonts-6x8.ttf" est sous [Creative Common CC-By-SA](http://creativecommons.org/licenses/by-sa/3.0/)
        - Par "DOS" voir la [license](https://github.com/Benje06/dx7interface/blob/gtk4/dx7interface/data/fonts/license.txt)
# Autheurs and Contributeurs
## Autheurs:
[THIÉMONGE Jean-Michel](https://github.com/jmechmech)\
[BENHAÏM Jérome](https://github.com/Benje06)

## Contributeurs:
ENNAIME Mirsal merci pour l'aide au démarrage et le truc debug.h \
Termitor, merci pour le calcul mathématique ^^
