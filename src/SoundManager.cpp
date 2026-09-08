#include "SoundManager.h"

#include <QAudioOutput>
#include <QBuffer>
#include <QDataStream>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QMediaPlayer>
#include <QSoundEffect>
#include <QStandardPaths>
#include <QUrl>

#include <cmath>
#include <cstdint>

namespace Gomoku {

namespace {

constexpr int kSampleRate = 44100;

void writeWav(const QString& path, const QVector<double>& samples) {
    QByteArray bytes;
    const int dataSize = samples.size() * 2;
    QBuffer buffer(&bytes);
    buffer.open(QIODevice::WriteOnly);
    QDataStream out(&buffer);
    out.setByteOrder(QDataStream::LittleEndian);
    out << quint32(0x46464952) // RIFF
        << quint32(36 + dataSize)
        << quint32(0x45564157) // WAVE
        << quint32(0x20746d66) // fmt
        << quint32(16)
        << quint16(1)
        << quint16(1)
        << quint32(kSampleRate);
    const int byteRate = kSampleRate * 2;
    out << quint32(byteRate)
        << quint16(2)
        << quint16(16)
        << quint32(0x61746164) // data
        << quint32(dataSize);

    for (const double sample : samples) {
        const double clamped = std::max(-1.0, std::min(1.0, sample));
        const int16_t value = static_cast<int16_t>(clamped * 32767);
        bytes.append(char(value & 0xff));
        bytes.append(char((value >> 8) & 0xff));
    }

    QFile file(path);
    if (file.open(QIODevice::WriteOnly | QIODevice::Truncate)) {
        file.write(bytes);
    }
}

QVector<double> tone(double frequency, double duration,
                     double endFrequency = 0,
                     double volume = 0.5,
                     double delay = 0) {
    const int start = static_cast<int>(delay * kSampleRate);
    const int count = static_cast<int>(duration * kSampleRate) + start + 20;
    QVector<double> samples(count, 0.0);
    for (int i = start; i < count; i++) {
        const double t = (i - start) / static_cast<double>(kSampleRate);
        const double progress = t / duration;
        const double freq = endFrequency > 0
            ? frequency + (endFrequency - frequency) * progress
            : frequency;
        const double attack = std::min(1.0, t / 0.012);
        const double release = std::min(1.0, (duration - t) / 0.04);
        samples[i] = volume * attack * std::max(0.0, release) *
                     std::sin(2.0 * M_PI * freq * t);
    }
    return samples;
}

QVector<double> mix(const QVector<QVector<double>>& layers) {
    int maxLength = 0;
    for (const auto& layer : layers) {
        maxLength = qMax(maxLength, int(layer.size()));
    }
    QVector<double> result(maxLength, 0.0);
    for (const auto& layer : layers) {
        for (int i = 0; i < layer.size(); i++) {
            result[i] += layer[i];
        }
    }
    return result;
}

void generatePlaceWood(const QString& path, int black) {
    writeWav(path, tone(black ? 240 : 320, 0.16,
                        black ? 90 : 120, 0.52));
}

void generatePlaceClick(const QString& path) {
    writeWav(path, tone(880, 0.10, 260, 0.35));
}

void generatePlaceBubble(const QString& path) {
    writeWav(path, tone(220, 0.14, 760, 0.32));
}

void generateWinChord(const QString& path) {
    writeWav(path, mix({
        tone(392, 0.34, 0, 0.22, 0.00),
        tone(523.25, 0.34, 0, 0.22, 0.08),
        tone(659.25, 0.34, 0, 0.22, 0.16),
        tone(783.99, 0.34, 0, 0.22, 0.24)
    }));
}

void generateWinRising(const QString& path) {
    writeWav(path, mix({
        tone(523.25, 0.22, 0, 0.20, 0.00),
        tone(659.25, 0.22, 0, 0.20, 0.07),
        tone(783.99, 0.22, 0, 0.20, 0.14),
        tone(1046.5, 0.26, 0, 0.20, 0.21),
        tone(1318.51, 0.34, 0, 0.20, 0.28)
    }));
}

void generateWinSci(const QString& path) {
    writeWav(path, mix({
        tone(160, 0.42, 1150, 0.18, 0.00),
        tone(720, 0.30, 280, 0.18, 0.12)
    }));
}

} // namespace

SoundManager::SoundManager(QObject* parent)
    : QObject(parent)
    , customPlayer_(new QMediaPlayer(this))
    , audioOutput_(new QAudioOutput(this))
{
    audioOutput_->setVolume(0.9);
    customPlayer_->setAudioOutput(audioOutput_);
    connect(customPlayer_, &QMediaPlayer::mediaStatusChanged,
            this, [this](QMediaPlayer::MediaStatus status) {
        if (!muted_ && pendingCustomPlay_ &&
            (status == QMediaPlayer::BufferedMedia ||
             status == QMediaPlayer::LoadedMedia)) {
            pendingCustomPlay_ = false;
            customPlayer_->play();
        }
    });
    ensureAllEffects();
}

SoundManager::~SoundManager() = default;

void SoundManager::setMuted(bool muted) {
    muted_ = muted;
    if (muted_) {
        pendingEffectPlays_.clear();
        pendingCustomPlay_ = false;
        customPlayer_->stop();
    }
}

void SoundManager::setPlaceSound(const QString& mode, const QString& customPath) {
    placeMode_ = mode;
    customPlacePath_ = customPath;
}

void SoundManager::setWinSound(const QString& mode, const QString& customPath) {
    winMode_ = mode;
    customWinPath_ = customPath;
}

QString SoundManager::ensureWav(const QString& name,
                                const QString& generatorKey) {
    const QString directory = QStandardPaths::writableLocation(
        QStandardPaths::TempLocation);
    const QString path = directory + "/gomoku-" + name + ".wav";
    if (QFileInfo::exists(path)) {
        if (QFileInfo(path).size() > 100) {
            return path;
        }
        QFile::remove(path);
    }
    if (generatorKey == "place-wood-black") {
        generatePlaceWood(path, true);
    } else if (generatorKey == "place-wood-white") {
        generatePlaceWood(path, false);
    } else if (generatorKey == "place-click") {
        generatePlaceClick(path);
    } else if (generatorKey == "place-bubble") {
        generatePlaceBubble(path);
    } else if (generatorKey == "win-chord") {
        generateWinChord(path);
    } else if (generatorKey == "win-rising") {
        generateWinRising(path);
    } else if (generatorKey == "win-sci") {
        generateWinSci(path);
    }
    return path;
}

void SoundManager::playBuiltin(const QString& key) {
    QString path;
    QString effectKey;
    if (key == "wood-black") {
        path = ensureWav("place-wood-black", "place-wood-black");
        effectKey = "place-wood-black";
    } else if (key == "wood-white") {
        path = ensureWav("place-wood-white", "place-wood-white");
        effectKey = "place-wood-white";
    } else if (key == "click") {
        path = ensureWav("place-click", "place-click");
        effectKey = "place-click";
    } else if (key == "bubble") {
        path = ensureWav("place-bubble", "place-bubble");
        effectKey = "place-bubble";
    } else if (key == "chord") {
        path = ensureWav("win-chord", "win-chord");
        effectKey = "win-chord";
    } else if (key == "rising") {
        path = ensureWav("win-rising", "win-rising");
        effectKey = "win-rising";
    } else if (key == "sci") {
        path = ensureWav("win-sci", "win-sci");
        effectKey = "win-sci";
    }
    if (path.isEmpty() || effectKey.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }
    QSoundEffect* effect = effects_.value(effectKey);
    if (!effect) {
        effect = new QSoundEffect(this);
        effect->setVolume(0.9);
        effects_.insert(effectKey, effect);
        connect(effect, &QSoundEffect::statusChanged,
                this, [this, effect, effectKey]() {
            if (effect->status() == QSoundEffect::Error) {
                pendingEffectPlays_.remove(effectKey);
            } else if (effect->status() == QSoundEffect::Ready &&
                       pendingEffectPlays_.remove(effectKey) && !muted_) {
                effect->play();
            }
        });
        effect->setSource(QUrl::fromLocalFile(path));
    }
    if (effect->status() == QSoundEffect::Ready) {
        effect->play();
    } else {
        pendingEffectPlays_.insert(effectKey);
    }
}

void SoundManager::ensureAllEffects() {
    const QStringList keys = {
        "place-wood-black", "place-wood-white", "place-click",
        "place-bubble", "win-chord", "win-rising", "win-sci"
    };
    for (const QString& key : keys) {
        QString path;
        if (key.startsWith("place-wood-")) {
            path = ensureWav(key, key);
        } else if (key.startsWith("place-")) {
            path = ensureWav(key, key);
        } else {
            path = ensureWav(key, key);
        }
        if (path.isEmpty()) {
            continue;
        }
        QSoundEffect* effect = new QSoundEffect(this);
        effect->setVolume(0.9);
        effects_.insert(key, effect);
        connect(effect, &QSoundEffect::statusChanged,
                this, [this, effect, key]() {
            if (effect->status() == QSoundEffect::Error) {
                pendingEffectPlays_.remove(key);
            } else if (effect->status() == QSoundEffect::Ready &&
                       pendingEffectPlays_.remove(key) && !muted_) {
                effect->play();
            }
        });
        effect->setSource(QUrl::fromLocalFile(path));
    }
}

void SoundManager::playCustom(const QString& path) {
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        return;
    }
    pendingCustomPlay_ = false;
    customPlayer_->stop();
    customPlayer_->setSource(QUrl::fromLocalFile(path));
    if (customPlayer_->mediaStatus() == QMediaPlayer::BufferedMedia ||
        customPlayer_->mediaStatus() == QMediaPlayer::LoadedMedia) {
        customPlayer_->play();
    } else {
        pendingCustomPlay_ = true;
    }
}

void SoundManager::playPlace(Piece piece) {
    if (muted_) {
        return;
    }
    if (placeMode_ == "custom") {
        playCustom(customPlacePath_);
        return;
    }
    const QString key = placeMode_ + (piece == Piece::Black ? "-black" : "-white");
    playBuiltin(key == "wood-black" || key == "wood-white" ? key : placeMode_);
}

void SoundManager::playWin() {
    if (muted_) {
        return;
    }
    if (winMode_ == "custom") {
        playCustom(customWinPath_);
        return;
    }
    playBuiltin(winMode_);
}

int SoundManager::readyBuiltinCount() const {
    int ready = 0;
    for (QSoundEffect* effect : effects_) {
        if (effect && effect->status() == QSoundEffect::Ready) {
            ready++;
        }
    }
    return ready;
}

} // namespace Gomoku
