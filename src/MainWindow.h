#pragma once

#include <QFrame>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>
#include <QPoint>
#include <QPushButton>

#include <QFutureWatcher>
#include <QPair>

#include "AppSettings.h"
#include "GomokuCore.h"
#include "NetworkManager.h"

class QButtonGroup;
class QComboBox;
class QLineEdit;
class QRadioButton;
class QSpinBox;

namespace Gomoku {

class BoardWidget;
class SoundManager;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

private slots:
    void onUndo();
    void onRestart();
    void onSoundToggle();
    void onSkinDialog();
    void onModeChanged();
    void onNetAction();
    void onAiFinished(int token);

private:
    enum class Mode {
        HumanHuman,
        HumanAI,
        Network
    };

    void setupUi();
    QWidget* buildHeader();
    QWidget* buildModeBar();
    QWidget* buildSidebar();
    QFrame* buildPlayerCard(bool black, QLabel** nameLabel, QLabel** tagLabel);
    QFrame* buildCard(const QString& title, QWidget* content);
    QPixmap makeStonePixmap(int piece, int size) const;

    void setMode(Mode mode);
    void restartCurrent(bool notifyRemote);
    void doPlace(int row, int col, bool aiMove = false, bool remote = false);
    void scheduleAi();
    void cancelAi();
    void startNetwork();
    void stopNetwork();
    void updateStatus();
    void updateModePanel();
    void applySettings();
    void loadSettings();
    void saveSettings() const;
    void setSoundButtonUi();
    void setBoardInteraction();
    void showNetMessage(const QString& text, bool error = false);

    Piece humanPiece() const {
        return humanIsBlack_ ? Piece::Black : Piece::White;
    }
    Piece aiPiece() const {
        return humanIsBlack_ ? Piece::White : Piece::Black;
    }
    bool canHumanInput() const;

    GameEngine game_;
    AppSettings settings_;
    NetworkManager network_;

    BoardWidget* board_ = nullptr;
    SoundManager* sounds_ = nullptr;

    QPushButton* soundButton_ = nullptr;
    QPushButton* skinButton_ = nullptr;
    QLabel* statusDot_ = nullptr;
    QLabel* statusText_ = nullptr;
    QLabel* moveText_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QPushButton* restartButton_ = nullptr;

    QLabel* blackName_ = nullptr;
    QLabel* blackTag_ = nullptr;
    QLabel* whiteName_ = nullptr;
    QLabel* whiteTag_ = nullptr;
    QLabel* blackIcon_ = nullptr;
    QLabel* whiteIcon_ = nullptr;
    QFrame* blackCard_ = nullptr;
    QFrame* whiteCard_ = nullptr;

    QPushButton* modeHuman_ = nullptr;
    QPushButton* modeAI_ = nullptr;
    QPushButton* modeNet_ = nullptr;

    QWidget* aiOptions_ = nullptr;
    QRadioButton* aiBlack_ = nullptr;
    QRadioButton* aiWhite_ = nullptr;
    QComboBox* difficultyCombo_ = nullptr;

    QWidget* netOptions_ = nullptr;
    QRadioButton* netHost_ = nullptr;
    QRadioButton* netClient_ = nullptr;
    QLineEdit* netAddress_ = nullptr;
    QSpinBox* netPort_ = nullptr;
    QPushButton* netAction_ = nullptr;
    QLabel* netStatus_ = nullptr;

    Mode mode_ = Mode::HumanHuman;
    bool humanIsBlack_ = true;
    bool aiThinking_ = false;
    bool netConnected_ = false;
    int aiToken_ = 0;
    QFutureWatcher<QPair<int, int>>* aiWatcher_ = nullptr;

    Piece myColor_ = Piece::Black;
    QString aiDifficulty_ = "normal";
};

} // namespace Gomoku
