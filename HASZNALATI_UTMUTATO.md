# Gyerekjáték – rövid használati útmutató

## Megvalósított játékok – 2026-10-20

A korábbi, csak dokumentált tervet felváltotta a megvalósítás: **Cyclon a 2–5. szinten, fehér célú Sorrend memóriajáték minden 2–5. szinten**, továbbá új **Kapcsolózár** és **Párbaj** játék érhető el. Az LCD-n az új nevek ékezet nélkül: **Kapcsolozar**, **Parbaj**. Az Excel `GameLevel` lapját a generátor továbbra sem olvassa; a szabályok a programban vannak megvalósítva, az Excel átírása önmagában nem változtatja meg őket.

| Sorrend-szint | Játéktípus / célfény | További színes zavaró LED | Idő helyes gombonként | Pityegés a lejárat előtt |
| --- | --- | --- | --- | --- |
| 2 | Memóriajáték / fehér | Nincs | Nincs időlimit | Nincs megadva |
| 3 | Memóriajáték / fehér | 4–6 | 3 mp | 2 és 1 mp |
| 4 | Memóriajáték / fehér | 4–10 | 2 mp | 1 és 0,5 mp |
| 5 | Memóriajáték / fehér | 8–10 | 1,2 mp | 1 és 0,5 mp |

A zavaró fények telített színűek, nem fehérek; darabszámuk a fehér célon felül értendő. **Csak a bemutatás alatt látszanak, lépésenként új helyet/színt kapnak; válaszadáskor a cél és a zavaró fények is eltűnnek.** Ez tudatos programbeli döntés, az Excel nem rögzítette a megjelenítés ütemét. A 4–5. szint már nem a korábbi, folyamatosan látható célt kereső játék.

A Cyclon szintenként négy belső fokozatot használ, eltérő sebességgel/találati tartománnyal; lásd lent és a [pontos táblázatban](docs/EXCEL_CONFIG_ARCHITECTURE.md#implemented-cyclon-profiles). A két LED-es ablak a középsőt és a következő, nagyobb sorszámú LED-et jelenti. A Random, Random 2 és Sima villogó működése változatlan.

Ez a kiegészítés csak a dokumentációt frissíti a már megvalósított működéshez; nem módosítja az Excelt/programot, és nem indít fordítást, tesztet vagy feltöltést. Az ellenőrzési állapot az útmutató végén olvasható.

## Kezelőszervek

- **Joystick fel/le:** rövid mozdulatra egyet lép. Nyomva tartva fél másodperc után 300 ms-onként ismét egyet lép, gyorsítás nélkül. Menüpontváltás után engedd középre, mielőtt újra léptetsz.
- **Menügomb:** menüpont megnyitása vagy választás jóváhagyása. Visszalépéshez válaszd a menü visszalépő sorát.
- **Játékgomb (D39):** Cyclon indítása/találat, illetve a Kapcsolózár következő menetének indítása készenlétből. Minden megnyomás után engedd fel!
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

A bemenet a **Gombok / Digitalis**, a LED a **Digitalis ki** menüben tesztelhető. **Random 2 módban a megnyomott Ikarus kapcsoló saját lámpája világít.** Az eredeti Random továbbra is a 11 panellámpa egyikét választja véletlenszerűen; Debug alatt nincs automatikus gomb–lámpa kapcsolás. A címezhető LED-szalag adatvezetéke **D4**, nem D2. A beépített LED feszültségét, áramát és szükséges meghajtóáramkörét bekötés előtt ellenőrizni kell.

## Főmenü és játékválasztás

- A főmenü: **Szint1**, **Szint2**, **Szint3**, **Szint4**, **Szint5**, **Ujrainditas**, **Debug**. Minden szintmenüben van visszalépés. A játékok sorrendje:

	| Menü | Játékok |
	| --- | --- |
	| Szint1 | Random, Random 2 |
	| Szint2 | Cyclon, Sorrend, Sima villogo (Sima villogó) |
	| Szint3 | Cyclon, Sorrend, Kapcsolozar, Parbaj |
	| Szint4 | Cyclon, Sorrend, Kapcsolozar |
	| Szint5 | Cyclon, Sorrend |
- Bekapcsoláskor **nincs kiválasztott játék, a szalag sötét**. Előbb válassz játékot; a főmenü szintjei nem azonosak a Cyclon négy belső nehézségével.
- **Bármely kiválasztott játéknál teljesen kikapcsol az automatikus menü-visszalépés**, készenlétben, játék/eredményjelzés közben és hiba után is. A Sima villogó 10 másodperces időkorlátja, a Sorrend3–5 válaszideje és a Kapcsolózár4 határideje ettől független játékszabály; a játékot nem választják le.
- A játékot tartalmazó szintmenü kézi elhagyása (visszalépés, főmenü vagy másik menü), a Debug és az újraindítás megszakítja a játékot és a hangot, mind a **100 tárolt LED-helyet** törli, és visszaállítja a normál menü/Debug **120 másodperces** időkorlátját. A Debugból visszatérve nincs automatikus folytatás: válassz újra játékot.

## Szint1 – Random

A **Random** kiválasztása után minden biztonságos, konfigurált digitális gomb- és kapcsolóérintkező használható, a Cyclon D39 gombja és az Ikarus kapcsolók is. A menü és a perifériák foglalt bemenetei kimaradnak. Minden új, pergésmentesített megnyomás a **11 panellámpa egyikét** gyújtja fel véletlen színnel, és **250 ms-os, 220–1760 Hz közötti hangot** ad. Nem a megnyomott Ikarus kapcsoló saját lámpáját kapcsolja. Új eseményhez engedd fel a gombot; a Random újbóli kiválasztása alaphelyzetbe állítja.

## Szint1 – Random 2

1. Válaszd a **Szint1 / Random 2** menüpontot. Bármely biztonságos, konfigurált gomb vagy kapcsolóérintkező használható, az Ikarus kapcsolók és a D39 is; a menügomb és a joystick nem játszik.
2. Új gombnyomásra **a hozzá tartozó lámpa világít 1,2 másodpercig**, és rövid, vidám dallam szól. Mind a hat Ikarus saját lámpája működik; az öt fémkapcsolóhoz a 76–80. LED tartozik. A panelgombok az alábbi javított kiosztást használják.
3. A dallam mellett a szalag első 70 LED-je közül **három színes fény váltakozik 150 ms-onként**, egyszerű diszkóhatásként. A rövid, saját nyolcütemes dallam memóriaigénye kicsi; nem a teljes Greensleeves szól.
4. Egyszerre több frissen megnyomott gomb saját lámpái együtt világítanak. Új nyomás újraindítja a dallamot és lecseréli az előző fényhatást. A gombot új eseményhez fel kell engedni; nyomva tartva nem ismétel. A kiválasztáskor már nyomott gombot is előbb engedd fel.
5. Az önálló lámpával nem párosított **BTN05, BTN06, BTN15 (D39) és SW06** érintkezők csak a dallamot és a diszkófényt indítják. A program nem talál ki hozzájuk saját lámpát.
6. Az 1,2 másodperc végén a fény és a hang leáll, de a játék időkorlát nélkül kiválasztva marad. Újbóli kiválasztás alaphelyzetbe állítja. Visszalépés, másik játék, Debug vagy újraindítás azonnal megszakítja a hatást, kikapcsolja a saját kezelésű Ikarus lámpákat és törli mind a 100 szalaghelyet.

Az eredeti **Random** működése változatlan: ott véletlen panellámpa és egyetlen rövid hang jár a gombnyomásért.

## Szint2–5 – Sorrend: jegyezd meg a fehér fénysort!

Az eddigi Simon mondja neve **Sorrend**. Most minden szinten **növekvő memóriajáték**, nem folyamatosan látható célt kereső játék.

1. Válaszd a kívánt szint **Sorrend** menüpontját. A 11 panelgomb bármelyikének **friss** megnyomása indít; az indítónyomás még nem válasz. Engedd fel a gombot. Újbóli kiválasztás törli az előző sort.
2. **500 ms** kezdeti szünet után a sor elemei egyenként jelennek meg: **500 ms fehér célfény**, majd **200 ms sötét**. A sort egy elemmel kezded, sikeres teljes visszajátszás után egy új elemmel bővül, legfeljebb **32 elemig**.
3. Szint2-n nincs zavaró fény. Szint3-on **4–6**, Szint4-en **4–10**, Szint5-ön **8–10** további telített színű lámpa jelenik meg minden bemutatott fehér cél mellett, különböző helyeken, a cél helyét kihagyva. A zavarók lépésenként frissülnek, egy bemutatott lépésen belül nem változnak. **A fehér célokat jegyezd meg, ne a színes fényeket!**
4. A bemutatás után **a cél és a zavarók is sötétek**. Ugyanazokkal a gombokkal ismételd meg a fehér fények sorrendjét. Bemutatás alatt nyomott gomb nem válasz; fel kell engedni az új nyomáshoz.
5. Minden helyes válasz **540 ms zöld jelzést és győzelmi hangot** ad. A következő válaszra ismét teljes idő jár: Szint2 **korlátlan**, Szint3 **3 mp**, Szint4 **2 mp**, Szint5 **1,2 mp**. Az idő minden válaszra várás kezdetén indul, nem az egész sorra együtt vonatkozik; a zöld jelzés nem fogyasztja a következő válasz idejét.
6. Szint3-on **2 és 1 mp**, Szint4/5-ön **1 és 0,5 mp** hátralévő időnél rövid, **80 ms-os, 880 Hz-es** pityegés figyelmeztet. Nem hosszabbítja meg a határidőt. A pontos határidőre vagy utána elfogadott válasz már késő.
7. Rossz gomb, több gomb együtt vagy határidő-lejárat: **740 ms piros jelzés és vesztési hang**, majd készenlét. Hiba vagy 32 elem teljesítése után friss panelgombnyomás indít új sort. Szint2-n a várakozás önmagában nem hiba.

Visszalépés, szintmenü elhagyása, másik játék, Debug vagy újraindítás leállít minden hangot és törli mind a 100 LED-helyet. Kiválasztva nincs automatikus menü-visszalépés, készenlétben sem.

## Szint3/4 – Kapcsolózár

1. A **Kapcsolozar** kiválasztásakor **azonnal indul az első menet**; újbóli kiválasztás új alaphelyzetből szintén azonnal indít. Az öt fémkapcsolót használd: **SW01–05**, bemenetük sorrendben **D40, D42, D38, D36, D37**, saját fehér lámpájuk a **76–80. LED**.
2. **Szint3:** világító lámpához **HIGH**, sötéthez **LOW** kapcsolóállapot kell, időkorlát nélkül. **Szint4:** fordítva, világítóhoz **LOW**, sötéthez **HIGH** kell, és **2 másodperc** alatt kell megoldani a menet kezdetétől.
3. A HIGH=felfelé állás **bekötési feltételezés, még készüléken ellenőrizendő**. A program tényleges HIGH/LOW lábszintet néz, nem általános „lenyomott” jelet. Ezt a Debug / Digitalis nézetben ellenőrizd, és csak az ismert bekötés szerint értelmezd a kapcsoló irányát.
4. **Mind az öt kapcsolónak egyszerre kell a megfelelő állásban lennie, legalább 30 ms-ig stabilan.** A sötét lámpákhoz tartozó állapot is számít. A véletlen új feladvány nem lehet az éppen már megoldott kapcsolókombináció.
5. Siker: **540 ms zöld jelzés és győzelmi hang**. Szint4-en **1 és 0,5 mp** hátralévő időnél pityegés; lejáratkor **740 ms piros jelzés és vesztési hang**. A határidő pillanatában stabilizálódó megoldás már késő.
6. Eredmény után készenlét következik: **a D39 játékgomb friss megnyomása** indít új feladványt. Nyomva tartás nem indít újra. Visszalépés, másik játék, Debug vagy újraindítás azonnal megszakít; nincs automatikus menü-időkorlát.

## Szint3 – Párbaj

1. Válaszd a **Parbaj** menüpontot: **azonnal indul a 2 másodperces bemutatás**. Bal játékos: felső **BTN01–03**; jobb játékos: felső **BTN12–14**. Mindkét oldalon három különböző szín látszik az ötszínű készletből, és **pontosan egy közös szín** van.
2. A bemutatás végéig ne válaszolj; **1 és 0,5 mp** hátralévő időnél pityegés szól. Utána az **alsó középső, BTN09 lámpája (73. LED)** megmutatja a közös színt; a felső színek tovább világítanak. Az alsó lámpa csak jelzés, **nem válaszgomb**.
3. **Saját felső sora megfelelő színű gombját** nyomja meg mindkét játékos. Nincs válaszadási időkorlát. A bemutatás alatt nyomott gomb nem számít; előbb minden felső gombot fel kell engedni, majd frissen nyomni.
4. Az első elfogadott válasz dönt: helyes gombbal nyersz, hibással veszítesz. Saját oldalon több gomb együtt **hibás**, akkor is, ha a jó gomb köztük van. Ha mindkét válasz ugyanabban a feldolgozási lépésben érkezik: mindkettő helyes → **mindkét oldal zöld, döntetlen**; mindkettő hibás → **mindkettő piros, döntetlen**; vegyes → **a helyes válaszoló nyer**.
5. Az eredmény három **90 ms világít/90 ms sötét** villanás, összesen **540 ms**. A nyertes oldal zöld, a vesztes piros. Egyetlen győzelmi hang szól, ha volt helyes válasz; különben egyszeri vesztési hang, amely az eredmény végén leáll.
6. Készenlétből **bármely felső gomb friss megnyomása** indít új bemutatást; ez még nem válasz. Az eredmény alatt nyomva tartott gomb nem indít újra. Újbóli menükiválasztás azonnal új bemutatást indít; D39 és az alsó gombok nem indítják újra a Párbajt.

A két új játék is a meglévő két alkalmazásfeladatot és egyetlen 100 LED-es tárolót használja. Visszalépés, másik játék, Debug vagy újraindítás minden hangot leállít és mind a 100 LED-helyet törli; nincs automatikus folytatás.

## Szint2 – Sima villogó

1. A Cyclon és a Sorrend utáni harmadik játékot válaszd: az LCD-n **Sima villogo** látszik. Ugyanazt a **11 panelgombot** és az alább megadott, javított lámpakiosztást használja, mint a Sorrend. Bármelyik panelgomb új megnyomása indít; ez még nem válasz. Engedd fel a gombot, a bemutatás alatti nyomva tartás nem számít válasznak.
2. **500 ms** kezdeti szünet után egy véletlen céllámpa **500 ms-ig** világít, majd **200 ms-ra** kialszik. Ezután ugyanaz a cél újra kigyullad, és **a hozzá tartozó gomb megnyomására várva folyamatosan látható marad**. Nem memóriajáték: nem kell eltűnt fénysort megjegyezni.
3. Nyomd meg a világító célhoz tartozó gombot. Helyes válasznál **540 ms zöld visszajelzés** és ugyanaz az emelkedő győzelmi hang szól, mint a Sorrendben, majd a játék új, egyetlen célt választ.
4. A körök száma **korlátlan**. Nincs felhalmozott vagy növekvő sorozat, és nincs 32 lépés utáni befejezés. Ez különbözik a minden 2–5. szinten növekvő Sorrend memóriajátéktól.
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

1. A **Szint2/3/4/5 / Cyclon** kiválasztása után a játék a szivárványos készenléti képnél vár a **D39** gomb új megnyomására. Újbóli kiválasztás alaphelyzetbe állítja az adott profilt. Nincs külön zeneválasztás.
2. Nyomd meg és engedd fel a **játékgombot**. Körülbelül 4 másodperces bevezető után elindul a piros fény.
3. Nyomd meg újra a játékgombot, amikor a piros fény a célterületre ér. A cél közepe zöld, az elfogadható szomszédos helyek narancssárgák, az adott profil/fokozat ablakának megfelelően.
4. **Találat:** rövid, emelkedő hangjelzés és három zöld villanás, majd automatikusan következik a következő szint. **Hibás találat:** mélyülő hangjelzés és három piros villanás, és a játék véget ér. A hangjelzés egyszer szólal meg, a fényjelzéssel együtt.

A kiválasztott profil **négy egymást követő belső fokozatból** áll; ezek nem a főmenü szintjei. Az értékek az 1/2/3/4. fokozatra vonatkoznak:

| Menüszint | Sebesség (ms/LED) | Elfogadott célterület (LED) |
| --- | --- | --- |
| 2 | 50 / 40 / 40 / 40 | 9 / 5 / 5 / 3 |
| 3 | 40 / 35 / 30 / 30 | 9 / 5 / 5 / 3 |
| 4 | 50 / 40 / 35 / 20 | 5 / 3 / 2 / 2 |
| 5 | 40 / 35 / 30 / 20 | 5 / 3 / 2 / 1 |

A páratlan szélesség középre szimmetrikus; **2 LED = közép és a következő, nagyobb sorszámú LED**, nem két szomszéd a közép mellett. A kék jelzőfények az aktuális fokozatot mutatják; az ötödik a negyedik teljesítésekor világít. A játék végén friss D39 nyomással indíthatsz új menetet. Minden nyomás után engedd fel!

## Újraindítás

- **Ujrainditas:** megszünteti a játékválasztást, megszakítja a hangot, és törli mind a 100 LED-helyet. A folytatáshoz válassz újra játékot a megfelelő szintmenüben.

A menü a hangjelzések alatt is használható.

## Debug – bemenetek és hibák ellenőrzése

Az Excel `ActiveLevel` mezője megadja a lámpa kapcsolási szintjét: a hat tranzisztoros Ikarus kimenetnél **HIGH = világít, LOW = kikapcsolva**. A `WS2812B` címkéjű kimenet LED-szalag, nem közvetlenül kapcsolható lámpa. A kötőjel utáni azonosító összerendeli a gombot/kapcsolót és a fényét, például `BTN01-left-blue_1` ↔ `LED01-left-blue_1`; a `down-5_N` ↔ `strip-down5_N` névváltozat is támogatott. Összesen 22 pár szerepel a konfigurációban; a saját Ikarus lámpát csak a **Random 2** játék kapcsolja gombnyomásra, a Debug nem.

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

**Korábbi Random 2 ellenőrzés, 2026-10-19, a Sorrend-módosítás előtt:** a Mega-fordítás és a teljes normál, valamint nem definiált műveleteket ellenőrző tesztfuttatás sikeres volt: **126 natív teszt és 33 Python-ellenőrzés**; statikus SRAM: **4123 / 8192 bájt**, flash: **62336 / 253952 bájt**. Ez nem a mostani Sorrend-változat teljes teszteredménye.

**Korábbi Sorrend-módosítás, 2026-10-19:** Mega-fordítás és teljes normál/szanitizált csomag sikeres: **135 natív teszt és 34 Python-ellenőrzés**, statikus SRAM **4168 / 8192 bájt**, flash **63386 / 253952 bájt**. Ez korábbi állapot, nem az új játékok igazolása.

**A megvalósítási munkamenet eredménye, 2026-10-20:** a teljes normál és a nem definiált műveleteket ellenőrző szanitizált csomag egyaránt sikeres: **157 natív teszt és 36 Python-ellenőrzés** futtatásonként. Mega-fordítás sikeres: statikus SRAM **4356 / 8192 bájt**, flash **67936 / 253952 bájt**. A statikus SRAM-adat nem tartalmazza a futás közbeni verem/heap igényét. Nem történt feltöltés vagy hardveres ellenőrzés ehhez a változathoz. Az új profilok, Kapcsolózár/Párbaj, Random 2 és az alsósor-javítás készülékes ellenőrzése szükséges; lásd [hardveres ellenőrzőlista](docs/HARDWARE_TEST_CHECKLIST.md). A workbook és a felhasználó tulajdonában álló átadó változatlan maradt.
