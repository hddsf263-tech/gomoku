#include "SettingsDialog.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QFileDialog>
#include <QFileInfo>
#include <QFormLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

namespace {

const char* kImageFilter = "图片文件 (*.png *.jpg *.jpeg *.bmp *.gif);;所有文件 (*.*)";
const char* kAudioFilter = "音频文件 (*.wav *.mp3 *.ogg *.flac);;所有文件 (*.*)";

} // namespace

SettingsDialog::SettingsDialog(const AppCfg::AppSettings& settings,
                               QWidget *parent)
    : QDialog(parent)
    , m_settings(settings)
    , boardSkinCombo(nullptr)
    , pieceSkinCombo(nullptr)
    , placeEffectCombo(nullptr)
    , winEffectCombo(nullptr)
    , placeSoundCombo(nullptr)
    , winSoundCombo(nullptr)
    , boardImageEdit(nullptr)
    , blackImageEdit(nullptr)
    , whiteImageEdit(nullptr)
    , customPlaceAudioEdit(nullptr)
    , customWinAudioEdit(nullptr)
    , boardImageBtn(nullptr)
    , blackImageBtn(nullptr)
    , whiteImageBtn(nullptr)
    , customPlaceAudioBtn(nullptr)
    , customWinAudioBtn(nullptr)
    , boardImageClearBtn(nullptr)
    , blackImageClearBtn(nullptr)
    , whiteImageClearBtn(nullptr)
    , customPlaceAudioClearBtn(nullptr)
    , customWinAudioClearBtn(nullptr)
    , soundCheckBox(nullptr)
{
    setupUI();
}

void SettingsDialog::setupUI() {
    setWindowTitle("外观与音效");
    setModal(true);

    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // ===== 棋盘皮肤 =====
    auto* boardGroup = new QGroupBox("棋盘皮肤", this);
    auto* boardForm = new QFormLayout(boardGroup);

    boardSkinCombo = new QComboBox(this);
    boardSkinCombo->addItem("翡翠", static_cast<int>(AppCfg::BoardSkin::Jade));
    boardSkinCombo->addItem("原木", static_cast<int>(AppCfg::BoardSkin::Wood));
    boardSkinCombo->addItem("墨玉", static_cast<int>(AppCfg::BoardSkin::Ink));
    boardSkinCombo->addItem("红木", static_cast<int>(AppCfg::BoardSkin::Rose));
    boardSkinCombo->addItem("自定义图片", static_cast<int>(AppCfg::BoardSkin::Custom));
    boardForm->addRow("配色", boardSkinCombo);

    auto* boardImageRow = new QHBoxLayout;
    boardImageEdit = new QLineEdit(this);
    boardImageEdit->setReadOnly(true);
    boardImageEdit->setPlaceholderText("未选择图片");
    boardImageBtn = new QPushButton("浏览", this);
    boardImageClearBtn = new QPushButton("清除", this);
    boardImageRow->addWidget(boardImageEdit, 1);
    boardImageRow->addWidget(boardImageBtn);
    boardImageRow->addWidget(boardImageClearBtn);
    boardForm->addRow("背景图片", boardImageRow);

    connect(boardImageBtn, &QPushButton::clicked, this,
            [this]() { browseFile(boardImageEdit, kImageFilter); });
    connect(boardImageClearBtn, &QPushButton::clicked, this,
            [this]() { clearFile(boardImageEdit); });
    mainLayout->addWidget(boardGroup);

    // ===== 棋子皮肤 =====
    auto* pieceGroup = new QGroupBox("棋子皮肤", this);
    auto* pieceForm = new QFormLayout(pieceGroup);

    pieceSkinCombo = new QComboBox(this);
    pieceSkinCombo->addItem("经典", static_cast<int>(AppCfg::PieceSkin::Classic));
    pieceSkinCombo->addItem("玉石", static_cast<int>(AppCfg::PieceSkin::Jade));
    pieceSkinCombo->addItem("曜石", static_cast<int>(AppCfg::PieceSkin::Onyx));
    pieceSkinCombo->addItem("琥珀", static_cast<int>(AppCfg::PieceSkin::Amber));
    pieceSkinCombo->addItem("自定义图片", static_cast<int>(AppCfg::PieceSkin::Custom));
    pieceForm->addRow("配色", pieceSkinCombo);

    auto addImageRow = [this, &pieceForm](const QString& label,
                                          QLineEdit*& edit,
                                          QPushButton*& btn,
                                          QPushButton*& clearBtn) {
        auto* row = new QHBoxLayout;
        edit = new QLineEdit(this);
        edit->setReadOnly(true);
        edit->setPlaceholderText("未选择图片");
        btn = new QPushButton("浏览", this);
        clearBtn = new QPushButton("清除", this);
        row->addWidget(edit, 1);
        row->addWidget(btn);
        row->addWidget(clearBtn);
        pieceForm->addRow(label, row);
        connect(btn, &QPushButton::clicked, this,
                [this, edit]() { browseFile(edit, kImageFilter); });
        connect(clearBtn, &QPushButton::clicked, this,
                [this, edit]() { clearFile(edit); });
    };

    addImageRow("黑棋图片", blackImageEdit, blackImageBtn, blackImageClearBtn);
    addImageRow("白棋图片", whiteImageEdit, whiteImageBtn, whiteImageClearBtn);
    mainLayout->addWidget(pieceGroup);

    // ===== 特效 =====
    auto* vfxGroup = new QGroupBox("特效", this);
    auto* vfxForm = new QFormLayout(vfxGroup);

    placeEffectCombo = new QComboBox(this);
    placeEffectCombo->addItem("扩散光环", static_cast<int>(AppCfg::PlaceEffect::Ring));
    placeEffectCombo->addItem("星光粒子", static_cast<int>(AppCfg::PlaceEffect::Spark));
    placeEffectCombo->addItem("关闭", static_cast<int>(AppCfg::PlaceEffect::None));
    vfxForm->addRow("落子特效", placeEffectCombo);

    winEffectCombo = new QComboBox(this);
    winEffectCombo->addItem("金色脉冲", static_cast<int>(AppCfg::WinEffect::Pulse));
    winEffectCombo->addItem("彩带爆裂", static_cast<int>(AppCfg::WinEffect::Burst));
    winEffectCombo->addItem("关闭", static_cast<int>(AppCfg::WinEffect::None));
    vfxForm->addRow("获胜特效", winEffectCombo);
    mainLayout->addWidget(vfxGroup);

    // ===== 音效 =====
    auto* soundGroup = new QGroupBox("音效", this);
    auto* soundForm = new QFormLayout(soundGroup);

    placeSoundCombo = new QComboBox(this);
    placeSoundCombo->addItem("木声", static_cast<int>(AppCfg::PlaceSound::Wood));
    placeSoundCombo->addItem("清脆", static_cast<int>(AppCfg::PlaceSound::Click));
    placeSoundCombo->addItem("水滴", static_cast<int>(AppCfg::PlaceSound::Bubble));
    placeSoundCombo->addItem("自定义", static_cast<int>(AppCfg::PlaceSound::Custom));
    placeSoundCombo->addItem("静音", static_cast<int>(AppCfg::PlaceSound::Silent));
    soundForm->addRow("落子音效", placeSoundCombo);

    auto* placeAudioRow = new QHBoxLayout;
    customPlaceAudioEdit = new QLineEdit(this);
    customPlaceAudioEdit->setReadOnly(true);
    customPlaceAudioEdit->setPlaceholderText("未选择音频");
    customPlaceAudioBtn = new QPushButton("浏览", this);
    customPlaceAudioClearBtn = new QPushButton("清除", this);
    placeAudioRow->addWidget(customPlaceAudioEdit, 1);
    placeAudioRow->addWidget(customPlaceAudioBtn);
    placeAudioRow->addWidget(customPlaceAudioClearBtn);
    soundForm->addRow("自定义落子音", placeAudioRow);
    connect(customPlaceAudioBtn, &QPushButton::clicked, this,
            [this]() { browseFile(customPlaceAudioEdit, kAudioFilter); });
    connect(customPlaceAudioClearBtn, &QPushButton::clicked, this,
            [this]() { clearFile(customPlaceAudioEdit); });

    winSoundCombo = new QComboBox(this);
    winSoundCombo->addItem("和弦", static_cast<int>(AppCfg::WinSound::Chord));
    winSoundCombo->addItem("欢快", static_cast<int>(AppCfg::WinSound::Rising));
    winSoundCombo->addItem("科幻", static_cast<int>(AppCfg::WinSound::Sci));
    winSoundCombo->addItem("自定义", static_cast<int>(AppCfg::WinSound::Custom));
    winSoundCombo->addItem("静音", static_cast<int>(AppCfg::WinSound::Silent));
    soundForm->addRow("获胜音效", winSoundCombo);

    auto* winAudioRow = new QHBoxLayout;
    customWinAudioEdit = new QLineEdit(this);
    customWinAudioEdit->setReadOnly(true);
    customWinAudioEdit->setPlaceholderText("未选择音频");
    customWinAudioBtn = new QPushButton("浏览", this);
    customWinAudioClearBtn = new QPushButton("清除", this);
    winAudioRow->addWidget(customWinAudioEdit, 1);
    winAudioRow->addWidget(customWinAudioBtn);
    winAudioRow->addWidget(customWinAudioClearBtn);
    soundForm->addRow("自定义获胜音", winAudioRow);
    connect(customWinAudioBtn, &QPushButton::clicked, this,
            [this]() { browseFile(customWinAudioEdit, kAudioFilter); });
    connect(customWinAudioClearBtn, &QPushButton::clicked, this,
            [this]() { clearFile(customWinAudioEdit); });

    soundCheckBox = new QCheckBox("启用全局音效", this);
    soundForm->addRow("", soundCheckBox);

    mainLayout->addWidget(soundGroup);

    // ===== 操作按钮 =====
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    auto* resetBtn = buttonBox->addButton("恢复默认", QDialogButtonBox::ResetRole);
    mainLayout->addWidget(buttonBox);

    connect(buttonBox, &QDialogButtonBox::accepted, this, [this]() {
        applySettings();
        accept();
    });
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    connect(resetBtn, &QPushButton::clicked, this, [this]() {
        m_settings.resetToDefaults();
        // 回填 UI
        boardSkinCombo->setCurrentIndex(boardSkinCombo->findData(
            static_cast<int>(m_settings.boardSkin)));
        pieceSkinCombo->setCurrentIndex(pieceSkinCombo->findData(
            static_cast<int>(m_settings.pieceSkin)));
        placeEffectCombo->setCurrentIndex(placeEffectCombo->findData(
            static_cast<int>(m_settings.placeEffect)));
        winEffectCombo->setCurrentIndex(winEffectCombo->findData(
            static_cast<int>(m_settings.winEffect)));
        placeSoundCombo->setCurrentIndex(placeSoundCombo->findData(
            static_cast<int>(m_settings.placeSound)));
        winSoundCombo->setCurrentIndex(winSoundCombo->findData(
            static_cast<int>(m_settings.winSound)));
        boardImageEdit->clear();
        blackImageEdit->clear();
        whiteImageEdit->clear();
        customPlaceAudioEdit->clear();
        customWinAudioEdit->clear();
        soundCheckBox->setChecked(m_settings.soundEnabled);
    });

    // 初始回填
    boardSkinCombo->setCurrentIndex(boardSkinCombo->findData(
        static_cast<int>(m_settings.boardSkin)));
    pieceSkinCombo->setCurrentIndex(pieceSkinCombo->findData(
        static_cast<int>(m_settings.pieceSkin)));
    placeEffectCombo->setCurrentIndex(placeEffectCombo->findData(
        static_cast<int>(m_settings.placeEffect)));
    winEffectCombo->setCurrentIndex(winEffectCombo->findData(
        static_cast<int>(m_settings.winEffect)));
    placeSoundCombo->setCurrentIndex(placeSoundCombo->findData(
        static_cast<int>(m_settings.placeSound)));
    winSoundCombo->setCurrentIndex(winSoundCombo->findData(
        static_cast<int>(m_settings.winSound)));
    boardImageEdit->setText(m_settings.boardImagePath);
    blackImageEdit->setText(m_settings.blackImagePath);
    whiteImageEdit->setText(m_settings.whiteImagePath);
    customPlaceAudioEdit->setText(m_settings.customPlaceAudioPath);
    customWinAudioEdit->setText(m_settings.customWinAudioPath);
    soundCheckBox->setChecked(m_settings.soundEnabled);

    resize(460, 620);
}

void SettingsDialog::applySettings() {
    m_settings.boardSkin = static_cast<AppCfg::BoardSkin>(
        boardSkinCombo->currentData().toInt());
    m_settings.pieceSkin = static_cast<AppCfg::PieceSkin>(
        pieceSkinCombo->currentData().toInt());
    m_settings.placeEffect = static_cast<AppCfg::PlaceEffect>(
        placeEffectCombo->currentData().toInt());
    m_settings.winEffect = static_cast<AppCfg::WinEffect>(
        winEffectCombo->currentData().toInt());
    m_settings.placeSound = static_cast<AppCfg::PlaceSound>(
        placeSoundCombo->currentData().toInt());
    m_settings.winSound = static_cast<AppCfg::WinSound>(
        winSoundCombo->currentData().toInt());

    m_settings.boardImagePath = boardImageEdit->text().trimmed();
    m_settings.blackImagePath = blackImageEdit->text().trimmed();
    m_settings.whiteImagePath = whiteImageEdit->text().trimmed();
    m_settings.customPlaceAudioPath = customPlaceAudioEdit->text().trimmed();
    m_settings.customWinAudioPath = customWinAudioEdit->text().trimmed();
    m_settings.soundEnabled = soundCheckBox->isChecked();
}

AppCfg::AppSettings SettingsDialog::getSettings() const {
    return m_settings;
}

void SettingsDialog::browseFile(QLineEdit* edit, const QString& filter) {
    const QString path = QFileDialog::getOpenFileName(
        this, "选择文件", edit->text(), filter);
    if (!path.isEmpty()) {
        edit->setText(path);
    }
}

void SettingsDialog::clearFile(QLineEdit* edit) {
    edit->clear();
}
