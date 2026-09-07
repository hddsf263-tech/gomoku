#pragma once

#include <QDialog>

#include <QList>
#include <QString>

#include "AppSettings.h"

class QLabel;
class QPushButton;
class QVBoxLayout;

namespace Gomoku {

class SkinDialog : public QDialog {
    Q_OBJECT

public:
    explicit SkinDialog(AppSettings* settings, QWidget* parent = nullptr);

signals:
    void applied();

private:
    struct ChoiceGroup {
        QString title;
        QString current;
        QStringList values;
        QList<QPushButton*> buttons;
        QString AppSettings::* member = nullptr;
    };

    void buildUi();
    QPushButton* makeChoiceButton(const QString& label, const QString& value);
    void addChoiceSection(const QString& title,
                          const QStringList& labels,
                          const QStringList& values,
                          QString AppSettings::* target);
    void addUploadRow(const QString& title, const QString& buttonText,
                      QString AppSettings::* target,
                      const QString& filter);
    void refreshChoices();
    void chooseFile(QString AppSettings::* target, const QString& filter);

    AppSettings* settings_;
    QVBoxLayout* contentLayout_ = nullptr;
    QList<ChoiceGroup> groups_;
    QList<QPushButton*> resetTargets_;
};

} // namespace Gomoku
