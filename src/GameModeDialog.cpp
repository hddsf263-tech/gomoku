#include "GameModeDialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>

namespace Gomoku {

GameModeDialog::GameModeDialog(QWidget *parent)
    : QDialog(parent)
{
    setupUI();
}

void GameModeDialog::setupUI() {
    setWindowTitle("新游戏");
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(15);

    auto* modeGroup = new QGroupBox("游戏模式", this);
    auto* modeLayout = new QVBoxLayout(modeGroup);

    humanVsHumanRadio = new QRadioButton("双人对战", this);
    humanVsAIRadio = new QRadioButton("人机对战", this);

    modeLayout->addWidget(humanVsHumanRadio);
    modeLayout->addWidget(humanVsAIRadio);

    humanVsHumanRadio->setChecked(true);

    mainLayout->addWidget(modeGroup);

    colorGroupBox = new QGroupBox("玩家颜色", this);
    auto* colorLayout = new QVBoxLayout(colorGroupBox);

    blackRadio = new QRadioButton("执黑（先行）", this);
    whiteRadio = new QRadioButton("执白（后手）", this);

    colorLayout->addWidget(blackRadio);
    colorLayout->addWidget(whiteRadio);

    blackRadio->setChecked(true);
    colorGroupBox->setEnabled(false);

    mainLayout->addWidget(colorGroupBox);

    difficultyGroupBox = new QGroupBox("AI 难度", this);
    auto* diffLayout = new QVBoxLayout(difficultyGroupBox);

    difficultyCombo = new QComboBox(this);
    difficultyCombo->addItem("简单 - 快速思考", static_cast<int>(AIDifficultyLevel::Easy));
    difficultyCombo->addItem("标准 - 中等难度", static_cast<int>(AIDifficultyLevel::Normal));
    difficultyCombo->addItem("困难 - 深度计算", static_cast<int>(AIDifficultyLevel::Hard));

    diffLayout->addWidget(difficultyCombo);
    difficultyGroupBox->setEnabled(false);

    mainLayout->addWidget(difficultyGroupBox);

    mainLayout->addStretch();

    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);

    okButton = buttonBox->button(QDialogButtonBox::Ok);
    cancelButton = buttonBox->button(QDialogButtonBox::Cancel);

    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);

    mainLayout->addWidget(buttonBox);

    connect(humanVsHumanRadio, &QRadioButton::toggled, this, [this]() {
        updateOptions();
    });
    connect(humanVsAIRadio, &QRadioButton::toggled, this, [this]() {
        updateOptions();
    });
}

void GameModeDialog::updateOptions() {
    bool isHumanVsAI = humanVsAIRadio->isChecked();
    colorGroupBox->setEnabled(isHumanVsAI);
    difficultyGroupBox->setEnabled(isHumanVsAI);
}

GameConfig GameModeDialog::getConfig() const {
    GameConfig config;

    config.isHumanVsAI = humanVsAIRadio->isChecked();
    config.humanColor =
        whiteRadio->isChecked() ? PlayerColor::White : PlayerColor::Black;

    int diffIndex = difficultyCombo->currentIndex();
    config.aiDifficulty = static_cast<AIDifficultyLevel>(diffIndex);

    return config;
}

} // namespace Gomoku