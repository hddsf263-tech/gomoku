#pragma once

#include <QObject>
#include <QHash>
#include <QSet>
#include <QString>

#include "GomokuCore.h"

class QMediaPlayer;
class QAudioOutput;
class QSoundEffect;

namespace Gomoku {

class SoundManager : public QObject {
    Q_OBJECT

public:
    explicit SoundManager(QObject* parent = nullptr);
    ~SoundManager() override;

    void setMuted(bool muted);
    void setPlaceSound(const QString& mode, const QString& customPath = QString());
    void setWinSound(const QString& mode, const QString& customPath = QString());
    void playPlace(Piece piece);
    void playWin();
    int readyBuiltinCount() const;

private:
    QString ensureWav(const QString& name, const QString& generatorKey);
    void playBuiltin(const QString& key);
    void playCustom(const QString& path);
    void ensureAllEffects();

    bool muted_ = false;
    QString placeMode_ = "wood";
    QString winMode_ = "chord";
    QString customPlacePath_;
    QString customWinPath_;

    QHash<QString, QSoundEffect*> effects_;
    QSet<QString> pendingEffectPlays_;
    QMediaPlayer* customPlayer_ = nullptr;
    QAudioOutput* audioOutput_ = nullptr;
    bool pendingCustomPlay_ = false;
};

} // namespace Gomoku
