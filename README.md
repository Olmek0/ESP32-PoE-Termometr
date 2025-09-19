# ESP32-PoE-Termometr

## Informacje o projekcie

Przy tworzeniu projektu wykorzystany został program Arduino-IDE 2 wraz z najnowszą wersją płytki ESP32 w tymże programie. W celu włączenia projektu wymagane jest zapoznanie się z podstawową obsługą Arduino-IDE 2.

### Biblioteki do pobrania

- **OneWire** 2.3.8 by Jim Studt
 
- **Dallas Temperature** 4.0.3 by Miles Burton

- **Sqlite3Esp32** 2.5 by Arundale Ramanathan

- **WebSockets** 2.7.0 by Markus Sattler

- **UrlEncode** 1.0.1 by Masayuki Sugahara

- **ArduinoJson** 7.1.0 by Benoit Blanchon

### Partycja Własna

Ta aplikacja wykorzystuje funkcje własnej partycji (dołączona razem z plikami) przy użyciu Arduino-IDE.
Jak aktywować własną partycję:
1. Wejść w zakładkę tools w górnym menu Arduino-IDE.
2. Nacisnąć na przycisk Partition Scheme: "Nazwa Partycji".
3. W rozwiniętym menu wybrać opcję "Custom".

### Przesył plików

Projekt wymaga wykorzystania LittleFS w celu przesłania plików do pamięci flash. Jak to zrobić w Windowsie 11:
1. Wejść na link (https://github.com/earlephilhower/arduino-littlefs-upload/releases) i pobrać najnowszą wersję. W zakładce assests jest to plik zakończony rozszerzeniem ".vsix".
2. Na komputerze, wejść pod ścieżkę (jeśli Arduino-IDE zostało zainstalowane na dysku C) "C:\Użytkownicy\'NazwaUżytkownika'\.arduinoIDE\" 
3. Należy stworzyć tam folder o nazwie "plugins" (jeśli nie istnieje) i wstawić tam nową wersję pluginu. Starszą wersję trzeba będzie usunąć.
4. Trzeba teraz włączyć na nowo Arduino-IDE. Kiedy już aplikacja się otworzy, trzeba wybrać projekt. 
5. Kiedy już to się zrobi, trzeba wcisnąć kombinacje klawiszy Ctrl + Shift + P. W nowym oknie trzeba wyszukać hasła "Upload Little FS to Pico/ESP8266/ESP32" i kliknąć na nie. Program zacznie wtedy przesyłać pliki do pamięci flash ESP32. Proszę pamiętać, że serial monitor <ins>NIE</ins> może być włączony podczas tego procesu, gdyż wtedy ta metoda nie zadziała.

## Jak korzystać z alertów Whatsapp
  1. Należy dodać numer <ins>+34 684 72 39 62</ins> do kontaktów w Whatsappie
  2. Należy wysłać wiadomość o treści "<ins>I allow callmebot to send me messages</ins>" do powyższego kontaktu
  3. W wiadomości zwrotnej zostanie wysłany kod Apikey w wiadomości "<ins>Your APIKEY is XXXXXX</ins>", należy wartość tego klucza skopiować.
  4. Następnie, będąc na Webserverze termometru, należy przejść do opcji w górnym rogu strony a następnie kliknąć na blok ustawienia.
  5. W sekcji ustawienia alertów należy wpisać skopiowany wcześniej APIKEY, numer telefonu (bez spacji, razem z kodem krajowym np. *48123456789*), wybrać przedziały temperatury w odpowiednie pola. Następnie należy włączyć opcję "<ins>włącz alerty</ins>" i kliknąć przycisk "<ins>zapisz</ins>". W taki sposób alerty zostaną włączone.

W razie zmian numeru telefonu należy powtórzyć powyższe kroki.<br><br>

Dodatkowo, warto sprawdzić czy numer telefonu "callmebot" nie uległ zmianie. Informacje na ten temat znajdziesz w poniższym linku:

https://www.callmebot.com/blog/free-api-whatsapp-messages/
