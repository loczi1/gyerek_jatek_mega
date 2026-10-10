# Gyerekjáték – rövid használati útmutató

## Kezelőszervek

- **Joystick fel/le:** rövid mozdulatra egyet lép. Nyomva tartva fél másodperc után 300 ms-onként ismét egyet lép, gyorsítás nélkül. Menüpontváltás után engedd középre, mielőtt újra léptetsz.
- **Menügomb:** menüpont megnyitása vagy választás jóváhagyása. Visszalépéshez válaszd a menü visszalépő sorát.
- **Játékgomb:** a Cyclon játék indítása és a találat megadása. Minden megnyomás után engedd fel!

## Cyclon – kapd el a fényt!

1. A főmenüben csak **Jatek**, **Ujrainditas** és **Debug** található. A **Jatek** az egyetlen elérhető játékot, a Cyclont választja; nincs külön zeneválasztás. Bekapcsolás után a LED-ek szivárványszínben világítanak, a készülék játékra kész.
2. Nyomd meg és engedd fel a **játékgombot**. Körülbelül 4 másodperces bevezető után elindul a piros fény.
3. Nyomd meg újra a játékgombot, amikor a piros fény a célterületre ér. A cél közepét zöld, az első két szinten az elfogadható szomszédos helyeket narancssárga fény jelöli.
4. **Találat:** rövid, emelkedő hangjelzés és három zöld villanás, majd automatikusan következik a következő szint. **Hibás találat:** mélyülő hangjelzés és három piros villanás, és a játék véget ér. A hangjelzés egyszer szólal meg, a fényjelzéssel együtt.

A játék **négy szintből** áll: a fény egyre gyorsabb, a célterület pedig szűkül. A kék jelzőfények az aktuális szintet mutatják; az ötödik a negyedik szint teljesítésekor világít. A játék végén a játékgombbal indíthatsz új menetet.

## Újraindítás

- **Ujrainditas:** a játékot kezdőállapotba állítja és megszakítja az esetleg még szóló játékhangot. Új menethez nyomd meg a játékgombot.

A menü a hangjelzések alatt is használható.

## Debug – bemenetek és hibák ellenőrzése

A Debug megnyitása leállítja az aktuális menetet és a hangot. Így a játékgomb is biztonságosan tesztelhető. A főmenübe visszatérve a játék új menet indítására vár.

**Automatikus visszalépés:** ha 2 percig nem használod a joystickot vagy a menügombot, a készülék visszatér a főmenübe. A bemeneti értékek automatikus frissülése és a tesztelt játékgomb nem hosszabbítja meg ezt az időt.

- **Gombok:** ismeretlen bekötésű gombok megkeresése. A teszt idejére a biztonságos digitális bemeneteken bekapcsol a belső felhúzás: felengedve HIGH, a gombbal földre húzva LOW. A kijelzőn megjelenik az érintett csatlakozó száma (például D46).
	- **Utolso:** az utolsó gombnyomás csatlakozója felengedés után is látható. **Lenyomva:** a jelenleg lenyomott gombok. **Darab:** darabszám. **Tesztelt:** a vizsgált bemenetek száma.
	- Több gombnál legfeljebb három csatlakozó és **+N** jelenik meg; lejjebb minden vizsgált csatlakozó külön is ellenőrizhető. A tesztből kilépve visszaállnak az eredeti felhúzások.
	- A D0/D1, D2, D5, D13, D20/D21, D53 és a kimenetként használt csatlakozók kimaradnak. A menügomb továbbra is a navigációt kezeli, nem tesztgomb.
- **Digitalis:** D0–D53 állapotai. A bekötött gomboknál LOW = lenyomva, HIGH = felengedve. A foglalt csatlakozók helyén UART, LED, HANG, I2C vagy OUT jelenik meg; ezeket a program nem alakítja át bemenetté.
- **Analog:** A0–A15 pillanatnyi értékei, **0–1023** között. Az A9 Joy a joystick bemenete. A **---** még nem mért értéket, az OUT kimenetként használt csatlakozót jelöl.
- **Allapot:** a készülék üzemállapota. Debug esetén a játék az ellenőrzés miatt áll.
- **Hibak:** a játék- és hangfeladat, valamint az LCD kapcsolatának állapota. **OK** = rendben; **FALLBACK** = a feladat tartalék működésre váltott; **NINCS / I2C HIBA / TIMEOUT** = kijelzőkommunikációs probléma.

A joystickkal görgess, a visszalépő sort a menügombbal válaszd ki. Az értékek automatikusan frissülnek. A Gombok teszthez a gombot a megfelelő bemenet és a közös GND közé kösd; külső lehúzó ellenállás nem szükséges. Egy állandó földelés vagy külső lehúzás szintén LOW-ként látszik. A bekötetlen bemenetek a többi nézetben ingadozhatnak, ez önmagában nem hiba. Csak ismert, 0–5 V-os jelhez és közös földhöz csatlakoztass tesztáramkört; foglalt vagy kimeneti csatlakozót ne köss a földre.

**Fontos:** a játékgomb nyomva tartása nem indít ismételten új menetet. Kikapcsolás után a készülék ismét Cyclon módban indul.