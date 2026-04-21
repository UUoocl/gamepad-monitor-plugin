#include "gamepad-settings-dialog.hpp"
#include <QHeaderView>
#include <QCheckBox>
#include <QLineEdit>
#include <QLabel>

GamepadSettingsDialog::GamepadSettingsDialog(QWidget *parent) : QDialog(parent) {
    SetupUI();
    RefreshList();
    
    refreshTimer = new QTimer(this);
    connect(refreshTimer, &QTimer::timeout, this, &GamepadSettingsDialog::RefreshList);
    refreshTimer->start(2000); // Refresh every 2 seconds to detect new plugins
}

GamepadSettingsDialog::~GamepadSettingsDialog() {
}

void GamepadSettingsDialog::SetupUI() {
    setWindowTitle("Gamepad Monitor Settings");
    setMinimumSize(500, 400);

    QVBoxLayout *layout = new QVBoxLayout(this);

    QLabel *label = new QLabel("Connected Gamepads:", this);
    layout->addWidget(label);

    table = new QTableWidget(this);
    table->setColumnCount(4);
    table->setHorizontalHeaderLabels({"Status", "Enable", "Name", "Alias"});
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(table);

    QHBoxLayout *btnLayout = new QHBoxLayout();
    QPushButton *btnRefresh = new QPushButton("Refresh Now", this);
    connect(btnRefresh, &QPushButton::clicked, this, &GamepadSettingsDialog::RefreshList);
    btnLayout->addWidget(btnRefresh);

    QPushButton *btnClose = new QPushButton("Close", this);
    connect(btnClose, &QPushButton::clicked, this, &GamepadSettingsDialog::SaveAndClose);
    btnLayout->addWidget(btnClose);

    layout->addLayout(btnLayout);
}

void GamepadSettingsDialog::RefreshList() {
    auto& mgr = GetGamepadManager();
    mgr.RefreshDevices();
    auto devices = mgr.GetDevices();

    table->setSortingEnabled(false);
    table->setRowCount(0);
    table->setRowCount((int)devices.size());

    for (int i = 0; i < (int)devices.size(); ++i) {
        auto dev = devices[i];

        // Status
        bool connected = (dev->instanceId != -1);
        QLabel *statusLabel = new QLabel(connected ? "Connected" : "Disconnected", this);
        statusLabel->setAlignment(Qt::AlignCenter);
        if (connected) statusLabel->setStyleSheet("color: green; font-weight: bold;");
        else statusLabel->setStyleSheet("color: gray;");
        table->setCellWidget(i, 0, statusLabel);

        // Enable
        QCheckBox *cb = new QCheckBox(this);
        cb->setChecked(dev->enabled);
        connect(cb, &QCheckBox::toggled, [dev, &mgr](bool checked) {
            mgr.SetDeviceEnabled(dev->guid, checked);
        });
        QWidget *cbWidget = new QWidget();
        QHBoxLayout *cbLayout = new QHBoxLayout(cbWidget);
        cbLayout->addWidget(cb);
        cbLayout->setAlignment(Qt::AlignCenter);
        cbLayout->setContentsMargins(0, 0, 0, 0);
        table->setCellWidget(i, 1, cbWidget);

        // Name
        table->setItem(i, 2, new QTableWidgetItem(QString::fromStdString(dev->name)));
        table->item(i, 2)->setFlags(table->item(i, 2)->flags() & ~Qt::ItemIsEditable);

        // Alias
        QLineEdit *aliasEdit = new QLineEdit(QString::fromStdString(dev->alias), this);
        connect(aliasEdit, &QLineEdit::textChanged, [dev, &mgr](const QString &text) {
            mgr.SetDeviceAlias(dev->guid, text.toStdString());
        });
        table->setCellWidget(i, 3, aliasEdit);
    }
}

void GamepadSettingsDialog::SaveAndClose() {
    GetGamepadManager().SaveConfig();
    accept();
}
