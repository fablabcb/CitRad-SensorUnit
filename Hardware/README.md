# Bauanleitung Akkuvariante

## Teile

- Elektronik
  - [Teensy 4.0](https://www.pjrc.com/store/teensy40.html)
  - [Teensy Audio Board](https://www.pjrc.com/store/teensy3_audio.html)
  - [Innosent IPS-354 Radarmodul](https://www.innosent.de/radarsensoren/ips-354/) ([Handbuch](https://mm.digikey.com/Volume0/opasdata/d220001/medias/docus/2406/200730_Data%20Sheet_IPS-354_V1.5.pdf))
  - [isolierter DC-DC: ROE-3.305S](https://de.rs-online.com/web/p/dcdc-wandler/1392970)
  - [Akkupack](https://exp-tech.de/products/battery-lipo6600mah?_pos=33&_sid=1be852f75&_ss=r)
   - optional [TC4056](https://www.amazon.de/Aideepen-Lithium-Akku-Ladeplatine-Ladeger%C3%A4t-doppelten-Schutzfunktionen-Gr%C3%BCnes/dp/B0BZRT7Q6G/ref=sr_1_3?__mk_de_DE=%C3%85M%C3%85%C5%BD%C3%95%C3%91&crid=2O8HT3HTYGLBT&keywords=lipo%2Blader%2B4056&qid=1699275667&sprefix=lipo%2Blader%2B4056%2Caps%2C94&sr=8-3&th=1) Laderegler zum Laden der Batterie
  - SD-Karte nach Wahl
  - 40mm Kabel für Gnd des Radarmoduls
  - 40mm 2-Ader Kabel für Batteriestecker
  - Batteriestecker


- Gehäuse
  - [TRU COMPONENTS F3 Gehäuse](https://www.conrad.de/de/p/tru-components-f3-tc-6649456-universal-gehaeuse-115-x-90-x-55-abs-hellgrau-1-st-1662364.html)
  - [3D-Druck Halterung](https://community.fablab-cottbus.de/uploads/short-url/1HOEbMkdQxn1l1QFID4sH8OvwRe.stl)
  - [Lasercut Teile](https://community.fablab-cottbus.de/uploads/default/original/2X/0/07901a34dd58c6f2333bfa3191eb76f0abf1efdd.svg) zur Befestigung.
  - Velcro zur Befestigung des Akkus
  - Befestigungss  chrauben
    - 6x M4 für Einlegplatte und Seitenteile des Radarmoduls
    - 3x M3 für Befestigung der Platine

- Pfahlbefestigung
   - Lasercut Platte (enthält auch die Unterlegscheiben)
   - 4 x M5x10 für die Befestigungsplatte
   - 2x Velcro
   - 1x Kabelbinder

## Aufbau

### Platine vorbereiten

1. Teensy Vusb-Verbindung trennen (mit Messer). Danach einmal an USB anstecken und sicherstellen dass der Teensy nicht mehr von USB versorgt wird.
1. Knopfzellenbatterie an Teensy anlöten.
1. 14-Pin Pinleiste mit kurzer Seite in Audio Board einsetzen
1. Teensy auf die langen Stifte aufsetzen (Pins stehen über)
1. Pins auf der Audio Board Seite festlöten
1. von der Teensy Seite: 5V Pin und GND Pin (rechts und links vom USB) nach vorne biegen.
1. Batteriekabel an die überstehenden Pins von Gnd und 5V des Teensy löten.
1. Danach alle 14 Pins auch von der Teensy-Seite anlöten
1. Batteriestecker an Batteriekabel löten.
1. Die restlichen überlangen Pins können optional gekürzt werden.
1. DC-DC mit 3.3V und Gnd des Teensy verbinden (siehe Foto)
1. 6-Pin-Stecker mit langen Beinen anlöten:
    1. Pin 1 (Nummerierung wie am Radar-Modul) aus dem Stecker ziehen (wird nicht benötigt).
    1. Pin 5 und 6 umbiegen sodass sie in die line-in Anschlüsse des Audio Boards passen (siehe Foto).
    1. Enable (Pin 2) umbiegen und mit Pin 17 des Teensy verbinden (passt gerade so).
    1. Gnd (Pin 4) und Vcc (Pin 3) mit dem Output des DC-DC verbinden (siehe Foto). Hier muss man ca 4mm mit Lötzinn oder einem kurzen Draht überbrücken.
1. 4-Pin Stecker: GND (Pin 9) über kurzes Kabel mit Gnd am linein oder mic in verbinden.

### Radarmodul zusammen stecken

1. Radarmodul in 3D-Druck Halterung stecken (siehe Foto).
1. Teensy an 3D-Druck Halterung anschrauben (siehe Foto). Dabei die Stecker des Radarmoduls aufstecken.
1. 4-Pin Stecker am Radarmodul einstecken.
1. Lasercut Teile seitlich an das Radarmodul anschrauben.
1. SD-Karte in Audioboard einsetzen.

### Gehäuse vorbereiten

1. Grundplatte in Gehäuse schrauben.
1. Gewinde in Befestigungslöcher (Rückseite) schneiden.
1. Mastplatte mit M5-Schrauben und den Unterlegschrauben (im Lasercut Teil enthalten) an Rückseite anschrauben. 
1. Gummidichtung in Deckel einlegen (was zu lang ist abschneiden).

### Zusammenbau

1. Akku mit Kletterverschluss an Grundplatte befestigen.
1. Knopfzelle in Knopzellenhalter einsetzen.
1. Radarmodul mit Halterung in Schlitze in der Grundplatte stecken.
1. Teensy mit Batterie verbinden.

## Software installieren
1. Sketch drauf laden
2. Uhrzeit einstellen
3. Mit Processing überprüfen ob der Radarsensor richtig funktioniert
