#include "AppSettings.h"

namespace Gomoku {

BoardPalette boardPalette(const QString& skin) {
    if (skin == "wood") {
        return { QColor(181, 124, 69), QColor(95, 59, 30), QColor(95, 59, 30) };
    }
    if (skin == "ink") {
        return { QColor(37, 43, 49), QColor(170, 179, 187), QColor(200, 206, 211) };
    }
    if (skin == "rose") {
        return { QColor(122, 61, 44), QColor(226, 194, 168), QColor(226, 194, 168) };
    }
    return { QColor(46, 93, 82), QColor(213, 228, 218), QColor(194, 216, 203) };
}

PiecePalette piecePalette(const QString& skin) {
    if (skin == "jade") {
        return {
            QColor(127, 184, 162), QColor(31, 91, 76),
            QColor(247, 253, 249), QColor(207, 229, 218)
        };
    }
    if (skin == "onyx") {
        return {
            QColor(106, 113, 118), QColor(20, 24, 27),
            QColor(255, 255, 255), QColor(188, 195, 200)
        };
    }
    if (skin == "amber") {
        return {
            QColor(244, 199, 126), QColor(150, 87, 29),
            QColor(255, 253, 246), QColor(234, 216, 186)
        };
    }
    return {
        QColor(78, 86, 85), QColor(16, 18, 20),
        QColor(255, 255, 255), QColor(221, 217, 203)
    };
}

} // namespace Gomoku
