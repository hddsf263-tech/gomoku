#include "GameModeDialog.h"
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>

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
    networkRadio = new QRadioButton("网络对战（双设备联机）", this);

    modeLayout->addWidget(humanVsHumanRadio);
    modeLayout->addWidget(humanVsAIRadio);
    modeLayout->addWidget(networkRadio);

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

    networkGroupBox = new QGroupBox("网络设置", this);
    auto* netLayout = new QVBoxLayout(networkGroupBox);

    hostRadio = new QRadioButton("创建房间（主机 · 执黑）", this);
    clientRadio = new QRadioButton("加入房间（客户端 · 执白）", this);
    hostRadio->setChecked(true);

    netLayout->addWidget(hostRadio);
    netLayout->addWidget(clientRadio);

    auto* addrRow = new QHBoxLayout;
    addrRow->addWidget(new QLabel("主机地址：", this));
    hostAddressEdit = new QLineEdit("127.0.0.1", this);
    addrRow->addWidget(hostAddressEdit, 1);
    netLayout->addLayout(addrRow);

    auto* portRow = new QHBoxLayout;
    portRow->addWidget(new QLabel("端口：", this));
    portSpinBox = new QSpinBox(this);
    portSpinBox->setRange(1, 65535);
    portSpinBox->setValue(12345);
    portRow->addWidget(portSpinBox, 1);
    netLayout->addLayout(portRow);

    networkGroupBox->setEnabled(false);
    mainLayout->addWidget(networkGroupBox);

    timerGroupBox = new QGroupBox("计时设置", this);
    auto* timerLayout = new QVBoxLayout(timerGroupBox);

    timerCheckBox = new QCheckBox("开启计时（超时判负）", this);
    timerCheckBox->setChecked(true);
    timerLayout->addWidget(timerCheckBox);

    auto* timerRow = new QHBoxLayout;
    timerRow->addWidget(new QLabel("每步时间：", this));
    timerSecondsSpinBox = new QSpinBox(this);
    timerSecondsSpinBox->setRange(5, 600);
    timerSecondsSpinBox->setValue(30);
    timerSecondsSpinBox->setSuffix(" 秒");
    timerRow->addWidget(timerSecondsSpinBox, 1);
    timerLayout->addLayout(timerRow);

    mainLayout->addWidget(timerGroupBox);

    connect(timerCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
        timerSecondsSpinBox->setEnabled(checked);
    });

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
    connect(networkRadio, &QRadioButton::toggled, this, [this]() {
        updateOptions();
    });
    connect(hostRadio, &QRadioButton::toggled, this, [this]() {
        updateNetworkOptions();
    });
    connect(clientRadio, &QRadioButton::toggled, this, [this]() {
        updateNetworkOptions();
    });
}

void GameModeDialog::updateOptions() {
    bool isHumanVsAI = humanVsAIRadio->isChecked();
    colorGroupBox->setEnabled(isHumanVsAI);
    difficultyGroupBox->setEnabled(isHumanVsAI);
    networkGroupBox->setEnabled(networkRadio->isChecked());
}

void GameModeDialog::updateNetworkOptions() {
    bool isHost = hostRadio->isChecked();
    hostAddressEdit->setEnabled(!isHost);
}

GameConfig GameModeDialog::getConfig() const {
    GameConfig config;

    config.isHumanVsAI = humanVsAIRadio->isChecked();
    config.humanColor =
        whiteRadio->isChecked() ? PlayerColor::White : PlayerColor::Black;

    int diffIndex = difficultyCombo->currentIndex();
    config.aiDifficulty = static_cast<AIDifficultyLevel>(diffIndex);

    config.isNetwork = networkRadio->isChecked();
    config.networkIsHost = hostRadio->isChecked();
    config.networkHost = hostAddressEdit->text().trimmed();
    config.networkPort = static_cast<quint16>(portSpinBox->value());

    config.enableTimer = timerCheckBox->isChecked();
    config.moveTimeSeconds = timerSecondsSpinBox->value();

    return config;
}

} // namespace Gomoku
