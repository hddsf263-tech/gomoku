#ifndef APPSETTINGS_H
#define APPSETTINGS_H

#include <QColor>
#include <QString>

namespace AppCfg {

/// @brief 棋盘皮肤枚举
enum class BoardSkin { Jade, Wood, Ink, Rose, Custom };

/// @brief 棋子皮肤枚举
enum class PieceSkin { Classic, Jade, Onyx, Amber, Custom };

/// @brief 落子特效枚举
enum class PlaceEffect { Ring, Spark, None };

/// @brief 获胜特效枚举
enum class WinEffect { Pulse, Burst, None };

/// @brief 落子音效枚举
enum class PlaceSound { Wood, Click, Bubble, Silent, Custom };

/// @brief 获胜音效枚举
enum class WinSound { Chord, Rising, Sci, Silent, Custom };

/// @brief 棋盘背景基色（用于皮肤查询）
QColor boardBase(BoardSkin skin);
/// @brief 棋盘网格线颜色
QColor boardLine(BoardSkin skin);
/// @brief 棋盘星位点颜色
QColor boardStar(BoardSkin skin);

/// @brief 黑棋内芯高光色
QColor pieceBlackInner(PieceSkin skin);
/// @brief 黑棋外缘色
QColor pieceBlackOuter(PieceSkin skin);
/// @brief 白棋内芯高光色
QColor pieceWhiteInner(PieceSkin skin);
/// @brief 白棋外缘色
QColor pieceWhiteOuter(PieceSkin skin);

/// @brief 应用外观与音效设置
struct AppSettings {
    BoardSkin boardSkin = BoardSkin::Jade;
    PieceSkin pieceSkin = PieceSkin::Classic;
    PlaceEffect placeEffect = PlaceEffect::Ring;
    WinEffect winEffect = WinEffect::Pulse;
    PlaceSound placeSound = PlaceSound::Wood;
    WinSound winSound = WinSound::Chord;

    QString boardImagePath;
    QString blackImagePath;
    QString whiteImagePath;
    QString customPlaceAudioPath;
    QString customWinAudioPath;
    bool soundEnabled = true;

    /// @brief 从本地存储加载设置
    static AppSettings load();
    /// @brief 保存设置到本地存储
    void save() const;
    /// @brief 恢复默认设置
    void resetToDefaults();
};

} // namespace AppCfg

#endif // APPSETTINGS_H
