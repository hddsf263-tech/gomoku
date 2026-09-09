#pragma once

#include <QFrame>
#include <QLabel>
#include <QMainWindow>
#include <QPixmap>
#include <QPoint>
#include <QPushButton>

#include <QFutureWatcher>
#include <QPair>

#include <memory>
#include <vector>

#include "AppSettings.h"
#include "GomokuCore.h"
#include "OnlineSession.h"

class QButtonGroup;
class QComboBox;
class QLineEdit;
class QRadioButton;
class QTextEdit;
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

    void onNetStateChanged(OnlineState state);
    void onNetColorAssigned(Piece color);
    void onNetSessionStarted();
    void onNetMoveCommitted(int row, int col, Piece piece, GameStatus status);
    void onNetGameStatusChanged(GameStatus status, const std::vector<GameMove>& line, bool online);
    void onNetMoveRejected(const QString& reason);
    void onNetRematchRequested();
    void onNetRematchAccepted();
    void onNetRematchDeclined();
    void onNetOpponentDisconnected();
    void onNetError(const QString& text);
    void onResign();
    void onNetResigned(Piece resigner, GameStatus status);
    void onNetTimeUpdated(qint64 blackRemainingMs, qint64 whiteRemainingMs, Piece currentPlayer);
    void onNetChatMessage(Piece sender, const QString& text, qint64 timestampMs);
    void onNetChatSendFailed(const QString& reason);
    void onSendChat();

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
    QString formatTime(qint64 ms) const;
    void updateTimerDisplay(qint64 blackMs, qint64 whiteMs, Piece current);
    void appendChatRecord(const QString& senderLabel, const QString& text);
    void setChatEnabled(bool enabled);

    Piece humanPiece() const {
        return humanIsBlack_ ? Piece::Black : Piece::White;
    }
    Piece aiPiece() const {
        return humanIsBlack_ ? Piece::White : Piece::Black;
    }
    bool canHumanInput() const;
    bool isNetActive() const {
        return session_ && session_->isConnected();
    }

    GameEngine game_;
    AppSettings settings_;
    std::unique_ptr<OnlineSession> session_;

    BoardWidget* board_ = nullptr;
    SoundManager* sounds_ = nullptr;

    QPushButton* soundButton_ = nullptr;
    QPushButton* skinButton_ = nullptr;
    QLabel* statusDot_ = nullptr;
    QLabel* statusText_ = nullptr;
    QLabel* moveText_ = nullptr;
    QPushButton* undoButton_ = nullptr;
    QPushButton* restartButton_ = nullptr;
    QPushButton* resignButton_ = nullptr;

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
    QComboBox* timeLimitCombo_ = nullptr;
    QFrame* timeCard_ = nullptr;
    QLabel* blackTime_ = nullptr;
    QLabel* whiteTime_ = nullptr;
    QWidget* chatPanel_ = nullptr;
    QTextEdit* chatView_ = nullptr;
    QLineEdit* chatInput_ = nullptr;
    QPushButton* chatSend_ = nullptr;

    Mode mode_ = Mode::HumanHuman;
    bool humanIsBlack_ = true;
    bool aiThinking_ = false;
    int aiToken_ = 0;
    QFutureWatcher<QPair<int, int>>* aiWatcher_ = nullptr;

    Piece myColor_ = Piece::Black;
    QString aiDifficulty_ = "normal";
};

} // namespace Gomoku
