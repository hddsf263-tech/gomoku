#ifndef GAMEMODEDIALOG_H
#define GAMEMODEDIALOG_H

#include <QDialog>
#include <QRadioButton>
#include <QComboBox>
#include <QPushButton>
#include <QVBoxLayout>
#include <QGroupBox>

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

    QRadioButton* humanVsHumanRadio;
    QRadioButton* humanVsAIRadio;

    QGroupBox* colorGroupBox;
    QRadioButton* blackRadio;
    QRadioButton* whiteRadio;

    QGroupBox* difficultyGroupBox;
    QComboBox* difficultyCombo;

    QPushButton* okButton;
    QPushButton* cancelButton;
};

} // namespace Gomoku

#endif // GAMEMODEDIALOG_H