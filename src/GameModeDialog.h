#ifndef GAMEMODEDIALOG_H
#define GAMEMODEDIALOG_H

#include <QDialog>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QCheckBox>
#include <QString>
#include <QtGlobal>

namespace Gomoku {

/// @brief 玩家颜色枚举
enum class PlayerColor {
    Black,
    White
};

/// @brief AI 难度枚举
enum class AIDifficultyLevel {
    Easy,
    Normal,
    Hard
};

/// @brief 游戏模式配置
struct GameConfig {
    bool isHumanVsAI = false;
    PlayerColor humanColor = PlayerColor::Black;
    AIDifficultyLevel aiDifficulty = AIDifficultyLevel::Normal;

    /// @brief 是否网络对战
    bool isNetwork = false;
    /// @brief true=创建房间(主机)，false=加入房间(客户端)
    bool networkIsHost = false;
    /// @brief 客户端连接的主机地址
    QString networkHost = "127.0.0.1";
    /// @brief 端口
    quint16 networkPort = 12345;

    /// @brief 是否启用每步计时
    bool enableTimer = true;
    /// @brief 每步允许时间（秒）
    int moveTimeSeconds = 30;
};

/// @brief 新游戏模式选择对话框
class GameModeDialog : public QDialog {
    Q_OBJECT

public:
    explicit GameModeDialog(QWidget *parent = nullptr);
    ~GameModeDialog() override = default;

    GameConfig getConfig() const;

private:
    void setupUI();
    void updateOptions();
    void updateNetworkOptions();

    QRadioButton* humanVsHumanRadio;
    QRadioButton* humanVsAIRadio;
    QRadioButton* networkRadio;

    QGroupBox* colorGroupBox;
    QRadioButton* blackRadio;
    QRadioButton* whiteRadio;

    QGroupBox* difficultyGroupBox;
    QComboBox* difficultyCombo;

    QGroupBox* networkGroupBox;
    QRadioButton* hostRadio;
    QRadioButton* clientRadio;
    QLineEdit* hostAddressEdit;
    QSpinBox* portSpinBox;

    QGroupBox* timerGroupBox;
    QCheckBox* timerCheckBox;
    QSpinBox* timerSecondsSpinBox;

    QPushButton* okButton;
    QPushButton* cancelButton;
};

} // namespace Gomoku

#endif // GAMEMODEDIALOG_H
