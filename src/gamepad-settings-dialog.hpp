#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QPushButton>
#include <QTimer>
#include <QCheckBox>
#include <QGroupBox>
#include <QPlainTextEdit>
#include <QLabel>
#include <QFrame>
#include <QLineEdit>
#include "gamepad-manager.hpp"

class GamepadDeviceRow : public QFrame {
    Q_OBJECT

public:
    explicit GamepadDeviceRow(GamepadDevicePtr device, QWidget *parent = nullptr);
    GamepadDevicePtr getDevice() const;

private:
    GamepadDevicePtr device;
    QLabel *nameLabel;
    QLabel *statusLabel;
    QCheckBox *enabledCheck;
    QLineEdit *aliasEdit;
};

class GamepadSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit GamepadSettingsDialog(QWidget *parent = nullptr);
    ~GamepadSettingsDialog();

public slots:
    void addLogMessage(const QString &msg);

private slots:
    void onRefreshDevices();
    void onSave();

private:
    void setupUi();
    void loadSettings();
    void saveSettings();

    QCheckBox *globalEnabledCheck;
    QCheckBox *autoStartCheck;
    
    QWidget *devicesContainer;
    QVBoxLayout *devicesLayout;
    std::vector<GamepadDeviceRow*> deviceRows;

    QCheckBox *logCheck;
    QPlainTextEdit *logEdit;
    QPushButton *toggleLogBtn;
    QWidget *logContentWidget;
};
