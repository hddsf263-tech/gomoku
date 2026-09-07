#ifndef SETTINGSDIALOG_H
#define SETTINGSDIALOG_H

#include <QDialog>

#include "AppSettings.h"

class QComboBox;
class QCheckBox;
class QLineEdit;
class QPushButton;

/// @brief 外观与音效设置对话框
class SettingsDialog : public QDialog {
public:
    explicit SettingsDialog(const AppCfg::AppSettings& settings,
                            QWidget *parent = nullptr);
    ~SettingsDialog() override = default;

    /// @brief 读取编辑后的设置
    AppCfg::AppSettings getSettings() const;

private:
    void setupUI();
    void applySettings();
    void browseFile(QLineEdit* edit, const QString& filter);
    void clearFile(QLineEdit* edit);

    AppCfg::AppSettings m_settings;

    QComboBox* boardSkinCombo;
    QComboBox* pieceSkinCombo;
    QComboBox* placeEffectCombo;
    QComboBox* winEffectCombo;
    QComboBox* placeSoundCombo;
    QComboBox* winSoundCombo;

    QLineEdit* boardImageEdit;
    QLineEdit* blackImageEdit;
    QLineEdit* whiteImageEdit;
    QLineEdit* customPlaceAudioEdit;
    QLineEdit* customWinAudioEdit;

    QPushButton* boardImageBtn;
    QPushButton* blackImageBtn;
    QPushButton* whiteImageBtn;
    QPushButton* customPlaceAudioBtn;
    QPushButton* customWinAudioBtn;

    QPushButton* boardImageClearBtn;
    QPushButton* blackImageClearBtn;
    QPushButton* whiteImageClearBtn;
    QPushButton* customPlaceAudioClearBtn;
    QPushButton* customWinAudioClearBtn;

    QCheckBox* soundCheckBox;
};

#endif // SETTINGSDIALOG_H
