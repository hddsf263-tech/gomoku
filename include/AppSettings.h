#pragma once

#include <QColor>
#include <QString>

namespace Gomoku {

struct BoardPalette {
    QColor base;
    QColor line;
    QColor star;
};

struct PiecePalette {
    QColor blackLight;
    QColor blackDark;
    QColor whiteLight;
    QColor whiteDark;
};

BoardPalette boardPalette(const QString& skin);
PiecePalette piecePalette(const QString& skin);

struct AppSettings {
    QString boardSkin = "jade";
    QString pieceSkin = "classic";
    QString placeEffect = "ring";
    QString winEffect = "pulse";
    QString placeSound = "wood";
    QString winSound = "chord";

    QString boardImage;
    QString blackImage;
    QString whiteImage;
    QString customPlaceAudio;
    QString customWinAudio;
    bool muted = false;
};

} // namespace Gomoku
