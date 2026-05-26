#include "settingswindow.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QCheckBox>
#include <QComboBox>
#include <QPushButton>

SettingsWindow::SettingsWindow(QWidget *parent)
    : QWidget(parent)
{
    setWindowTitle("Налаштування");
    resize(400, 300);

    QVBoxLayout *layout = new QVBoxLayout(this);

    // Приклад елементів
    QLabel *labelTheme = new QLabel("Тема:", this);
    QCheckBox *darkTheme = new QCheckBox("Темна тема", this);

    QLabel *labelLang = new QLabel("Мова:", this);
    QComboBox *langBox = new QComboBox(this);
    langBox->addItems({"Українська", "English", "Deutsch"});

    QPushButton *okBtn = new QPushButton("OK", this);
    QPushButton *cancelBtn = new QPushButton("Скасувати", this);

    layout->addWidget(labelTheme);
    layout->addWidget(darkTheme);
    layout->addSpacing(10);
    layout->addWidget(labelLang);
    layout->addWidget(langBox);
    layout->addStretch();
    layout->addWidget(okBtn);
    layout->addWidget(cancelBtn);

    // Закриття вікна при натисканні
    connect(okBtn, &QPushButton::clicked, this, &QWidget::close);
    connect(cancelBtn, &QPushButton::clicked, this, &QWidget::close);
}
