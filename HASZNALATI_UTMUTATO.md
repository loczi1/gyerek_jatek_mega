# Gyerekjáték – rövid használati útmutató

## Kezelőszervek

- **Joystick fel/le:** rövid mozdulatra egyet lép. Nyomva tartva fél másodperc után 300 ms-onként ismét egyet lép, gyorsítás nélkül. Menüpontváltás után engedd középre, mielőtt újra léptetsz.
- **Menügomb:** menüpont megnyitása vagy választás jóváhagyása. Visszalépéshez válaszd a menü visszalépő sorát.
- **Játékgomb (D39):** a Cyclon játék indítása és a találat megadása. Minden megnyomás után engedd fel!
- **Potméter:** az A15 bemenetre csatlakozik. A háromállású kapcsoló két bemenete D25 és D24.

### Világító Ikarus kapcsolók

Az Ikarus kapcsoló és a beépített LED **külön csatlakozón** működik, de ugyanahhoz a fizikai kapcsolóhoz tartozik:

| Kapcsoló | Kapcsolóbemenet | Beépített LED kimenete |
| --- | --- | --- |
| 1 | D52 | D2 |
| 2 | D50 | D16 |
| 3 | D46 | D17 |
| 4 | D48 | D3 |
| 5 | D23 | D18 |
| 6 | D51 | D19 |

A bemenet a **Gombok / Digitalis**, a LED a **Digitalis ki** menüben tesztelhető. **A kapcsoló nem kapcsolja automatikusan a saját fényét**; Random módban is a 11 panellámpa egyikét választja véletlenszerűen. A címezhető LED-szalag adatvezetéke **D4**, nem D2. A beépített LED feszültségét, áramát és szükséges meghajtóáramkörét bekötés előtt ellenőrizni kell.

## Főmenü és játékválasztás

- A főmenü: **Szint1**, **Szint2**, **Szint3**, **Szint4**, **Szint5**, **Ujrainditas**, **Debug**. Szint1 alatt **Random**, Szint2 alatt sorrendben **Cyclon**, **Simon mondja**, majd **Sima villogó** választható (az LCD ékezet nélküli felirata: **Sima villogo**, belső módja: `Flasher`). Szint3–5 kérésre fenntartott, csak visszalépést tartalmazó menü.
- Bekapcsoláskor **nincs kiválasztott játék, a szalag sötét**. Előbb válassz játékot; a főmenü szintjei nem azonosak a Cyclon négy belső nehézségével.
- **Bármely játék (Cyclon, Random, Simon vagy Sima villogó / Flasher) kiválasztásakor az automatikus menü-visszalépés teljesen kikapcsol.** Készenlétben, játék és eredményjelzés közben, valamint a menet befejezése vagy hiba után is kiválasztva marad a játék, **menü-időkorlát nélkül**. A Simon és a Sima villogó 10 másodperces bemeneti tétlenségi időkorlátja ettől független játékszabály.
- A játékot tartalmazó szintmenü kézi elhagyása (visszalépés, főmenü vagy másik menü), a Debug és az újraindítás megszakítja a játékot és a hangot, mind a **100 tárolt LED-helyet** törli, és visszaállítja a normál menü/Debug **120 másodperces** időkorlátját. A Debugból visszatérve nincs automatikus folytatás: válassz újra játékot.

## Szint1 – Random

A **Random** kiválasztása után minden biztonságos, konfigurált digitális gomb- és kapcsolóérintkező használható, a Cyclon D39 gombja és az Ikarus kapcsolók is. A menü és a perifériák foglalt bemenetei kimaradnak. Minden új, pergésmentesített megnyomás a **11 panellámpa egyikét** gyújtja fel véletlen színnel, és **250 ms-os, 220–1760 Hz közötti hangot** ad. Nem a megnyomott Ikarus kapcsoló saját lámpáját kapcsolja. Új eseményhez engedd fel a gombot; a Random újbóli kiválasztása alaphelyzetbe állítja.

## Szint2 – Simon mondja

1. Válaszd a **Simon mondja** menüpontot. A 11 panelgomb bármelyikének új megnyomása indít; ez az indító nyomás még nem válasz. Engedd fel a gombot.
2. Figyeld a véletlen fénysort: **500 ms** kezdeti szünet után egy fény **500 ms-ig** világít, a lépések között **200 ms** szünet van. A sor egy elemmel indul, majd sikeres ismétlésenként bővül, legfeljebb **32 elemig**.
3. Ugyanazokkal a gombokkal ismételd meg a sorrendet. A bemutatás alatti nyomás nem válasz; a nyomva tartott gombot előbb fel kell engedni.
4. Minden helyes nyomás **540 ms zöld visszajelzést** és a Cyclon emelkedő győzelmi hangját adja. Rossz gomb, egyszerre több gomb vagy **10 másodperc válasz nélküli várakozás** esetén **740 ms piros visszajelzés** és a Cyclon vesztési hangja következik.
5. Hiba vagy a 32 elem teljesítése után új, friss gombnyomással indulhatsz ismét. A Simon újbóli kiválasztása törli az előző sort.

## Szint2 – Sima villogó

1. A Cyclon és a Simon mondja utáni harmadik játékot válaszd: az LCD-n **Sima villogo** látszik. Ugyanazt a **11 panelgombot** és az alább megadott, javított lámpakiosztást használja, mint a Simon. Bármelyik panelgomb új megnyomása indít; ez még nem válasz. Engedd fel a gombot, a bemutatás alatti nyomva tartás nem számít válasznak.
2. **500 ms** kezdeti szünet után egy véletlen céllámpa **500 ms-ig** világít, majd **200 ms-ra** kialszik. Ezután ugyanaz a cél újra kigyullad, és **a hozzá tartozó gomb megnyomására várva folyamatosan látható marad**. Nem memóriajáték: nem kell eltűnt fénysort megjegyezni.
3. Nyomd meg a világító célhoz tartozó gombot. Helyes válasznál **540 ms zöld visszajelzés** és ugyanaz az emelkedő győzelmi hang szól, mint a Simonban, majd a játék új, egyetlen célt választ.
4. A körök száma **korlátlan**. Nincs felhalmozott vagy növekvő sorozat, és nincs 32 lépés utáni befejezés. A Simon mondja növekvő sorozata és szabályai változatlanok.
5. Rossz gomb, egyszerre több gomb vagy **10 másodperc bemeneti tétlenség** esetén **740 ms piros visszajelzés** és ugyanaz a vesztési hang következik, majd a játék készenlétben új, friss indítónyomásra vár. A Sima villogó újbóli kiválasztása alaphelyzetbe állítja a játékot.
6. Készenlétben, bemutatáskor, válaszra várva és zöld/piros visszajelzéskor sincs automatikus menü-visszalépés, hiba után sem. Kézi visszalépés, a szintmenü elhagyása, Debug vagy újraindítás megszakítja a játékot és a hangot, törli mind a **100 LED-helyet**, és visszaállítja a normál 120 másodperces menü-időkorlátot.

## A 11 panellámpa helye

Az Excel `PinMap_export` lapja és a generált `LampPairs` változatlan. A jelzett fordított megjelenítés javítására a program az alsó sor lámpáit ellenkező irányban rendeli hozzá: **BTN07–11 → 75, 74, 73, 72, 71**. A gombbemenetek sorrendje és a felső két csoport nem változik. Az alábbiak a játékok tényleges, **1-től számozott** LED-helyei:

| Gombok | LED-helyek ugyanebben a sorrendben |
| --- | --- |
| BTN01, BTN02, BTN03 | 84, 85, 86 |
| BTN07, BTN08, BTN09, BTN10, BTN11 | 75, 74, 73, 72, 71 |
| BTN12, BTN13, BTN14 | 83, 82, 81 |

Az összes panellámpához **legalább 86 fizikai LED** szükséges; a Cyclon 75 LED-je ehhez nem elég. A generált `PinDefinition::stripPixel` mező 1-alapú, a 0 nem megadott helyet jelent. A generátor a szöveges/tartományos leírást kihagyja, a számszerű helyeket 1–100 között, ismétlődés nélkül fogadja el.

## Cyclon – kapd el a fényt!

1. A **Szint2 / Cyclon** menüpont kiválasztása után a játék a szivárványos készenléti képnél vár a **D39** gomb új megnyomására. Nincs külön zeneválasztás.
2. Nyomd meg és engedd fel a **játékgombot**. Körülbelül 4 másodperces bevezető után elindul a piros fény.
3. Nyomd meg újra a játékgombot, amikor a piros fény a célterületre ér. A cél közepét zöld, az első két szinten az elfogadható szomszédos helyeket narancssárga fény jelöli.
4. **Találat:** rövid, emelkedő hangjelzés és három zöld villanás, majd automatikusan következik a következő szint. **Hibás találat:** mélyülő hangjelzés és három piros villanás, és a játék véget ér. A hangjelzés egyszer szólal meg, a fényjelzéssel együtt.

A játék **négy szintből** áll: a fény egyre gyorsabb, a célterület pedig szűkül. A kék jelzőfények az aktuális szintet mutatják; az ötödik a negyedik szint teljesítésekor világít. A játék végén a játékgombbal indíthatsz új menetet.

## Újraindítás

- **Ujrainditas:** megszünteti a játékválasztást, megszakítja a hangot, és törli mind a 100 LED-helyet. A folytatáshoz válassz újra játékot a megfelelő szintmenüben.

A menü a hangjelzések alatt is használható.

## Debug – bemenetek és hibák ellenőrzése

Az Excel `ActiveLevel` mezője megadja a lámpa kapcsolási szintjét: a hat tranzisztoros Ikarus kimenetnél **HIGH = világít, LOW = kikapcsolva**. A `WS2812B` címkéjű kimenet LED-szalag, nem közvetlenül kapcsolható lámpa. A kötőjel utáni azonosító összerendeli a gombot/kapcsolót és a fényét, például `BTN01-left-blue_1` ↔ `LED01-left-blue_1`; a `down-5_N` ↔ `strip-down5_N` névváltozat is támogatott. Összesen 22 pár szerepel a konfigurációban; az Ikarus kapcsolók saját lámpája továbbra sem kapcsol be automatikusan.

A Debug megnyitása leállítja az aktuális játékot és a hangot, törli a szalagot. Így a játékgomb is biztonságosan tesztelhető. A főmenübe visszatérve nincs kiválasztott játék; újra ki kell választanod, nem folytatódik magától.

**Automatikus visszalépés a normál menüben és a Debugban:** ha nincs kiválasztott játék, és **120 másodpercig** nem használod a joystickot vagy a menügombot, a készülék visszatér a főmenübe. A bemeneti értékek automatikus frissülése és a tesztelt játékgomb nem hosszabbítja meg ezt az időt. **Kiválasztott játéknál nincs automatikus visszalépés**, készenlétben és a menet vége után sem. A LED-szalag teszt külön, saját 120 másodperces időzítője változatlan.

- **Gombok:** ismeretlen bekötésű gombok megkeresése a biztonságos **D0–D53 és A0–A15** bemeneteken. Az analóg csatlakozókat ilyenkor digitális gombbemenetként használja. A teszt idejére bekapcsol a hiányzó belső felhúzás: felengedve HIGH, a gombbal földre húzva LOW. A konfigurált gombok és kapcsolók felhúzása már bekapcsoláskor aktív. A kijelzőn a megfelelő név látszik (például **D39**, **A0** vagy **A15**).
	- **Utolso:** az utolsó gombnyomás csatlakozója felengedés után is látható. **Lenyomva:** a jelenleg lenyomott gombok. **Darab:** darabszám. **Tesztelt:** a vizsgált bemenetek száma.
	- Több gombnál legfeljebb három csatlakozó és **+N** jelenik meg; lejjebb minden vizsgált csatlakozó külön is ellenőrizhető. A tesztből kilépve visszaállnak az eredeti felhúzások.
	- A D0/D1, D4, D5, D13, D20/D21, D53, a joystick **A9** bemenete és a kimenetként használt csatlakozók kimaradnak. Legfeljebb **61 bemenet** tesztelhető. A menügomb és az A9 joystick továbbra is a navigációt kezeli.
	- A teszt a szabad, definiált lámpakimeneteket **LOW** szinten tartja: **D2, D3, D16, D17, D18, D19**. Így a lámpák nem világítanak, a lábak OUT-ként látszanak, és jelenleg **55 bemenet** vesz részt a tesztben. Más által már használt kimenetet és a D4 szalagjelet nem módosítja. Kilépéskor és időtúllépéskor visszaállítja a saját ideiglenes kimeneteinek korábbi bemeneti/felhúzási állapotát. Ha később LOW-aktív lámpa kerül az Excelbe, azt HIGH szinttel kapcsolja ki.
- **Digitalis ki:** a workbookban `DigitalOut` típussal jelölt, biztonságosan tesztelhető kimenetek kapcsolása. A jelenlegi konfigurációban a hat Ikarus LED kimenete érhető el: **D2, D3, D16, D17, D18, D19**, ebben a sorrendben. A D39 már játékgomb, nem tesztkimenet.
	- **DO allapot:** a kiválasztott kimenet és az utoljára beállított állapot: kezdetben **D2 ---**, majd például **D2 LOW** vagy **D2 HIGH**.
	- **Kov pin / Elozo:** a tesztelhető digitális kimenetek közötti váltás. Új `DigitalOut` csak HIGH vagy LOW `ActiveLevel` megadásával jelenik meg; a `WS2812B` és az üres érték kimarad. A LOW/HIGH menüpont tényleges lábszintet állít, nem logikai KI/BE parancsot.
	- **LOW / HIGH:** a kiválasztott kimenetet alacsony vagy magas szintre állítja. Másik csatlakozóra váltva a korábban meghajtott láb LOW bemenetre áll vissza. A menüből kilépve, a Debug elhagyásakor vagy időtúllépésnél a program a pillanatnyilag meghajtott tesztlábat is így állítja vissza.
- **LED szalag:** a D4-re kötött címezhető LED-szalag ellenőrzése.
	1. A **LED db** sorban állítsd be a szalag hosszát **1–100 LED** között, majd hagyd jóvá a menügombbal.
	2. Válaszd az **Inditas** lehetőséget. Az első LED fehéren világít.
	3. A **joystick gombjának rövid megnyomása és felengedése** egy LED-del továbbviszi a fehér fényt. Az utolsó után ismét az első következik. A kijelzőn például **LED 3/75** látszik: a 75 LED-es szalag harmadik LED-je világít.
	4. Kilépéshez **tartsd nyomva, majd engedd fel a joystick gombját**. A tesztfény kialszik. Két perc kattintás nélküli várakozás után automatikusan visszatér a főmenübe.
	- A teszt közben a fel/le mozgatás nem léptet, és a fény nem mozog automatikusan. A normál játékgomb helyett a menü joystickjának gombját használd.
- **Digitalis:** D0–D53 állapotai. A bekötött gomboknál LOW = lenyomva, HIGH = felengedve. A foglalt csatlakozók helyén UART, LED, HANG, I2C vagy OUT jelenik meg; ezeket a program nem alakítja át bemenetté.
- **Analog:** A0–A15 pillanatnyi értékei, **0–1023** között. Az A9 Joy a joystick bemenete. A **---** még nem mért értéket, az OUT kimenetként használt csatlakozót jelöl.
- **Allapot:** a készülék üzemállapota. Debug esetén a játék az ellenőrzés miatt áll.
- **Hibak:** a játék- és hangfeladat, valamint az LCD kapcsolatának állapota. **OK** = rendben; **FALLBACK** = a feladat tartalék működésre váltott; **NINCS / I2C HIBA / TIMEOUT** = kijelzőkommunikációs probléma.

A joystickkal görgess, a visszalépő sort a menügombbal válaszd ki. Az értékek automatikusan frissülnek. A Gombok teszthez a gombot a megfelelő bemenet és a közös GND közé kösd; külső lehúzó ellenállás nem szükséges. Egy állandó földelés vagy külső lehúzás szintén LOW-ként látszik. A bekötetlen bemenetek a többi nézetben ingadozhatnak, ez önmagában nem hiba. Csak ismert, 0–5 V-os jelhez és közös földhöz csatlakoztass tesztáramkört; foglalt vagy kimeneti csatlakozót ne köss a földre.

**Fontos:** a gomb nyomva tartása nem indít ismételten új menetet. Bekapcsoláskor nincs kiválasztott játék, a szalag sötét. Az új játékmódok is a meglévő két alkalmazásfeladatot használják. A jelen munkamenet Sima villogó módot tartalmazó Mega-fordítása és teljes automatikus tesztcsomagja sikeres: **118 natív teszt és 32 Python-ellenőrzés**; statikus SRAM: **3981 / 8192 bájt**, flash: **60542 / 253952 bájt**. Az új működés és a megőrzött alsósor-javítás **hardveren nincs ellenőrizve, feltöltés nem történt**. A kizárólag dokumentációs frissítéskor a fordítást és a teszteket nem futtattuk újra.
