#include "SkinDialog.h"

#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QScrollArea>
#include <QVBoxLayout>

#include <algorithm>

namespace Gomoku {

SkinDialog::SkinDialog(AppSettings* settings, QWidget* parent)
    : QDialog(parent)
    , settings_(settings)
{
    setWindowTitle("皮肤与特效");
    setModal(true);
    setMinimumSize(430, 620);
    resize(460, 700);
    buildUi();
}

void SkinDialog::buildUi() {
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(0, 0, 0, 0);

    auto* header = new QWidget(this);
    header->setFixedHeight(54);
    header->setStyleSheet("background:#ffffff; border-bottom:1px solid #e4e2da;");
    auto* headerLayout = new QHBoxLayout(header);
    headerLayout->setContentsMargins(18, 0, 12, 0);
    auto* title = new QLabel("皮肤与特效", header);
    title->setStyleSheet("font-size:17px; font-weight:700; color:#1f2623;");
    headerLayout->addWidget(title);
    headerLayout->addStretch();
    auto* closeButton = new QPushButton("×", header);
    closeButton->setFixedSize(34, 34);
    closeButton->setCursor(Qt::PointingHandCursor);
    closeButton->setStyleSheet(
        "QPushButton{border:1px solid #e2e0d8;border-radius:6px;"
        "background:#fff;font-size:20px;color:#1f2623;}"
        "QPushButton:hover{background:#f2f1ec;}");
    connect(closeButton, &QPushButton::clicked, this, &QDialog::accept);
    headerLayout->addWidget(closeButton);
    root->addWidget(header);

    auto* scroll = new QScrollArea(this);
    scroll->setWidgetResizable(true);
    scroll->setFrameShape(QFrame::NoFrame);
    auto* scrollContent = new QWidget(scroll);
    contentLayout_ = new QVBoxLayout(scrollContent);
    contentLayout_->setContentsMargins(16, 16, 16, 16);
    contentLayout_->setSpacing(14);
    scroll->setWidget(scrollContent);
    root->addWidget(scroll, 1);

    addChoiceSection("棋盘皮肤", { "翡翠", "原木", "墨玉", "红木" },
                     { "jade", "wood", "ink", "rose" },
                     &AppSettings::boardSkin);
    addUploadRow("自定义棋盘照片", "上传棋盘", &AppSettings::boardImage,
                 "图片 (*.png *.jpg *.jpeg *.bmp *.webp)");

    addChoiceSection("棋子皮肤", { "经典", "玉石", "曜石", "琥珀" },
                     { "classic", "jade", "onyx", "amber" },
                     &AppSettings::pieceSkin);
    addUploadRow("黑棋照片", "上传黑棋", &AppSettings::blackImage,
                 "图片 (*.png *.jpg *.jpeg *.bmp *.webp)");
    addUploadRow("白棋照片", "上传白棋", &AppSettings::whiteImage,
                 "图片 (*.png *.jpg *.jpeg *.bmp *.webp)");

    addChoiceSection("落子特效", { "光环", "星光", "关闭" },
                     { "ring", "spark", "none" },
                     &AppSettings::placeEffect);
    addChoiceSection("获胜特效", { "金色脉冲", "彩带爆裂", "关闭" },
                     { "pulse", "burst", "none" },
                     &AppSettings::winEffect);
    addChoiceSection("落子音效", { "木声", "清脆", "水滴", "静音", "自定义" },
                     { "wood", "click", "bubble", "silent", "custom" },
                     &AppSettings::placeSound);
    addChoiceSection("获胜音效", { "和弦", "欢快", "科幻", "静音", "自定义" },
                     { "chord", "rising", "sci", "silent", "custom" },
                     &AppSettings::winSound);
    addUploadRow("自定义落子音频", "导入落子音频", &AppSettings::customPlaceAudio,
                 "音频 (*.wav *.mp3 *.ogg *.m4a *.aac)");
    addUploadRow("自定义获胜音频", "导入获胜音频", &AppSettings::customWinAudio,
                 "音频 (*.wav *.mp3 *.ogg *.m4a *.aac)");

    auto* resetButton = new QPushButton("恢复默认设置", scrollContent);
    resetButton->setCursor(Qt::PointingHandCursor);
    resetButton->setStyleSheet(
        "QPushButton{height:40px;border:1px solid #d8d7cf;border-radius:7px;"
        "background:#fff;font-weight:600;color:#1f2623;}"
        "QPushButton:hover{background:#f2f1ec;}");
    connect(resetButton, &QPushButton::clicked, this, [this]() {
        *settings_ = AppSettings();
        refreshChoices();
        emit applied();
    });
    contentLayout_->addWidget(resetButton);
    contentLayout_->addStretch();
}

QPushButton* SkinDialog::makeChoiceButton(const QString& label,
                                          const QString& value) {
    auto* button = new QPushButton(label, this);
    button->setProperty("choiceValue", value);
    button->setCheckable(true);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(34);
    button->setStyleSheet(
        "QPushButton{border:1px solid #d8d7cf;border-radius:7px;"
        "background:#fff;color:#1f2623;font-size:13px;font-weight:600;}"
        "QPushButton:hover{background:#f2f1ec;}"
        "QPushButton:checked{background:#e7f0ea;border-color:#2e5d52;"
        "color:#2e5d52;}");
    return button;
}

void SkinDialog::addChoiceSection(const QString& title,
                                  const QStringList& labels,
                                  const QStringList& values,
                                  QString AppSettings::* target) {
    auto* sectionLabel = new QLabel(title, this);
    sectionLabel->setStyleSheet(
        "font-size:13px;font-weight:700;color:#6b746f;background:transparent;");
    contentLayout_->addWidget(sectionLabel);

    auto* row = new QWidget(this);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(8);

    ChoiceGroup group;
    group.title = title;
    group.current = settings_->*target;
    group.member = target;
    for (int i = 0; i < labels.size(); i++) {
        QPushButton* button = makeChoiceButton(labels[i], values[i]);
        group.values << values[i];
        group.buttons << button;
        layout->addWidget(button);
        connect(button, &QPushButton::clicked, this,
                [this, target, value = values[i]]() {
            settings_->*target = value;
            refreshChoices();
            emit applied();
        });
    }
    groups_.push_back(group);
    contentLayout_->addWidget(row);
}

void SkinDialog::addUploadRow(const QString& title, const QString& buttonText,
                              QString AppSettings::* target,
                              const QString& filter) {
    auto* row = new QWidget(this);
    auto* layout = new QHBoxLayout(row);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(10);
    auto* titleLabel = new QLabel(title, row);
    titleLabel->setStyleSheet("font-size:13px;color:#6b746f;font-weight:600;");
    auto* button = new QPushButton(buttonText, row);
    button->setCursor(Qt::PointingHandCursor);
    button->setMinimumHeight(34);
    button->setStyleSheet(
        "QPushButton{border:1px solid #d8d7cf;border-radius:7px;background:#fff;"
        "font-size:13px;font-weight:600;padding:0 10px;color:#1f2623;}"
        "QPushButton:hover{background:#f2f1ec;}");
    layout->addWidget(titleLabel);
    layout->addStretch();
    layout->addWidget(button);
    connect(button, &QPushButton::clicked, this,
            [this, target, filter]() { chooseFile(target, filter); });
    contentLayout_->addWidget(row);
}

void SkinDialog::refreshChoices() {
    for (ChoiceGroup& group : groups_) {
        const QString current = settings_->*group.member;
        for (QPushButton* button : group.buttons) {
            button->setChecked(button->property("choiceValue").toString() == current);
        }
    }
}

void SkinDialog::chooseFile(QString AppSettings::* target, const QString& filter) {
    const QString file = QFileDialog::getOpenFileName(this, "选择文件", QString(), filter);
    if (!file.isEmpty()) {
        settings_->*target = file;
        emit applied();
    }
}

} // namespace Gomoku
