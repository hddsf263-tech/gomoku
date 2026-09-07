#include "AppSettings.h"

#include <QSettings>

namespace AppCfg {

QColor boardBase(BoardSkin skin) {
    switch (skin) {
        case BoardSkin::Jade:   return QColor("#2e5d52");
        case BoardSkin::Wood:   return QColor("#b57c45");
        case BoardSkin::Ink:    return QColor("#252b31");
        case BoardSkin::Rose:   return QColor("#7a3d2c");
        case BoardSkin::Custom: return QColor("#2e5d52");
    }
    return QColor("#2e5d52");
}

QColor boardLine(BoardSkin skin) {
    switch (skin) {
        case BoardSkin::Jade:   return QColor("#d5e4da");
        case BoardSkin::Wood:   return QColor("#5f3b1e");
        case BoardSkin::Ink:    return QColor("#aab3bb");
        case BoardSkin::Rose:   return QColor("#e2c2a8");
        case BoardSkin::Custom: return QColor("#d5e4da");
    }
    return QColor("#d5e4da");
}

QColor boardStar(BoardSkin skin) {
    switch (skin) {
        case BoardSkin::Jade:   return QColor("#c2d8cb");
        case BoardSkin::Wood:   return QColor("#5f3b1e");
        case BoardSkin::Ink:    return QColor("#c8ced3");
        case BoardSkin::Rose:   return QColor("#e2c2a8");
        case BoardSkin::Custom: return QColor("#c2d8cb");
    }
    return QColor("#c2d8cb");
}

QColor pieceBlackInner(PieceSkin skin) {
    switch (skin) {
        case PieceSkin::Classic: return QColor("#4e5655");
        case PieceSkin::Jade:    return QColor("#7fb8a2");
        case PieceSkin::Onyx:    return QColor("#6a7176");
        case PieceSkin::Amber:   return QColor("#f4c77e");
        case PieceSkin::Custom:  return QColor("#4e5655");
    }
    return QColor("#4e5655");
}

QColor pieceBlackOuter(PieceSkin skin) {
    switch (skin) {
        case PieceSkin::Classic: return QColor("#101214");
        case PieceSkin::Jade:    return QColor("#1f5b4c");
        case PieceSkin::Onyx:    return QColor("#14181b");
        case PieceSkin::Amber:   return QColor("#96571d");
        case PieceSkin::Custom:  return QColor("#101214");
    }
    return QColor("#101214");
}

QColor pieceWhiteInner(PieceSkin skin) {
    switch (skin) {
        case PieceSkin::Classic: return QColor("#ffffff");
        case PieceSkin::Jade:    return QColor("#f7fdf9");
        case PieceSkin::Onyx:    return QColor("#ffffff");
        case PieceSkin::Amber:   return QColor("#fffdf6");
        case PieceSkin::Custom:  return QColor("#ffffff");
    }
    return QColor("#ffffff");
}

QColor pieceWhiteOuter(PieceSkin skin) {
    switch (skin) {
        case PieceSkin::Classic: return QColor("#ddd9cb");
        case PieceSkin::Jade:    return QColor("#cfe5da");
        case PieceSkin::Onyx:    return QColor("#bcc3c8");
        case PieceSkin::Amber:   return QColor("#ead8ba");
        case PieceSkin::Custom:  return QColor("#ddd9cb");
    }
    return QColor("#ddd9cb");
}

AppSettings AppSettings::load() {
    AppSettings s;
    QSettings settings;

    s.boardSkin = static_cast<BoardSkin>(
        settings.value("skin/board", static_cast<int>(BoardSkin::Jade)).toInt());
    s.pieceSkin = static_cast<PieceSkin>(
        settings.value("skin/pieces", static_cast<int>(PieceSkin::Classic)).toInt());
    s.boardImagePath = settings.value("skin/boardImage").toString();
    s.blackImagePath = settings.value("skin/blackImage").toString();
    s.whiteImagePath = settings.value("skin/whiteImage").toString();

    s.placeEffect = static_cast<PlaceEffect>(
        settings.value("vfx/placeEffect", static_cast<int>(PlaceEffect::Ring)).toInt());
    s.winEffect = static_cast<WinEffect>(
        settings.value("vfx/winEffect", static_cast<int>(WinEffect::Pulse)).toInt());
    s.placeSound = static_cast<PlaceSound>(
        settings.value("vfx/placeSound", static_cast<int>(PlaceSound::Wood)).toInt());
    s.winSound = static_cast<WinSound>(
        settings.value("vfx/winSound", static_cast<int>(WinSound::Chord)).toInt());
    s.customPlaceAudioPath = settings.value("vfx/customPlaceAudio").toString();
    s.customWinAudioPath = settings.value("vfx/customWinAudio").toString();
    s.soundEnabled = settings.value("audio/soundEnabled", true).toBool();

    return s;
}

void AppSettings::save() const {
    QSettings settings;

    settings.setValue("skin/board", static_cast<int>(boardSkin));
    settings.setValue("skin/pieces", static_cast<int>(pieceSkin));
    settings.setValue("skin/boardImage", boardImagePath);
    settings.setValue("skin/blackImage", blackImagePath);
    settings.setValue("skin/whiteImage", whiteImagePath);

    settings.setValue("vfx/placeEffect", static_cast<int>(placeEffect));
    settings.setValue("vfx/winEffect", static_cast<int>(winEffect));
    settings.setValue("vfx/placeSound", static_cast<int>(placeSound));
    settings.setValue("vfx/winSound", static_cast<int>(winSound));
    settings.setValue("vfx/customPlaceAudio", customPlaceAudioPath);
    settings.setValue("vfx/customWinAudio", customWinAudioPath);
    settings.setValue("audio/soundEnabled", soundEnabled);

    settings.sync();
}

void AppSettings::resetToDefaults() {
    *this = AppSettings();
}

} // namespace AppCfg
