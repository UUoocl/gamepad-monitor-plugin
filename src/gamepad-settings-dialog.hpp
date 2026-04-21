#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QTableWidget>
#include <QPushButton>
#include <QTimer>
#include "gamepad-manager.hpp"

class GamepadSettingsDialog : public QDialog {
    Q_OBJECT

public:
    explicit GamepadSettingsDialog(QWidget *parent = nullptr);
    ~GamepadSettingsDialog();

private slots:
    void RefreshList();
    void SaveAndClose();

private:
    void SetupUI();
    
    QTableWidget *table;
    QTimer *refreshTimer;
};
