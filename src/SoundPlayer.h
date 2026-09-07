#ifndef SOUNDPLAYER_H
#define SOUNDPLAYER_H

#include <QSoundEffect>
#include <QString>
#include <QVector>

#include "AppSettings.h"

/// @brief 运行时音效播放器，可合成并与皮肤/特效设置联动
class SoundPlayer {
public:
    SoundPlayer();
    ~SoundPlayer();

    /// @brief 根据设置重建并加载音效
    void configure(const AppCfg::AppSettings& settings);

    /// @brief 播放落子音效，player：0=黑方，1=白方
    void playPlaceSound(int player);

    /// @brief 播放获胜音效
    void playWinSound();

    bool isMuted() const { return m_muted; }

private:
    struct ToneSpec {
        double freq;
        double endFreq;
        QString type;
        double duration;
        double volume;
        double delay;
        double filterFreq;
    };

    QString ensureToneFile(const QString& key,
                           const QVector<ToneSpec>& tones,
                           double totalDuration) const;
    static QByteArray synthesize(const QVector<ToneSpec>& tones,
                                 double totalDuration);

    QSoundEffect m_placeBlack;
    QSoundEffect m_placeWhite;
    QSoundEffect m_win;
    bool m_muted = false;
};

#endif // SOUNDPLAYER_H
