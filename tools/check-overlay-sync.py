# -*- coding: utf-8 -*-
"""Czy obie kopie zrodel Playerbotow sa identyczne?

    python tools/check-overlay-sync.py

Dockerfile buduje z linux-port/docker/game/src/server/game/src. Kopia
w linux-port/overlays/playerbot/src/game/src jest tym, co publikuje
allowlista aktualizacji (launcher/server-update-files.mt2009.txt), a
docs/COMPANION_MOD.md wymaga, zeby byly bajtowo identyczne.

Ten niezmiennik pekl juz trzy razy i za kazdym razem inaczej sie mscil:
raz wypuszczona paczka cofnelaby zmiany z kopii budowanej, raz zepsuty kod
przelezal dobe i wyszedl dopiero przy pierwszej pelnej przebudowie
(SIGN_SCRAP uzywany w playerbot_town.h, nigdzie niezdefiniowany). Recznie
tego nikt nie upilnuje - stad ten skrypt i punkt w CI.

Kod wyjscia: 0 gdy zgodne, 1 gdy nie.
"""
import hashlib
import os
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.abspath(os.path.join(HERE, '..'))
BUILD = os.path.join(ROOT, 'linux-port', 'docker', 'game', 'src', 'server', 'game', 'src')
OVERLAY = os.path.join(ROOT, 'linux-port', 'overlays', 'playerbot', 'src', 'game', 'src')


def digest(path):
    h = hashlib.sha256()
    with open(path, 'rb') as handle:
        for chunk in iter(lambda: handle.read(65536), b''):
            h.update(chunk)
    return h.hexdigest()


def main():
    for label, path in (('kontekst builda', BUILD), ('overlay', OVERLAY)):
        if not os.path.isdir(path):
            print('BLAD: nie ma katalogu %s: %s' % (label, path))
            return 1

    overlay_files = sorted(f for f in os.listdir(OVERLAY)
                           if os.path.isfile(os.path.join(OVERLAY, f)))
    if not overlay_files:
        print('BLAD: overlay jest pusty - to nie moze byc prawda')
        return 1

    rozne, brakujace = [], []
    for name in overlay_files:
        build_path = os.path.join(BUILD, name)
        if not os.path.isfile(build_path):
            brakujace.append(name)
            continue
        if digest(build_path) != digest(os.path.join(OVERLAY, name)):
            rozne.append(name)

    print('plikow w overlayu: %d' % len(overlay_files))
    if brakujace:
        print('')
        print('NIE MA W KOPII BUDOWANEJ (%d):' % len(brakujace))
        for name in brakujace:
            print('  %s' % name)
    if rozne:
        print('')
        print('ROZJECHANE (%d):' % len(rozne))
        for name in rozne:
            print('  %s' % name)

    if brakujace or rozne:
        print('')
        print('Kopia budowana jest ta, ktora sie kompiluje, wiec zwykle to ona')
        print('jest nowsza. Sprawdz, ktora strona jest prawdziwa, i wyrownaj:')
        print('  copy linux-port\\docker\\game\\src\\server\\game\\src\\<plik> '
              'linux-port\\overlays\\playerbot\\src\\game\\src\\')
        return 1

    print('OK: obie kopie sa bajtowo identyczne')
    return 0


if __name__ == '__main__':
    sys.exit(main())
