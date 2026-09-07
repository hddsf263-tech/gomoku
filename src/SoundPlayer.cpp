#include "SoundPlayer.h"

#include <QByteArray>
#include <QDir>
#include <QFile>
#include <QStandardPaths>
#include <QUrl>

#include <algorithm>
#include <cmath>

namespace {

constexpr int kSampleRate = 44100;
constexpr double kPi = 3.14159265358979323846;

struct LowPass {
    double alpha = 1.0;
    double state = 0.0;

    void setCutoff(double cutoff) {
        if (cutoff <= 0.0) {
            alpha = 1.0;
            return;
        }
        const double dt = 1.0 / kSampleRate;
        const double rc = 1.0 / (2.0 * kPi * cutoff);
        alpha = dt / (rc + dt);
    }

    double process(double input) {
        state += alpha * (input - state);
        return state;
    }
};

double waveform(double phase, const QString& type) {
    // phase 为弧度；归一化到 [0,1)
    double p = std::fmod(phase, 2.0 * kPi) / (2.0 * kPi);
    if (p < 0.0) {
        p += 1.0;
    }

    if (type == "triangle") {
        return 4.0 * std::abs(p - 0.5) - 1.0;
    }
    if (type == "sawtooth" || type == "saw") {
        return 2.0 * p - 1.0;
    }
    return std::sin(2.0 * kPi * p);
}

QByteArray makeWav(const QVector<float>& samples) {
    const int dataSize = static_cast<int>(samples.size()) * 2;

    QByteArray wav;
    wav.reserve(44 + dataSize);

    auto writeLE32 = [&wav](quint32 value) {
        char buf[4] = {
            static_cast<char>(value & 0xff),
            static_cast<char>((value >> 8) & 0xff),
            static_cast<char>((value >> 16) & 0xff),
            static_cast<char>((value >> 24) & 0xff)
        };
        wav.append(buf, 4);
    };
    auto writeLE16 = [&wav](quint16 value) {
        char buf[2] = {
            static_cast<char>(value & 0xff),
            static_cast<char>((value >> 8) & 0xff)
        };
        wav.append(buf, 2);
    };

    // RIFF 头
    wav.append("RIFF");
    writeLE32(static_cast<quint32>(36 + dataSize));
    wav.append("WAVE");

    // fmt 块
    wav.append("fmt ");
    writeLE32(16);
    writeLE16(1);            // PCM
    writeLE16(1);            // 单声道
    writeLE32(kSampleRate);
    writeLE32(kSampleRate * 2); // 字节率
    writeLE16(2);            // 块对齐
    writeLE16(16);           // 位深

    // data 块
    wav.append("data");
    writeLE32(static_cast<quint32>(dataSize));
    for (float sample : samples) {
        double clamped = std::max(-1.0, std::min(1.0, static_cast<double>(sample)));
        qint16 v = static_cast<qint16>(std::lround(clamped * 32767.0));
        char lo = static_cast<char>(v & 0xff);
        char hi = static_cast<char>((v >> 8) & 0xff);
        wav.append(lo);
        wav.append(hi);
    }

    return wav;
}

quint64 hashBytes(const QByteArray& data) {
    quint64 h = 1469598103934665603ULL;
    for (char c : data) {
        h ^= static_cast<unsigned char>(c);
        h *= 1099511628211ULL;
    }
    return h;
}

} // namespace

SoundPlayer::SoundPlayer() {
    m_placeBlack.setLoopCount(1);
    m_placeWhite.setLoopCount(1);
    m_win.setLoopCount(1);
    m_placeBlack.setVolume(1.0);
    m_placeWhite.setVolume(1.0);
    m_win.setVolume(1.0);
}

SoundPlayer::~SoundPlayer() = default;

QByteArray SoundPlayer::synthesize(const QVector<ToneSpec>& tones,
                                   double totalDuration) {
    int totalSamples = static_cast<int>(totalDuration * kSampleRate) + 1;
    if (totalSamples <= 0) {
        return {};
    }

    QVector<float> mix(totalSamples, 0.0f);

    for (const ToneSpec& spec : tones) {
        int startSample = static_cast<int>(spec.delay * kSampleRate);
        int count = static_cast<int>(spec.duration * kSampleRate);
        if (count <= 0) {
            continue;
        }

        LowPass lp;
        lp.setCutoff(spec.filterFreq);

        double phase = 0.0;
        const double attackSamples = 0.008 * kSampleRate;
        for (int i = 0; i < count; ++i) {
            int idx = startSample + i;
            if (idx < 0 || idx >= totalSamples) {
                continue;
            }
            const double t = static_cast<double>(i) / count;
            const double freq = spec.freq + (spec.endFreq - spec.freq) * t;
            phase += 2.0 * kPi * freq / kSampleRate;

            double s = waveform(phase, spec.type);
            s = lp.process(s);

            const double attack = std::min(1.0, static_cast<double>(i) / attackSamples);
            const double decay = std::exp(-4.0 * t);
            const double env = attack * decay;
            mix[idx] += static_cast<float>(s * env * spec.volume);
        }
    }

    // 归一化，避免削波
    float peak = 0.0f;
    for (float v : mix) {
        peak = std::max(peak, std::abs(v));
    }
    if (peak > 1.0f) {
        for (float& v : mix) {
            v /= peak;
        }
    }

    return makeWav(mix);
}

QString SoundPlayer::ensureToneFile(const QString& key,
                                    const QVector<ToneSpec>& tones,
                                    double totalDuration) const {
    QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                  + "/sounds";
    QDir().mkpath(dir);

    const QByteArray data = synthesize(tones, totalDuration);
    if (data.isEmpty()) {
        return {};
    }

    const quint64 crc = hashBytes(data);
    const QString path = dir + "/" + key + "_"
                         + QString::number(crc, 16).rightJustified(16, '0') + ".wav";

    if (!QFile::exists(path)) {
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly)) {
            return {};
        }
        file.write(data);
        file.close();
    }
    return path;
}

void SoundPlayer::configure(const AppCfg::AppSettings& settings) {
    m_muted = !settings.soundEnabled;

    QString placeBlackPath;
    QString placeWhitePath;
    QString winPath;

    // 落子音效
    switch (settings.placeSound) {
        case AppCfg::PlaceSound::Wood:
            placeBlackPath = ensureToneFile("place_wood_b",
                {{240, 90, "triangle", 0.16, 0.45, 0.0, 900}}, 0.25);
            placeWhitePath = ensureToneFile("place_wood_w",
                {{320, 120, "triangle", 0.16, 0.45, 0.0, 900}}, 0.25);
            break;
        case AppCfg::PlaceSound::Click:
            placeBlackPath = ensureToneFile("place_click",
                {{880, 260, "triangle", 0.10, 0.28, 0.0, 2000}}, 0.16);
            placeWhitePath = placeBlackPath;
            break;
        case AppCfg::PlaceSound::Bubble:
            placeBlackPath = ensureToneFile("place_bubble",
                {{220, 760, "sine", 0.14, 0.28, 0.0, 1400}}, 0.20);
            placeWhitePath = placeBlackPath;
            break;
        case AppCfg::PlaceSound::Custom:
            if (!settings.customPlaceAudioPath.isEmpty()) {
                placeBlackPath = settings.customPlaceAudioPath;
                placeWhitePath = settings.customPlaceAudioPath;
            }
            break;
        case AppCfg::PlaceSound::Silent:
            break;
    }

    // 获胜音效
    switch (settings.winSound) {
        case AppCfg::WinSound::Chord: {
            const double freqs[4] = {392.0, 523.25, 659.25, 783.99};
            QVector<ToneSpec> tones;
            for (int i = 0; i < 4; ++i) {
                tones.push_back({freqs[i], freqs[i], "sine", 0.30, 0.24,
                                 i * 0.09, 0.0});
            }
            winPath = ensureToneFile("win_chord", tones, 0.30 + 3 * 0.09 + 0.05);
            break;
        }
        case AppCfg::WinSound::Rising: {
            const double freqs[5] = {523.25, 659.25, 783.99, 1046.5, 1318.51};
            QVector<ToneSpec> tones;
            for (int i = 0; i < 5; ++i) {
                tones.push_back({freqs[i], freqs[i], "triangle", 0.20, 0.22,
                                 i * 0.07, 0.0});
            }
            winPath = ensureToneFile("win_rising", tones, 0.20 + 4 * 0.07 + 0.05);
            break;
        }
        case AppCfg::WinSound::Sci: {
            const QVector<ToneSpec> tones = {
                {160.0, 1150.0, "sawtooth", 0.40, 0.18, 0.0, 1500.0},
                {720.0, 280.0, "sine", 0.30, 0.18, 0.12, 1000.0}
            };
            winPath = ensureToneFile("win_sci", tones, 0.55);
            break;
        }
        case AppCfg::WinSound::Custom:
            if (!settings.customWinAudioPath.isEmpty()) {
                winPath = settings.customWinAudioPath;
            }
            break;
        case AppCfg::WinSound::Silent:
            break;
    }

    m_placeBlack.setSource(QUrl::fromLocalFile(placeBlackPath));
    m_placeWhite.setSource(QUrl::fromLocalFile(placeWhitePath));
    m_win.setSource(QUrl::fromLocalFile(winPath));
}

void SoundPlayer::playPlaceSound(int player) {
    if (m_muted) {
        return;
    }
    (player == 0 ? m_placeBlack : m_placeWhite).play();
}

void SoundPlayer::playWinSound() {
    if (m_muted) {
        return;
    }
    m_win.play();
}
