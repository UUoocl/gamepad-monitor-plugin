#include "gamepad-settings-dialog.hpp"
#include <QHeaderView>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>
#include <QScrollArea>
#include <QHBoxLayout>

GamepadDeviceRow::GamepadDeviceRow(GamepadDevicePtr device, QWidget *parent) : QFrame(parent), device(device) {
    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->setContentsMargins(5, 5, 5, 5);

    bool connected = (device->instanceId != -1);
    statusLabel = new QLabel(connected ? "●" : "○", this);
    statusLabel->setFixedWidth(20);
    statusLabel->setStyleSheet(connected ? "color: #a6e3a1; font-size: 16px;" : "color: #585b70; font-size: 16px;");
    
    nameLabel = new QLabel(QString::fromStdString(device->name), this);
    nameLabel->setMinimumWidth(200);
    nameLabel->setStyleSheet("font-weight: bold;");

    enabledCheck = new QCheckBox("Enabled", this);
    enabledCheck->setChecked(device->enabled);

    aliasEdit = new QLineEdit(this);
    aliasEdit->setText(QString::fromStdString(device->alias));
    aliasEdit->setPlaceholderText("Alias (e.g. Player1)");
    aliasEdit->setMinimumWidth(120);
    aliasEdit->setStyleSheet("background: #1e1e2e; border: 1px solid #45475a; color: white; padding: 4px; border-radius: 4px;");

    layout->addWidget(statusLabel);
    layout->addWidget(nameLabel);
    layout->addWidget(enabledCheck);
    layout->addWidget(new QLabel("Alias:", this));
    layout->addWidget(aliasEdit);

    setStyleSheet("GamepadDeviceRow { background: #2a2a2a; border-radius: 4px; border: 1px solid #3a3a3a; }");
}

GamepadDevicePtr GamepadDeviceRow::getDevice() const {
    device->enabled = enabledCheck->isChecked();
    device->alias = aliasEdit->text().toStdString();
    return device;
}

GamepadSettingsDialog::GamepadSettingsDialog(QWidget *parent) : QDialog(parent) {
    setupUi();
    loadSettings();
    
    GetGamepadManager().SetLogCallback([this](const std::string &msg) {
        QMetaObject::invokeMethod(this, "addLogMessage", Qt::QueuedConnection, Q_ARG(QString, QString::fromStdString(msg)));
    });
}

GamepadSettingsDialog::~GamepadSettingsDialog() {
    GetGamepadManager().SetLogCallback(nullptr);
}

void GamepadSettingsDialog::setupUi() {
    setWindowTitle("Gamepad Monitor Settings");
    setMinimumSize(700, 600);
    setStyleSheet("GamepadSettingsDialog { background-color: #1e1e2e; color: #cdd6f4; } QLabel { color: #cdd6f4; }");

    QVBoxLayout *root = new QVBoxLayout(this);
    root->setContentsMargins(16, 16, 16, 16);
    root->setSpacing(12);

    // Global Settings Group
    QGroupBox *globalGb = new QGroupBox("Gamepad Signal Configuration");
    globalGb->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid rgba(255,255,255,0.1); margin-top: 12px; padding-top: 12px; color: #f5c2e7; } QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }");
    QVBoxLayout *globalLay = new QVBoxLayout(globalGb);

    globalEnabledCheck = new QCheckBox("Enable Gamepad System");
    globalEnabledCheck->setStyleSheet("font-weight: bold; color: #a6e3a1; margin-bottom: 5px;");
    globalLay->addWidget(globalEnabledCheck);

    autoStartCheck = new QCheckBox("Auto-start with OBS");
    globalLay->addWidget(autoStartCheck);
    
    root->addWidget(globalGb);

    // Devices Group
    QGroupBox *devicesGb = new QGroupBox("Gamepad Devices");
    devicesGb->setStyleSheet(
        "QGroupBox { font-weight: bold; border: 1px solid rgba(255,255,255,0.1); margin-top: 12px; padding-top: 12px; color: #89b4fa; } QGroupBox::title { subcontrol-origin: margin; left: 8px; padding: 0 4px; }");
    QVBoxLayout *devicesGbLayout = new QVBoxLayout(devicesGb);
    
    QPushButton *refreshBtn = new QPushButton("Refresh Devices", this);
    refreshBtn->setStyleSheet("background-color: #313244; color: white; padding: 6px; border-radius: 4px;");
    connect(refreshBtn, &QPushButton::clicked, this, &GamepadSettingsDialog::onRefreshDevices);
    devicesGbLayout->addWidget(refreshBtn);

    QScrollArea *scrollArea = new QScrollArea();
    scrollArea->setWidgetResizable(true);
    scrollArea->setFrameShape(QFrame::NoFrame);
    scrollArea->setStyleSheet("QScrollArea { background: transparent; }");
    
    devicesContainer = new QWidget();
    devicesLayout = new QVBoxLayout(devicesContainer);
    devicesLayout->addStretch();
    
    scrollArea->setWidget(devicesContainer);
    devicesGbLayout->addWidget(scrollArea);
    
    root->addWidget(devicesGb, 1);

    // Log Group
    QGroupBox *logGb = new QGroupBox("Log Console");
    logGb->setStyleSheet("QGroupBox { color: #89b4fa; font-weight: bold; border: 1px solid rgba(255,255,255,0.1); }");
    QVBoxLayout *logOuterLay = new QVBoxLayout(logGb);

    logContentWidget = new QWidget();
    QVBoxLayout *logLay = new QVBoxLayout(logContentWidget);
    logLay->setContentsMargins(0, 0, 0, 0);

    logCheck = new QCheckBox("Enable Logging");
    logLay->addWidget(logCheck);
    connect(logCheck, &QCheckBox::toggled, [](bool checked) { GetGamepadManager().EnableLogging(checked); });

    logEdit = new QPlainTextEdit();
    logEdit->setReadOnly(true);
    logEdit->setFixedHeight(150);
    logEdit->setStyleSheet("background: #11111b; color: #a6e3a1; border: 1px solid #45475a; font-family: monospace;");
    logLay->addWidget(logEdit);

    toggleLogBtn = new QPushButton(GetGamepadManager().IsLogCollapsed() ? "Expand Log" : "Collapse Log");
    toggleLogBtn->setStyleSheet(
        "QPushButton { background: rgba(255,255,255,0.05); color: #89b4fa; border: none; padding: 4px; border-radius: 4px; text-align: left; } QPushButton:hover { background: rgba(255,255,255,0.1); }");

    logOuterLay->addWidget(toggleLogBtn);
    logOuterLay->addWidget(logContentWidget);
    root->addWidget(logGb);

    if (GetGamepadManager().IsLogCollapsed()) {
        logContentWidget->hide();
    }

    connect(toggleLogBtn, &QPushButton::clicked, [this]() {
        bool collapsed = !logContentWidget->isHidden();
        logContentWidget->setVisible(!collapsed);
        toggleLogBtn->setText(collapsed ? "Expand Log" : "Collapse Log");
        GetGamepadManager().SetLogCollapsed(collapsed);
        GetGamepadManager().SaveConfig();
    });

    // Footer
    QHBoxLayout *footer = new QHBoxLayout();
    QPushButton *saveBtn = new QPushButton("Save & Apply");
    saveBtn->setStyleSheet("background-color: #89b4fa; color: #11111b; font-weight: bold; padding: 10px; border-radius: 4px;");
    connect(saveBtn, &QPushButton::clicked, this, &GamepadSettingsDialog::onSave);
    
    footer->addStretch();
    footer->addWidget(saveBtn);
    root->addLayout(footer);
}

void GamepadSettingsDialog::onRefreshDevices() {
    for (auto row : deviceRows) {
        row->getDevice(); 
    }

    GetGamepadManager().RefreshDevices();
    
    for (auto row : deviceRows) {
        devicesLayout->removeWidget(row);
        delete row;
    }
    deviceRows.clear();

    auto devices = GetGamepadManager().GetDevices();
    for (const auto &dev : devices) {
        GamepadDeviceRow *row = new GamepadDeviceRow(dev, this);
        deviceRows.push_back(row);
        devicesLayout->insertWidget((int)deviceRows.size() - 1, row);
    }
}

void GamepadSettingsDialog::loadSettings() {
    auto &mgr = GetGamepadManager();
    mgr.LoadConfig();
    globalEnabledCheck->setChecked(mgr.IsGlobalEnabled());
    autoStartCheck->setChecked(mgr.GetAutoStart());
    logCheck->setChecked(mgr.IsLoggingEnabled());
    
    onRefreshDevices();
}

void GamepadSettingsDialog::saveSettings() {
    auto &mgr = GetGamepadManager();
    mgr.SetGlobalEnabled(globalEnabledCheck->isChecked());
    mgr.SetAutoStart(autoStartCheck->isChecked());
    mgr.EnableLogging(logCheck->isChecked());
    
    for (auto row : deviceRows) {
        row->getDevice();
    }

    mgr.SaveConfig();
}

void GamepadSettingsDialog::onSave() {
    saveSettings();
    accept();
}

void GamepadSettingsDialog::addLogMessage(const QString &msg) {
    logEdit->appendPlainText(msg);
}
