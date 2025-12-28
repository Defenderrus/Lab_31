#include "mainwindow.h"
#include "ui_mainwindow.h"

#include "../sequences/ArraySequence.hpp"
#include "../LazySequence.hpp"
#include "../Stream.hpp"
#include "../StreamEncoder.hpp"
#include "../StreamStatistics.hpp"

#include <QInputDialog>
#include <QFileDialog>
#include <QMessageBox>
#include <QTextStream>
#include <QDateTime>
#include <QRegularExpression>
#include <QLabel>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QSpinBox>

#include <memory>
#include <vector>
#include <sstream>
#include <functional>
#include <algorithm>
#include <cmath>

using namespace std;

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , currentMode(Mode::Finite)
{
    ui->setupUi(this);

    // Настройка интерфейса
    setWindowTitle("Система последовательностей и потоков");

    // Подключение сигналов
    connect(ui->radioFinite, &QRadioButton::toggled, this, &MainWindow::onModeChanged);
    connect(ui->radioInfinite, &QRadioButton::toggled, this, &MainWindow::onModeChanged);
    connect(ui->radioStream, &QRadioButton::toggled, this, &MainWindow::onModeChanged);

    connect(ui->btnLoadFile, &QPushButton::clicked, this, &MainWindow::onLoadFileClicked);
    connect(ui->btnInitialize, &QPushButton::clicked, this, &MainWindow::onInitializeClicked);
    connect(ui->btnClearInput, &QPushButton::clicked, this, &MainWindow::onClearInputClicked);
    connect(ui->btnClearOutput, &QPushButton::clicked, this, &MainWindow::onClearOutputClicked);
    connect(ui->btnSaveOutput, &QPushButton::clicked, this, &MainWindow::onSaveOutputClicked);

    connect(ui->comboOperation, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &MainWindow::onOperationChanged);
    connect(ui->btnExecute, &QPushButton::clicked, this, &MainWindow::onExecuteClicked);

    // Инициализация
    initFiniteSequence();
    initInfiniteSequence();
    initStream();

    // Обновление списка операций
    updateOperationsList();

    log("Система инициализирована. Выбран режим: Конечная последовательность");
    log("Используйте кнопку 'Инициализировать' для загрузки входных данных в текущий объект");
    ui->statusBar->showMessage("Готово");
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::log(const QString &message)
{
    QString timestamp = QDateTime::currentDateTime().toString("[hh:mm:ss] ");
    ui->textEditOutput->append(timestamp + message);

    // Прокрутка вниз
    QTextCursor cursor = ui->textEditOutput->textCursor();
    cursor.movePosition(QTextCursor::End);
    ui->textEditOutput->setTextCursor(cursor);
}

void MainWindow::clearOutput()
{
    ui->textEditOutput->clear();
}

void MainWindow::onModeChanged()
{
    // Определяем новый режим
    if (ui->radioFinite->isChecked() && currentMode != Mode::Finite) {
        currentMode = Mode::Finite;
        log("Переключен режим: Конечная последовательность");
        ui->labelInputHint->setText("Введите числа через пробел или каждое с новой строки. Пример: 1 2 3 4 5");
        ui->statusBar->showMessage("Режим: Конечная последовательность");
    } else if (ui->radioInfinite->isChecked() && currentMode != Mode::Infinite) {
        currentMode = Mode::Infinite;
        log("Переключен режим: Бесконечная последовательность");
        ui->labelInputHint->setText("Введите числа для создания циклического паттерна, который будет повторяться бесконечно.\n"
                                    "Если оставить поле пустым, будет использована последовательность натуральных чисел.");
        ui->statusBar->showMessage("Режим: Бесконечная последовательность (натуральные числа)");
    } else if (ui->radioStream->isChecked() && currentMode != Mode::Stream) {
        currentMode = Mode::Stream;
        log("Переключен режим: Поток (ReadWriteStream)");
        ui->labelInputHint->setText("Введите текст для работы с потоком или загрузите файл. Пример: Hello World");
        ui->statusBar->showMessage("Режим: Поток");
    }

    // Обновляем список операций
    updateOperationsList();
}

void MainWindow::updateOperationsList()
{
    ui->comboOperation->clear();

    switch(currentMode) {
    case Mode::Finite:
        ui->comboOperation->addItems({
            "Получить первый элемент",
            "Получить последний элемент",
            "Получить элемент по индексу",
            "Получить длину последовательности",
            "Добавить элемент в конец",
            "Добавить элемент в начало",
            "Вставить элемент в позицию",
            "Удалить элемент по индексу",
            "Объединить с другой последовательностью",
            "Получить подпоследовательность",
            "Возвести все элементы в квадрат (Map)",
            "Фильтровать чётные/нечётные числа (Where)",
            "Сумма/произведение элементов (Reduce)"
        });
        break;

    case Mode::Infinite:
        ui->comboOperation->addItems({
            "Получить первый элемент",
            "Получить элемент по индексу",
            "Получить информацию о длине",
            "Получить подпоследовательность",
            "Возвести элементы в квадрат (Map)",
            "Фильтровать чётные/нечётные числа (Where)",
            "Сумма/произведение элементов (Reduce, ограниченное)"
        });
        break;

    case Mode::Stream:
        ui->comboOperation->addItems({
            "Прочитать один символ",
            "Прочитать все символы",
            "Записать один символ",
            "Записать все символы",
            "Проверить конец потока",
            "Получить символ по индексу",
            "Сменить позицию в потоке",
            "Получить длину буфера записи",
            "Получить содержимое буфера",
            "Получить данные для чтения",
            "RLE кодирование",
            "RLE декодирование",
            "Статистика потока"
        });
        break;
    }

    clearParams();
    ui->comboOperation->setCurrentIndex(0);
    onOperationChanged(0);
}

void MainWindow::clearParams()
{
    // Удаляем все виджеты параметров
    QLayoutItem* child;
    while ((child = ui->gridLayoutParams->takeAt(0)) != nullptr) {
        if (child->widget()) {
            child->widget()->deleteLater();
        }
        delete child;
    }

    // Скрываем виджет параметров
    ui->widgetParams->setVisible(false);
    ui->labelParamHint->setVisible(true);
}

void MainWindow::showParams()
{
    ui->widgetParams->setVisible(true);
    ui->labelParamHint->setVisible(false);
}

void MainWindow::updateParamHints()
{
    QString operation = ui->comboOperation->currentText();
    QString hint;

    if (operation.contains("индекс")) {
        hint = "Введите целое неотрицательное число";
    } else if (operation.contains("элемент") && !operation.contains("Получить")) {
        hint = "Введите число (для последовательности) или символ (для потока)";
    } else if (operation.contains("Объединить")) {
        hint = "Введите числа через пробел для второй последовательности";
    } else if (operation.contains("подпоследовательность")) {
        hint = "Введите начальный и конечный индексы";
    } else if (operation.contains("Фильтровать")) {
        hint = "Выберите тип фильтрации";
    } else if (operation.contains("Сумма/произведение")) {
        hint = "Выберите операцию агрегации";
    } else if (operation.contains("кодирование") || operation.contains("декодирование")) {
        hint = "Для RLE операций используются данные из поля ввода";
    } else {
        hint = "Дополнительные параметры не требуются";
    }

    ui->labelParamHint->setText(hint);
}

void MainWindow::onOperationChanged(int index)
{
    Q_UNUSED(index);

    clearParams();
    updateParamHints();

    QString operation = ui->comboOperation->currentText();

    if (currentMode == Mode::Finite) {
        if (operation.contains("индекс") && !operation.contains("длину")) {
            showParams();
            QLabel *label = new QLabel("Индекс:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramIndex");
            edit->setPlaceholderText("0, 1, 2, ...");
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(edit, 0, 1);

            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("Добавить") && !operation.contains("начало")) {
            showParams();
            QLabel *label = new QLabel("Элемент:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramElement");
            edit->setPlaceholderText("Введите число");
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(edit, 0, 1);

            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("начало")) {
            showParams();
            QLabel *label = new QLabel("Элемент:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramElement");
            edit->setPlaceholderText("Введите число");
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(edit, 0, 1);

            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("Вставить")) {
            showParams();
            QLabel *label1 = new QLabel("Элемент:");
            QLineEdit *edit1 = new QLineEdit();
            edit1->setObjectName("paramElement");
            edit1->setPlaceholderText("Введите число");

            QLabel *label2 = new QLabel("Индекс:");
            QLineEdit *edit2 = new QLineEdit();
            edit2->setObjectName("paramIndex");
            edit2->setPlaceholderText("0, 1, 2, ...");

            ui->gridLayoutParams->addWidget(label1, 0, 0);
            ui->gridLayoutParams->addWidget(edit1, 0, 1);
            ui->gridLayoutParams->addWidget(label2, 1, 0);
            ui->gridLayoutParams->addWidget(edit2, 1, 1);

            connect(edit1, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
            connect(edit2, &QLineEdit::textChanged, this, &MainWindow::onParam2Changed);
        }
        else if (operation.contains("Объединить")) {
            showParams();
            QLabel *label = new QLabel("Вторая последовательность:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramSequence");
            edit->setPlaceholderText("Числа через пробел, например: 10 20 30");
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(edit, 0, 1);

            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("подпоследовательность")) {
            showParams();
            QLabel *label1 = new QLabel("Начальный индекс:");
            QLineEdit *edit1 = new QLineEdit();
            edit1->setObjectName("paramStart");
            edit1->setPlaceholderText("0");

            QLabel *label2 = new QLabel("Конечный индекс:");
            QLineEdit *edit2 = new QLineEdit();
            edit2->setObjectName("paramEnd");
            edit2->setPlaceholderText("0");

            ui->gridLayoutParams->addWidget(label1, 0, 0);
            ui->gridLayoutParams->addWidget(edit1, 0, 1);
            ui->gridLayoutParams->addWidget(label2, 1, 0);
            ui->gridLayoutParams->addWidget(edit2, 1, 1);

            connect(edit1, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
            connect(edit2, &QLineEdit::textChanged, this, &MainWindow::onParam2Changed);
        }
        else if (operation.contains("Фильтровать")) {
            showParams();
            QLabel *label = new QLabel("Тип фильтра:");
            QComboBox *combo = new QComboBox();
            combo->setObjectName("paramFilter");
            combo->addItems({"Чётные числа", "Нечётные числа"});
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(combo, 0, 1);

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("Сумма/произведение")) {
            showParams();
            QLabel *label = new QLabel("Операция:");
            QComboBox *combo = new QComboBox();
            combo->setObjectName("paramOperation");
            combo->addItems({"Сумма", "Произведение"});
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(combo, 0, 1);

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &MainWindow::onParam1Changed);
        }
    }
    else if (currentMode == Mode::Infinite) {
        if (operation.contains("индекс") && !operation.contains("информацию")) {
            showParams();
            QLabel *label = new QLabel("Индекс:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramIndex");
            edit->setPlaceholderText("0, 1, 2, ...");
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(edit, 0, 1);

            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("подпоследовательность")) {
            showParams();
            QLabel *label1 = new QLabel("Начальный индекс:");
            QLineEdit *edit1 = new QLineEdit();
            edit1->setObjectName("paramStart");
            edit1->setPlaceholderText("0");

            QLabel *label2 = new QLabel("Конечный индекс:");
            QLineEdit *edit2 = new QLineEdit();
            edit2->setObjectName("paramEnd");
            edit2->setPlaceholderText("10");

            ui->gridLayoutParams->addWidget(label1, 0, 0);
            ui->gridLayoutParams->addWidget(edit1, 0, 1);
            ui->gridLayoutParams->addWidget(label2, 1, 0);
            ui->gridLayoutParams->addWidget(edit2, 1, 1);

            connect(edit1, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
            connect(edit2, &QLineEdit::textChanged, this, &MainWindow::onParam2Changed);
        }
        else if (operation.contains("Фильтровать")) {
            showParams();
            QLabel *label = new QLabel("Тип фильтра:");
            QComboBox *combo = new QComboBox();
            combo->setObjectName("paramFilter");
            combo->addItems({"Чётные числа", "Нечётные числа"});
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(combo, 0, 1);

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("Сумма/произведение")) {
            showParams();
            QLabel *label1 = new QLabel("Операция:");
            QComboBox *combo = new QComboBox();
            combo->setObjectName("paramOperation");
            combo->addItems({"Сумма", "Произведение"});

            QLabel *label2 = new QLabel("Количество элементов:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramCount");
            edit->setPlaceholderText("100");
            edit->setText("100");

            ui->gridLayoutParams->addWidget(label1, 0, 0);
            ui->gridLayoutParams->addWidget(combo, 0, 1);
            ui->gridLayoutParams->addWidget(label2, 1, 0);
            ui->gridLayoutParams->addWidget(edit, 1, 1);

            connect(combo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &MainWindow::onParam1Changed);
            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam2Changed);
        }
    }
    else if (currentMode == Mode::Stream) {
        if (operation.contains("один") && !operation.contains("Прочитать")) {
            showParams();
            QLabel *label = new QLabel("Символ:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramChar");
            edit->setPlaceholderText("Введите один символ");
            edit->setMaxLength(1);
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(edit, 0, 1);

            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("индекс") && !operation.contains("длину")) {
            showParams();
            QLabel *label = new QLabel("Индекс:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramIndex");
            edit->setPlaceholderText("0, 1, 2, ...");
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(edit, 0, 1);

            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
        }
        else if (operation.contains("Сменить позицию")) {
            showParams();
            QLabel *label = new QLabel("Новая позиция:");
            QLineEdit *edit = new QLineEdit();
            edit->setObjectName("paramPosition");
            edit->setPlaceholderText("0, 1, 2, ...");
            ui->gridLayoutParams->addWidget(label, 0, 0);
            ui->gridLayoutParams->addWidget(edit, 0, 1);

            connect(edit, &QLineEdit::textChanged, this, &MainWindow::onParam1Changed);
        }
    }
}

void MainWindow::onParam1Changed()
{
    // Валидация первого параметра в реальном времени
    QString operation = ui->comboOperation->currentText();
    QWidget* paramWidget = ui->widgetParams->findChild<QWidget*>("paramIndex");

    if (!paramWidget) {
        paramWidget = ui->widgetParams->findChild<QWidget*>("paramElement");
    }
    if (!paramWidget) {
        paramWidget = ui->widgetParams->findChild<QWidget*>("paramChar");
    }
    if (!paramWidget) {
        paramWidget = ui->widgetParams->findChild<QWidget*>("paramPosition");
    }
    if (!paramWidget) {
        paramWidget = ui->widgetParams->findChild<QWidget*>("paramStart");
    }
    if (!paramWidget) {
        paramWidget = ui->widgetParams->findChild<QWidget*>("paramSequence");
    }

    if (paramWidget) {
        QString paramValue;
        if (auto lineEdit = qobject_cast<QLineEdit*>(paramWidget)) {
            paramValue = lineEdit->text().trimmed();

            // Валидация в зависимости от типа параметра
            if (operation.contains("индекс") || operation.contains("позицию") ||
                operation.contains("длину") || operation.contains("позицию")) {

                bool ok;
                if (!paramValue.isEmpty()) {
                    int value = paramValue.toInt(&ok);
                    if (!ok || value < 0) {
                        // Подсветка некорректного ввода
                        lineEdit->setStyleSheet("border: 1px solid red;");
                        ui->statusBar->showMessage("Некорректный индекс. Должно быть неотрицательное целое число", 3000);
                    } else {
                        lineEdit->setStyleSheet("");
                        ui->statusBar->showMessage("✓", 1000);
                    }
                } else {
                    lineEdit->setStyleSheet("");
                }
            }
            else if (operation.contains("элемент") && !operation.contains("Получить")) {
                if (currentMode == Mode::Finite || currentMode == Mode::Infinite) {
                    // Для последовательностей - должно быть число
                    bool ok;
                    if (!paramValue.isEmpty()) {
                        paramValue.toInt(&ok);
                        if (!ok) {
                            lineEdit->setStyleSheet("border: 1px solid red;");
                            ui->statusBar->showMessage("Некорректный элемент. Должно быть целое число", 3000);
                        } else {
                            lineEdit->setStyleSheet("");
                            ui->statusBar->showMessage("✓", 1000);
                        }
                    } else {
                        lineEdit->setStyleSheet("");
                    }
                }
                else if (currentMode == Mode::Stream) {
                    // Для потока - должен быть один символ
                    if (!paramValue.isEmpty() && paramValue.length() != 1) {
                        lineEdit->setStyleSheet("border: 1px solid red;");
                        ui->statusBar->showMessage("Должен быть указан ровно один символ", 3000);
                    } else {
                        lineEdit->setStyleSheet("");
                        ui->statusBar->showMessage("✓", 1000);
                    }
                }
            }
        }
    }
}

void MainWindow::onParam2Changed()
{
    // Валидация второго параметра в реальном времени
    QString operation = ui->comboOperation->currentText();
    QWidget* paramWidget = ui->widgetParams->findChild<QWidget*>("paramEnd");
    if (!paramWidget) {
        paramWidget = ui->widgetParams->findChild<QWidget*>("paramCount");
    }

    if (paramWidget) {
        QString paramValue;
        if (auto lineEdit = qobject_cast<QLineEdit*>(paramWidget)) {
            paramValue = lineEdit->text().trimmed();

            // Валидация в зависимости от типа параметра
            if (operation.contains("подпоследовательность")) {
                bool ok;
                if (!paramValue.isEmpty()) {
                    int value = paramValue.toInt(&ok);

                    // Получаем значение первого параметра для сравнения
                    QString startValue = getParamValue("paramStart");
                    int start = startValue.toInt(&ok);

                    if (!ok || value < start) {
                        lineEdit->setStyleSheet("border: 1px solid red;");
                        ui->statusBar->showMessage("Конечный индекс должен быть >= начального", 3000);
                    } else {
                        lineEdit->setStyleSheet("");
                        ui->statusBar->showMessage("✓", 1000);
                    }
                } else {
                    lineEdit->setStyleSheet("");
                }
            }
            else if (operation.contains("Сумма/произведение") && currentMode == Mode::Infinite) {
                bool ok;
                if (!paramValue.isEmpty()) {
                    int value = paramValue.toInt(&ok);
                    if (!ok || value <= 0) {
                        lineEdit->setStyleSheet("border: 1px solid red;");
                        ui->statusBar->showMessage("Количество элементов должно быть положительным целым числом", 3000);
                    } else {
                        lineEdit->setStyleSheet("");
                        ui->statusBar->showMessage("✓", 1000);
                    }
                } else {
                    lineEdit->setStyleSheet("");
                }
            }
            else if (operation.contains("индекс") || operation.contains("позицию")) {
                bool ok;
                if (!paramValue.isEmpty()) {
                    int value = paramValue.toInt(&ok);
                    if (!ok || value < 0) {
                        lineEdit->setStyleSheet("border: 1px solid red;");
                        ui->statusBar->showMessage("Некорректное значение. Должно быть неотрицательное целое число", 3000);
                    } else {
                        lineEdit->setStyleSheet("");
                        ui->statusBar->showMessage("✓", 1000);
                    }
                } else {
                    lineEdit->setStyleSheet("");
                }
            }
        }
    }

    // Дополнительная проверка согласованности параметров
    if (operation.contains("подпоследовательность")) {
        QString startStr = getParamValue("paramStart");
        QString endStr = getParamValue("paramEnd");

        bool ok1, ok2;
        int start = startStr.toInt(&ok1);
        int end = endStr.toInt(&ok2);

        if (ok1 && ok2 && startStr != "" && endStr != "") {
            if (end < start) {
                ui->statusBar->showMessage("Внимание: конечный индекс меньше начального", 3000);
            } else {
                ui->statusBar->showMessage("Диапазон: " + QString::number(end - start + 1) + " элементов", 3000);
            }
        }
    }
    else if (operation.contains("Вставить элемент")) {
        QString elementStr = getParamValue("paramElement");
        QString indexStr = getParamValue("paramIndex");

        bool ok;
        int index = indexStr.toInt(&ok);

        if (ok && indexStr != "") {
            if (index < 0) {
                ui->statusBar->showMessage("Индекс должен быть неотрицательным", 3000);
            } else if (index > 1000) {
                ui->statusBar->showMessage("Внимание: большой индекс", 3000);
            }
        }
    }
}

void MainWindow::onLoadFileClicked()
{
    if (currentMode != Mode::Stream) {
        QMessageBox::information(this, "Информация", "Загрузка файлов доступна только в режиме потока.");
        return;
    }

    QString fileName = QFileDialog::getOpenFileName(this, "Выберите файл", QDir::homePath(),
                                                    "Текстовые файлы (*.txt);;Все файлы (*)");

    if (!fileName.isEmpty()) {
        ui->lineEditFile->setText(fileName);
        QFile file(fileName);
        if (!file.open(QIODevice::ReadOnly)) {
            log("ОШИБКА: Не удалось открыть файл: " + fileName);
            return;
        }
        QByteArray data = file.readAll();
        file.close();
        QString content = QString::fromUtf8(data);

        // Ограничиваем отображение
        if (content.length() > 5000) {
            content = content.left(5000) + "\n... [файл обрезан, всего " + QString::number(content.length()) + " символов]";
        }
        ui->textEditInput->setPlainText(content);
        log("Загружен файл: " + fileName + " (" + QString::number(content.length()) + " символов)");
    }
}

void MainWindow::onInitializeClicked()
{
    log("\n=== ИНИЦИАЛИЗАЦИЯ ОБЪЕКТА ===");

    switch(currentMode) {
    case Mode::Finite:
        initializeFiniteSequence();
        break;
    case Mode::Infinite:
        initializeInfiniteSequence();
        break;
    case Mode::Stream:
        initializeStream();
        break;
    }
}

void MainWindow::onClearInputClicked()
{
    ui->textEditInput->clear();
    ui->lineEditFile->clear();

    log("\n=== ОЧИСТКА ===");

    switch(currentMode) {
    case Mode::Finite:
        finiteSequence = make_shared<LazySequence<int>>();
        log("Конечная последовательность сброшена (пустая)");
        ui->statusBar->showMessage("Конечная последовательность сброшена", 3000);
        break;

    case Mode::Infinite:
        initInfiniteSequence();
        log("Бесконечная последовательность сброшена к исходному состоянию (натуральные числа)");
        ui->statusBar->showMessage("Бесконечная последовательность сброшена", 3000);
        break;

    case Mode::Stream:
        if (stream && stream->IsOpen()) {
            stream->Close();
        }
        initStream();
        log("Поток сброшен (пустой)");
        ui->statusBar->showMessage("Поток сброшен", 3000);
        break;
    }

    log("Входные данные очищены");
}

void MainWindow::onClearOutputClicked()
{
    clearOutput();
    log("Вывод очищен");
}

void MainWindow::onSaveOutputClicked()
{
    QString fileName = QFileDialog::getSaveFileName(this, "Сохранить вывод",
                                                    QDir::homePath() + "/output.txt",
                                                    "Текстовые файлы (*.txt);;Все файлы (*)");

    if (!fileName.isEmpty()) {
        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
            QTextStream out(&file);
            out << ui->textEditOutput->toPlainText();
            file.close();
            log("Вывод сохранен в файл: " + fileName);
        } else {
            log("ОШИБКА: Не удалось сохранить файл: " + fileName);
        }
    }
}

void MainWindow::onExecuteClicked()
{
    QString operation = ui->comboOperation->currentText();
    log("\n=== Выполнение операции ===");
    log("Режим: " + QString(currentMode == Mode::Finite ? "Конечная последовательность" :
                                currentMode == Mode::Infinite ? "Бесконечная последовательность" : "Поток"));
    log("Операция: " + operation);

    try {
        switch(currentMode) {
        case Mode::Finite:
            if (operation.contains("Получить первый элемент")) finiteGetFirst();
            else if (operation.contains("Получить последний элемент")) finiteGetLast();
            else if (operation.contains("Получить элемент по индексу")) finiteGetAt();
            else if (operation.contains("Получить длину последовательности")) finiteGetLength();
            else if (operation.contains("Добавить элемент в конец")) finiteAppend();
            else if (operation.contains("Добавить элемент в начало")) finitePrepend();
            else if (operation.contains("Вставить элемент в позицию")) finiteInsertAt();
            else if (operation.contains("Удалить элемент по индексу")) finiteRemove();
            else if (operation.contains("Объединить с другой последовательностью")) finiteConcat();
            else if (operation.contains("Получить подпоследовательность")) finiteSubsequence();
            else if (operation.contains("Возвести все элементы в квадрат")) finiteMapSquare();
            else if (operation.contains("Фильтровать чётные/нечётные числа")) finiteFilterEvenOdd();
            else if (operation.contains("Сумма/произведение элементов")) finiteReduceSumProduct();
            break;

        case Mode::Infinite:
            if (operation.contains("Получить первый элемент")) infiniteGetFirst();
            else if (operation.contains("Получить элемент по индексу")) infiniteGetAt();
            else if (operation.contains("Получить информацию о длине")) infiniteGetLength();
            else if (operation.contains("Получить подпоследовательность")) infiniteSubsequence();
            else if (operation.contains("Возвести элементы в квадрат")) infiniteMapSquare();
            else if (operation.contains("Фильтровать чётные/нечётные числа")) infiniteFilterEvenOdd();
            else if (operation.contains("Сумма/произведение элементов")) infiniteReduceSumProduct();
            break;

        case Mode::Stream:
            if (operation.contains("Прочитать один символ")) streamReadOne();
            else if (operation.contains("Прочитать все символы")) streamReadAll();
            else if (operation.contains("Записать один символ")) streamWriteOne();
            else if (operation.contains("Записать все символы")) streamWriteAll();
            else if (operation.contains("Проверить конец потока")) streamIsEnd();
            else if (operation.contains("Получить символ по индексу")) streamGetAt();
            else if (operation.contains("Сменить позицию в потоке")) streamSeek();
            else if (operation.contains("Получить длину буфера записи")) streamGetLength();
            else if (operation.contains("Получить содержимое буфера")) streamGetBuffer();
            else if (operation.contains("Получить данные для чтения")) streamGetData();
            else if (operation.contains("RLE кодирование")) streamEncode();
            else if (operation.contains("RLE декодирование")) streamDecode();
            else if (operation.contains("Статистика потока")) streamStats();
            break;
        }
    } catch (const exception &e) {
        log("ОШИБКА выполнения: " + QString::fromStdString(e.what()));
    } catch (...) {
        log("Неизвестная ошибка выполнения");
    }
}

// =============== ИНИЦИАЛИЗАЦИЯ ОБЪЕКТОВ ===============

void MainWindow::initFiniteSequence()
{
    // Инициализируем пустую последовательность
    finiteSequence = make_shared<LazySequence<int>>();
    log("Инициализирована конечная последовательность (пустая)");
}

void MainWindow::initInfiniteSequence()
{
    // Создаем генератор натуральных чисел: 1, 2, 3, ...
    auto generator = make_shared<Generator<int>>(
        []() {
            static int counter = 1;
            return counter++;
        },
        []() { return true; }
        );

    infiniteSequence = make_shared<LazySequence<int>>(generator, Cardinal::Infinite());
    log("Инициализирована бесконечная последовательность натуральных чисел");
}

void MainWindow::initStream()
{
    // Создаем пустой поток
    auto writeBuffer = make_shared<DynamicArray<char>>(0);
    auto emptySeq = make_shared<LazySequence<char>>();

    stream = make_shared<ReadWriteStream<char>>(emptySeq, writeBuffer);
    stream->Open();
    log("Инициализирован поток (ReadWriteStream)");
}

void MainWindow::initializeFiniteSequence()
{
    log("Инициализация конечной последовательности...");

    QString data = ui->textEditInput->toPlainText().trimmed();
    if (data.isEmpty()) {
        log("ОШИБКА: Нет входных данных для инициализации");
        ui->statusBar->showMessage("Ошибка: нет данных", 3000);
        return;
    }

    // Парсим числа
    vector<int> numbers = parseNumbers();
    if (numbers.empty()) {
        log("ОШИБКА: Не найдено корректных чисел во входных данных");
        ui->statusBar->showMessage("Ошибка: нет чисел", 3000);
        return;
    }

    try {
        // Создаем DynamicArray из чисел
        auto array = make_shared<DynamicArray<int>>(numbers.size());
        for (size_t i = 0; i < numbers.size(); i++) {
            array->Set(i, numbers[i]);
        }

        // Создаем новую LazySequence
        finiteSequence = make_shared<LazySequence<int>>(array);

        log("  Конечная последовательность успешно инициализирована");
        log("  Загружено элементов: " + QString::number(numbers.size()));
        log("  Длина последовательности: " + QString::number(finiteSequence->GetLength()));

        // Показываем первые несколько элементов
        size_t showCount = std::min(numbers.size(), (size_t)10);
        if (showCount > 0) {
            log("  Первые " + QString::number(showCount) + " элементов:");
            for (size_t i = 0; i < showCount; i++) {
                log("    [" + QString::number(i) + "] = " + QString::number(numbers[i]));
            }
            if (numbers.size() > 10) {
                log("    ... и ещё " + QString::number(numbers.size() - 10) + " элементов");
            }
        }

        ui->statusBar->showMessage("Конечная последовательность инициализирована: " +
                                       QString::number(numbers.size()) + " элементов", 5000);

    } catch (const exception &e) {
        log("ОШИБКА при инициализации: " + QString::fromStdString(e.what()));
        ui->statusBar->showMessage("Ошибка инициализации", 3000);
    }
}

void MainWindow::initializeInfiniteSequence()
{
    log("Инициализация бесконечной последовательности...");

    QString data = ui->textEditInput->toPlainText().trimmed();

    if (data.isEmpty()) {
        initInfiniteSequence();
        ui->statusBar->showMessage("Бесконечная последовательность: натуральные числа", 5000);
        return;
    }

    // Парсим входные данные как числа
    vector<int> numbers = parseNumbers();

    if (numbers.empty()) {
        log("  Предупреждение: не найдено чисел во входных данных");
        log("  Используется генератор натуральных чисел");
        initInfiniteSequence();
        ui->statusBar->showMessage("Бесконечная последовательность: натуральные числа", 5000);
        return;
    }

    try {
        // Создаем генератор на основе входных чисел
        // Будем циклически повторять введённые числа
        auto numbers_copy = make_shared<vector<int>>(numbers);
        auto current_index = make_shared<size_t>(0);
        auto generator = make_shared<Generator<int>>(
            [numbers_copy, current_index]() mutable -> int {
                if (numbers_copy->empty()) {
                    throw runtime_error("Нет данных для генерации");
                }

                int value = (*numbers_copy)[*current_index];
                *current_index = (*current_index + 1) % numbers_copy->size();
                return value;
            },
            []() -> bool {
                return true; // Бесконечный генератор
            }
            );

        infiniteSequence = make_shared<LazySequence<int>>(generator, Cardinal::Infinite());

        log("  Бесконечная последовательность инициализирована из входных данных");
        log("  Загружено уникальных элементов: " + QString::number(numbers.size()));
        log("  Паттерн будет повторяться бесконечно");

        // Показываем входной паттерн
        log("  Входной паттерн:");
        for (size_t i = 0; i < std::min(numbers.size(), (size_t)10); i++) {
            log("    [" + QString::number(i) + "] = " + QString::number(numbers[i]));
        }
        if (numbers.size() > 10) {
            log("    ... и ещё " + QString::number(numbers.size() - 10) + " элементов");
        }

        ui->statusBar->showMessage("Бесконечная последовательность: циклический паттерн из " +
                                       QString::number(numbers.size()) + " элементов", 5000);

    } catch (const exception &e) {
        log("ОШИБКА при инициализации: " + QString::fromStdString(e.what()));
        log("  Используется генератор натуральных чисел");
        initInfiniteSequence();
        ui->statusBar->showMessage("Ошибка, использованы натуральные числа", 3000);
    }
}

void MainWindow::initializeStream()
{
    log("Инициализация потока...");

    QString data = ui->textEditInput->toPlainText().trimmed();
    if (data.isEmpty()) {
        log("ОШИБКА: Нет данных для инициализации потока");
        ui->statusBar->showMessage("Ошибка: нет данных", 3000);
        return;
    }

    try {
        // Закрываем текущий поток если открыт
        if (stream && stream->IsOpen()) {
            stream->Close();
        }

        // Создаем новую последовательность символов
        auto readArray = make_shared<DynamicArray<char>>(data.length());
        for (int i = 0; i < data.length(); i++) {
            readArray->Set(i, data[i].toLatin1());
        }

        auto readSeq = make_shared<LazySequence<char>>(readArray);
        auto writeBuffer = make_shared<DynamicArray<char>>(data.length());

        // Создаем новый поток
        stream = make_shared<ReadWriteStream<char>>(readSeq, writeBuffer);
        stream->Open();

        log("  Поток успешно инициализирован");
        log("  Загружено символов: " + QString::number(data.length()));
        log("  Текущая позиция: " + QString::number(stream->GetPosition()));

        // Показываем начало данных
        QString preview = data.left(100);
        if (data.length() > 100) {
            preview += "... [ещё " + QString::number(data.length() - 100) + " символов]";
        }
        log("  Данные: " + preview);

        ui->statusBar->showMessage("Поток инициализирован: " +
                                       QString::number(data.length()) + " символов", 5000);

    } catch (const exception &e) {
        log("ОШИБКА при инициализации потока: " + QString::fromStdString(e.what()));
        ui->statusBar->showMessage("Ошибка инициализации потока", 3000);
    }
}

// =============== ВСПОМОГАТЕЛЬНЫЕ МЕТОДЫ ===============

bool MainWindow::validateInputData()
{
    QString data = ui->textEditInput->toPlainText().trimmed();
    if (data.isEmpty()) {
        log("Предупреждение: Входные данные пустые");
        return false;
    }
    return true;
}

std::vector<int> MainWindow::parseNumbers()
{
    vector<int> numbers;
    QString data = ui->textEditInput->toPlainText().trimmed();

    if (data.isEmpty()) {
        return numbers;
    }

    // Разделяем по пробелам или переводам строк
    QStringList parts = data.split(QRegularExpression("[\\s\\n,;]+"), Qt::SkipEmptyParts);

    for (const QString &part : parts) {
        bool ok;
        int num = part.toInt(&ok);
        if (ok) {
            numbers.push_back(num);
        } else {
            log("Предупреждение: '" + part + "' не является числом и будет пропущено");
        }
    }

    return numbers;
}

QString MainWindow::getCurrentData()
{
    return ui->textEditInput->toPlainText().trimmed();
}

QString MainWindow::getParamValue(const QString &paramName)
{
    QWidget *widget = ui->widgetParams->findChild<QWidget*>(paramName);
    if (!widget) return "";

    if (auto edit = qobject_cast<QLineEdit*>(widget)) {
        return edit->text().trimmed();
    } else if (auto combo = qobject_cast<QComboBox*>(widget)) {
        return combo->currentText();
    }

    return "";
}

// =============== ОПЕРАЦИИ ДЛЯ КОНЕЧНОЙ ПОСЛЕДОВАТЕЛЬНОСТИ ===============

void MainWindow::finiteGetFirst()
{
    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Проверяем, нужно ли инициализировать последовательность из входных данных
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (numbers.empty()) {
                log("ОШИБКА: Последовательность пустая и нет входных данных");
                return;
            }

            auto array = make_shared<DynamicArray<int>>(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) {
                array->Set(i, numbers[i]);
            }

            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность из " + QString::number(numbers.size()) + " элементов");
        }

        int first = finiteSequence->GetFirst();
        log("Первый элемент: " + QString::number(first));
        log("Состояние последовательности:");
        log("  Длина: " + QString::number(finiteSequence->GetLength()));
        log("  Материализовано: " + QString::number(finiteSequence->GetMaterializedCount()));

        // Показываем первые несколько элементов
        size_t showCount = min(finiteSequence->GetLength(), (size_t)5);
        if (showCount > 0) {
            log("  Первые " + QString::number(showCount) + " элементов:");
            for (size_t i = 0; i < showCount; i++) {
                log("    [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)));
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteGetLast()
{
    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Проверяем, нужно ли инициализировать последовательность из входных данных
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (numbers.empty()) {
                log("ОШИБКА: Последовательность пустая и нет входных данных");
                return;
            }

            auto array = make_shared<DynamicArray<int>>(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) {
                array->Set(i, numbers[i]);
            }

            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность из " + QString::number(numbers.size()) + " элементов");
        }

        int last = finiteSequence->GetLast();
        log("Последний элемент: " + QString::number(last));
        log("Состояние последовательности:");
        log("  Длина: " + QString::number(finiteSequence->GetLength()));
        log("  Материализовано: " + QString::number(finiteSequence->GetMaterializedCount()));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteGetAt()
{
    QString indexStr = getParamValue("paramIndex");
    if (indexStr.isEmpty()) {
        log("ОШИБКА: Укажите индекс");
        return;
    }

    bool ok;
    int index = indexStr.toInt(&ok);
    if (!ok || index < 0) {
        log("ОШИБКА: Некорректный индекс. Должно быть неотрицательное целое число");
        return;
    }

    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Проверяем, нужно ли инициализировать последовательность из входных данных
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (numbers.empty()) {
                log("ОШИБКА: Последовательность пустая и нет входных данных");
                return;
            }

            auto array = make_shared<DynamicArray<int>>(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) {
                array->Set(i, numbers[i]);
            }

            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность из " + QString::number(numbers.size()) + " элементов");
        }

        if (index >= (int)finiteSequence->GetLength()) {
            log("ОШИБКА: Индекс " + QString::number(index) +
                " выходит за пределы последовательности (длина: " +
                QString::number(finiteSequence->GetLength()) + ")");
            return;
        }

        int value = finiteSequence->Get(index);
        log("Элемент с индексом " + QString::number(index) + ": " + QString::number(value));

        // Показываем контекст
        int start = max(0, index - 2);
        int end = min((int)finiteSequence->GetLength() - 1, index + 2);

        if (start <= end) {
            log("Окружающие элементы:");
            for (int i = start; i <= end; i++) {
                QString marker = (i == index) ? " <--" : "";
                log("  [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)) + marker);
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteGetLength()
{
    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Если последовательность пустая, пытаемся создать из входных данных
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (!numbers.empty()) {
                auto array = make_shared<DynamicArray<int>>(numbers.size());
                for (size_t i = 0; i < numbers.size(); i++) {
                    array->Set(i, numbers[i]);
                }

                finiteSequence = make_shared<LazySequence<int>>(array);
                log("Создана последовательность из входных данных");
            }
        }

        size_t length = finiteSequence->GetLength();
        log("Длина последовательности: " + QString::number(length));
        log("Материализовано элементов: " + QString::number(finiteSequence->GetMaterializedCount()));

        if (length > 0) {
            log("Элементы (первые 10):");
            size_t showCount = min(length, (size_t)10);
            for (size_t i = 0; i < showCount; i++) {
                log("  [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)));
            }
            if (length > 10) {
                log("  ... и еще " + QString::number(length - 10) + " элементов");
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteAppend()
{
    QString elementStr = getParamValue("paramElement");
    if (elementStr.isEmpty()) {
        log("ОШИБКА: Укажите элемент для добавления");
        return;
    }

    bool ok;
    int element = elementStr.toInt(&ok);
    if (!ok) {
        log("ОШИБКА: Некорректный элемент. Должно быть целое число");
        return;
    }

    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Если последовательность пустая, создаем новую с одним элементом
        if (finiteSequence->GetLength() == 0) {
            auto array = make_shared<DynamicArray<int>>(1);
            array->Set(0, element);
            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность с одним элементом");
        } else {
            auto newSeq = finiteSequence->Append(element);
            finiteSequence = shared_ptr<LazySequence<int>>(dynamic_cast<LazySequence<int>*>(newSeq));
            log("Элемент " + QString::number(element) + " добавлен в конец");
        }

        log("Новое состояние последовательности:");
        log("  Длина: " + QString::number(finiteSequence->GetLength()));
        log("  Материализовано: " + QString::number(finiteSequence->GetMaterializedCount()));

        // Показываем последние несколько элементов
        size_t length = finiteSequence->GetLength();
        size_t showStart = (length > 5) ? length - 5 : 0;

        if (showStart < length) {
            log("  Последние " + QString::number(length - showStart) + " элементов:");
            for (size_t i = showStart; i < length; i++) {
                log("    [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)));
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finitePrepend()
{
    QString elementStr = getParamValue("paramElement");
    if (elementStr.isEmpty()) {
        log("ОШИБКА: Укажите элемент для добавления");
        return;
    }

    bool ok;
    int element = elementStr.toInt(&ok);
    if (!ok) {
        log("ОШИБКА: Некорректный элемент. Должно быть целое число");
        return;
    }

    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Если последовательность пустая, создаем новую с одним элементом
        if (finiteSequence->GetLength() == 0) {
            auto array = make_shared<DynamicArray<int>>(1);
            array->Set(0, element);
            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность с одним элементом");
        } else {
            auto newSeq = finiteSequence->Prepend(element);
            finiteSequence = shared_ptr<LazySequence<int>>(dynamic_cast<LazySequence<int>*>(newSeq));
            log("Элемент " + QString::number(element) + " добавлен в начало");
        }

        log("Новое состояние последовательности:");
        log("  Длина: " + QString::number(finiteSequence->GetLength()));
        log("  Материализовано: " + QString::number(finiteSequence->GetMaterializedCount()));

        // Показываем первые несколько элементов
        size_t showCount = min(finiteSequence->GetLength(), (size_t)5);
        if (showCount > 0) {
            log("  Первые " + QString::number(showCount) + " элементов:");
            for (size_t i = 0; i < showCount; i++) {
                log("    [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)));
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteInsertAt()
{
    QString elementStr = getParamValue("paramElement");
    QString indexStr = getParamValue("paramIndex");

    if (elementStr.isEmpty() || indexStr.isEmpty()) {
        log("ОШИБКА: Укажите элемент и индекс");
        return;
    }

    bool ok1, ok2;
    int element = elementStr.toInt(&ok1);
    int index = indexStr.toInt(&ok2);

    if (!ok1) {
        log("ОШИБКА: Некорректный элемент. Должно быть целое число");
        return;
    }
    if (!ok2 || index < 0) {
        log("ОШИБКА: Некорректный индекс. Должно быть неотрицательное целое число");
        return;
    }

    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        if (finiteSequence->GetLength() == 0 && index == 0) {
            auto array = make_shared<DynamicArray<int>>(1);
            array->Set(0, element);
            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность с одним элементом");
        } else {
            if (index > (int)finiteSequence->GetLength()) {
                log("ОШИБКА: Индекс " + QString::number(index) +
                    " выходит за пределы последовательности (длина: " +
                    QString::number(finiteSequence->GetLength()) + ")");
                return;
            }

            auto newSeq = finiteSequence->InsertAt(element, index);
            finiteSequence = shared_ptr<LazySequence<int>>(dynamic_cast<LazySequence<int>*>(newSeq));
            log("Элемент " + QString::number(element) + " вставлен в позицию " + QString::number(index));
        }

        log("Новое состояние последовательности:");
        log("  Длина: " + QString::number(finiteSequence->GetLength()));
        log("  Материализовано: " + QString::number(finiteSequence->GetMaterializedCount()));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteRemove()
{
    QString indexStr = getParamValue("paramIndex");
    if (indexStr.isEmpty()) {
        log("ОШИБКА: Укажите индекс элемента для удаления");
        return;
    }

    bool ok;
    int index = indexStr.toInt(&ok);
    if (!ok || index < 0) {
        log("ОШИБКА: Некорректный индекс. Должно быть неотрицательное целое число");
        return;
    }

    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (numbers.empty()) {
                log("ОШИБКА: Последовательность пустая и нет входных данных");
                return;
            }

            auto array = make_shared<DynamicArray<int>>(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) {
                array->Set(i, numbers[i]);
            }

            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность из " + QString::number(numbers.size()) + " элементов");
        }

        if (index >= (int)finiteSequence->GetLength()) {
            log("ОШИБКА: Индекс " + QString::number(index) +
                " выходит за пределы последовательности (длина: " +
                QString::number(finiteSequence->GetLength()) + ")");
            return;
        }

        // Получаем значение удаляемого элемента
        int removedValue = finiteSequence->Get(index);

        auto newSeq = finiteSequence->Remove(index);
        finiteSequence = shared_ptr<LazySequence<int>>(dynamic_cast<LazySequence<int>*>(newSeq));

        log("Удален элемент с индексом " + QString::number(index) + ": " + QString::number(removedValue));
        log("Новое состояние последовательности:");
        log("  Длина: " + QString::number(finiteSequence->GetLength()));
        log("  Материализовано: " + QString::number(finiteSequence->GetMaterializedCount()));

        size_t showCount = min(finiteSequence->GetLength(), (size_t)10);
        if (showCount > 0) {
            log("  Первые " + QString::number(showCount) + " элементов новой последовательности:");
            for (size_t i = 0; i < showCount; i++) {
                log("    [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)));
            }
            if (finiteSequence->GetLength() > 10) {
                log("    ... и еще " + QString::number(finiteSequence->GetLength() - 10) + " элементов");
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteConcat()
{
    QString secondSeqStr = getParamValue("paramSequence");
    if (secondSeqStr.isEmpty()) {
        log("ОШИБКА: Укажите вторую последовательность (числа через пробел)");
        return;
    }

    if (!finiteSequence) {
        log("ОШИБКА: Первая последовательность не инициализирована");
        return;
    }

    try {
        // Парсим вторую последовательность
        vector<int> secondNumbers;
        QStringList parts = secondSeqStr.split(QRegularExpression("[\\s,;]+"), Qt::SkipEmptyParts);

        for (const QString &part : parts) {
            bool ok;
            int num = part.toInt(&ok);
            if (ok) {
                secondNumbers.push_back(num);
            }
        }

        if (secondNumbers.empty()) {
            log("ОШИБКА: Вторая последовательность пустая или содержит некорректные данные");
            return;
        }

        // Создаем вторую последовательность
        auto secondArray = make_shared<DynamicArray<int>>(secondNumbers.size());
        for (size_t i = 0; i < secondNumbers.size(); i++) {
            secondArray->Set(i, secondNumbers[i]);
        }

        LazySequence<int> secondSeq(secondArray);

        log("Первая последовательность (длина: " + QString::number(finiteSequence->GetLength()) + "):");
        size_t showCount1 = min(finiteSequence->GetLength(), (size_t)5);
        for (size_t i = 0; i < showCount1; i++) {
            log("  [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)));
        }
        if (finiteSequence->GetLength() > 5) {
            log("  ... и еще " + QString::number(finiteSequence->GetLength() - 5) + " элементов");
        }

        log("Вторая последовательность (длина: " + QString::number(secondNumbers.size()) + "):");
        size_t showCount2 = min(secondNumbers.size(), (size_t)5);
        for (size_t i = 0; i < showCount2; i++) {
            log("  [" + QString::number(i) + "] = " + QString::number(secondNumbers[i]));
        }
        if (secondNumbers.size() > 5) {
            log("  ... и еще " + QString::number(secondNumbers.size() - 5) + " элементов");
        }

        // Объединяем
        auto newSeq = finiteSequence->Concat(&secondSeq);
        finiteSequence = shared_ptr<LazySequence<int>>(dynamic_cast<LazySequence<int>*>(newSeq));

        log("\nРезультат объединения:");
        log("  Общая длина: " + QString::number(finiteSequence->GetLength()));
        log("  Материализовано: " + QString::number(finiteSequence->GetMaterializedCount()));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteSubsequence()
{
    QString startStr = getParamValue("paramStart");
    QString endStr = getParamValue("paramEnd");

    if (startStr.isEmpty() || endStr.isEmpty()) {
        log("ОШИБКА: Укажите начальный и конечный индексы");
        return;
    }

    bool ok1, ok2;
    int start = startStr.toInt(&ok1);
    int end = endStr.toInt(&ok2);

    if (!ok1 || start < 0) {
        log("ОШИБКА: Некорректный начальный индекс");
        return;
    }
    if (!ok2 || end < start) {
        log("ОШИБКА: Некорректный конечный индекс. Должен быть >= начального");
        return;
    }

    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Проверяем, нужно ли инициализировать последовательность из входных данных
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (numbers.empty()) {
                log("ОШИБКА: Последовательность пустая и нет входных данных");
                return;
            }

            auto array = make_shared<DynamicArray<int>>(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) {
                array->Set(i, numbers[i]);
            }

            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность из " + QString::number(numbers.size()) + " элементов");
        }

        if (end >= (int)finiteSequence->GetLength()) {
            log("ОШИБКА: Конечный индекс " + QString::number(end) +
                " выходит за пределы последовательности (длина: " +
                QString::number(finiteSequence->GetLength()) + ")");
            return;
        }

        log("Исходная последовательность (длина: " + QString::number(finiteSequence->GetLength()) + "):");
        size_t showTotal = min(finiteSequence->GetLength(), (size_t)10);
        for (size_t i = 0; i < showTotal; i++) {
            log("  [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)));
        }
        if (finiteSequence->GetLength() > 10) {
            log("  ... и еще " + QString::number(finiteSequence->GetLength() - 10) + " элементов");
        }

        auto lazySeq = dynamic_cast<LazySequence<int>*>(finiteSequence->GetSubsequence(start, end));
        auto newArray = make_shared<DynamicArray<int>>(lazySeq->GetLength());
        for (size_t i = 0; i < lazySeq->GetLength(); i++) {
            newArray->Set(i, lazySeq->Get(i));
        }
        delete lazySeq;
        finiteSequence = make_shared<LazySequence<int>>(newArray);

        log("\nПодпоследовательность с индекса " + QString::number(start) +
            " по " + QString::number(end) + ":");
        log("  Длина: " + QString::number(finiteSequence->GetLength()));
        log("  Элементы:");

        for (size_t i = 0; i < finiteSequence->GetLength(); i++) {
            log("    [" + QString::number(i + start) + "] = " + QString::number(finiteSequence->Get(i)));
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteMapSquare()
{
    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Проверяем, нужно ли инициализировать последовательность из входных данных
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (numbers.empty()) {
                log("ОШИБКА: Последовательность пустая и нет входных данных");
                return;
            }

            auto array = make_shared<DynamicArray<int>>(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) {
                array->Set(i, numbers[i]);
            }

            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность из " + QString::number(numbers.size()) + " элементов");
        }

        log("Исходная последовательность (длина: " + QString::number(finiteSequence->GetLength()) + "):");
        size_t showCount = min(finiteSequence->GetLength(), (size_t)5);
        for (size_t i = 0; i < showCount; i++) {
            log("  [" + QString::number(i) + "] = " + QString::number(finiteSequence->Get(i)));
        }
        if (finiteSequence->GetLength() > 5) {
            log("  ... и еще " + QString::number(finiteSequence->GetLength() - 5) + " элементов");
        }

        // Создаем новую последовательность с квадратами
        auto squaredSeq = finiteSequence->Map<int>([](int x) { return x * x; });

        log("\nПоследовательность квадратов:");
        for (size_t i = 0; i < showCount; i++) {
            int original = finiteSequence->Get(i);
            int squared = squaredSeq->Get(i);
            log("  [" + QString::number(i) + "] = " + QString::number(original) +
                "² = " + QString::number(squared));
        }

        delete squaredSeq;

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteFilterEvenOdd()
{
    QString filterType = getParamValue("paramFilter");
    if (filterType.isEmpty()) {
        log("ОШИБКА: Выберите тип фильтра");
        return;
    }

    bool filterEven = filterType.contains("Чётные");

    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Проверяем, нужно ли инициализировать последовательность из входных данных
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (numbers.empty()) {
                log("ОШИБКА: Последовательность пустая и нет входных данных");
                return;
            }

            auto array = make_shared<DynamicArray<int>>(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) {
                array->Set(i, numbers[i]);
            }

            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность из " + QString::number(numbers.size()) + " элементов");
        }

        log("Исходная последовательность (длина: " + QString::number(finiteSequence->GetLength()) + "):");
        log("Фильтр: " + filterType);
        log("Результат фильтрации:");

        vector<int> filtered;
        for (size_t i = 0; i < finiteSequence->GetLength(); i++) {
            int value = finiteSequence->Get(i);
            if ((filterEven && value % 2 == 0) || (!filterEven && value % 2 != 0)) {
                filtered.push_back(value);
            }
        }

        if (filtered.empty()) {
            log("  Не найдено подходящих элементов");
        } else {
            log("  Найдено " + QString::number(filtered.size()) + " элементов:");
            size_t showCount = min(filtered.size(), (size_t)10);
            for (size_t i = 0; i < showCount; i++) {
                log("    " + QString::number(filtered[i]));
            }
            if (filtered.size() > 10) {
                log("    ... и еще " + QString::number(filtered.size() - 10) + " элементов");
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::finiteReduceSumProduct()
{
    QString operationType = getParamValue("paramOperation");
    if (operationType.isEmpty()) {
        log("ОШИБКА: Выберите операцию");
        return;
    }

    bool isSum = operationType.contains("Сумма");

    if (!finiteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Проверяем, нужно ли инициализировать последовательность из входных данных
        if (finiteSequence->GetLength() == 0) {
            vector<int> numbers = parseNumbers();
            if (numbers.empty()) {
                log("ОШИБКА: Последовательность пустая и нет входных данных");
                return;
            }

            auto array = make_shared<DynamicArray<int>>(numbers.size());
            for (size_t i = 0; i < numbers.size(); i++) {
                array->Set(i, numbers[i]);
            }

            finiteSequence = make_shared<LazySequence<int>>(array);
            log("Создана последовательность из " + QString::number(numbers.size()) + " элементов");
        }

        log("Последовательность (длина: " + QString::number(finiteSequence->GetLength()) + "):");
        size_t showCount = min(finiteSequence->GetLength(), (size_t)5);
        for (size_t i = 0; i < showCount; i++) {
            log("  " + QString::number(finiteSequence->Get(i)));
        }
        if (finiteSequence->GetLength() > 5) {
            log("  ... и еще " + QString::number(finiteSequence->GetLength() - 5) + " элементов");
        }

        // Вычисляем результат
        long long result;
        QString opSymbol;

        if (isSum) {
            result = 0;
            opSymbol = "сумма";
            for (size_t i = 0; i < finiteSequence->GetLength(); i++) {
                result += finiteSequence->Get(i);
            }
        } else {
            result = 1;
            opSymbol = "произведение";
            for (size_t i = 0; i < finiteSequence->GetLength(); i++) {
                result *= finiteSequence->Get(i);
            }
        }

        log("\n" + operationType + " всех элементов: " + QString::number(result));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

// =============== ОПЕРАЦИИ ДЛЯ БЕСКОНЕЧНОЙ ПОСЛЕДОВАТЕЛЬНОСТИ ===============

void MainWindow::infiniteGetFirst()
{
    if (!infiniteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        int first = infiniteSequence->GetFirst();
        log("Первый элемент бесконечной последовательности: " + QString::number(first));
        log("Материализовано элементов: " + QString::number(infiniteSequence->GetMaterializedCount()));

        // Показываем несколько следующих элементов
        log("Следующие 5 элементов:");
        for (int i = 1; i <= 5; i++) {
            int value = infiniteSequence->Get(i);
            log("  [" + QString::number(i) + "] = " + QString::number(value));
        }

        log("Примечание: Это последовательность натуральных чисел, начиная с 1");

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::infiniteGetAt()
{
    QString indexStr = getParamValue("paramIndex");
    if (indexStr.isEmpty()) {
        log("ОШИБКА: Укажите индекс");
        return;
    }

    bool ok;
    int index = indexStr.toInt(&ok);
    if (!ok || index < 0) {
        log("ОШИБКА: Некорректный индекс. Должно быть неотрицательное целое число");
        return;
    }

    if (!infiniteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        int value = infiniteSequence->Get(index);
        log("Элемент с индексом " + QString::number(index) + ": " + QString::number(value));
        log("Материализовано элементов: " + QString::number(infiniteSequence->GetMaterializedCount()));

        // Показываем несколько соседних элементов
        int start = max(0, index - 2);
        int end = index + 2;

        if (start <= end) {
            log("Окружающие элементы:");
            for (int i = start; i <= end; i++) {
                QString marker = (i == index) ? " <--" : "";
                int neighbor = infiniteSequence->Get(i);
                log("  [" + QString::number(i) + "] = " + QString::number(neighbor) + marker);
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::infiniteGetLength()
{
    if (!infiniteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        // Попытка получить длину должна вызвать исключение для бесконечной последовательности
        infiniteSequence->GetLength();
        log("ОШИБКА: Ожидалось исключение для бесконечной последовательности");
    } catch (const runtime_error &e) {
        log("Длина последовательности: БЕСКОНЕЧНА");
        log("Материализовано элементов: " + QString::number(infiniteSequence->GetMaterializedCount()));
        log("Тип: последовательность натуральных чисел (1, 2, 3, ...)");

        // Показываем несколько элементов
        log("Первые 10 элементов:");
        for (int i = 0; i < 10; i++) {
            int value = infiniteSequence->Get(i);
            log("  [" + QString::number(i) + "] = " + QString::number(value));
        }
    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::infiniteSubsequence()
{
    QString startStr = getParamValue("paramStart");
    QString endStr = getParamValue("paramEnd");

    if (startStr.isEmpty() || endStr.isEmpty()) {
        log("ОШИБКА: Укажите начальный и конечный индексы");
        return;
    }

    bool ok1, ok2;
    int start = startStr.toInt(&ok1);
    int end = endStr.toInt(&ok2);

    if (!ok1 || start < 0) {
        log("ОШИБКА: Некорректный начальный индекс");
        return;
    }
    if (!ok2 || end < start) {
        log("ОШИБКА: Некорректный конечный индекс. Должен быть >= начального");
        return;
    }

    if (!infiniteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        auto subSeq = infiniteSequence->GetSubsequence(start, end);
        infiniteSequence = shared_ptr<LazySequence<int>>(dynamic_cast<LazySequence<int>*>(subSeq));

        auto lazySeq = dynamic_cast<LazySequence<int>*>(infiniteSequence->GetSubsequence(start, end));
        auto newArray = make_shared<DynamicArray<int>>(lazySeq->GetLength());
        for (size_t i = 0; i < lazySeq->GetLength(); i++) {
            newArray->Set(i, lazySeq->Get(i));
        }
        delete lazySeq;
        infiniteSequence = make_shared<LazySequence<int>>(newArray);

        log("Подпоследовательность бесконечной последовательности");
        log("Индексы: с " + QString::number(start) + " по " + QString::number(end));
        log("Длина: " + QString::number(infiniteSequence->GetLength()));
        log("Элементы:");

        for (size_t i = 0; i < infiniteSequence->GetLength(); i++) {
            log("  [" + QString::number(i + start) + "] = " + QString::number(infiniteSequence->Get(i)));
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::infiniteMapSquare()
{
    if (!infiniteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        log("Исходная бесконечная последовательность (первые 5 элементов):");
        for (int i = 0; i < 5; i++) {
            log("  [" + QString::number(i) + "] = " + QString::number(infiniteSequence->Get(i)));
        }

        // Создаем новую последовательность с квадратами
        auto squaredSeq = infiniteSequence->Map<int>([](int x) { return x * x; });

        log("\nПоследовательность квадратов (первые 5 элементов):");
        for (int i = 0; i < 5; i++) {
            int original = infiniteSequence->Get(i);
            int squared = squaredSeq->Get(i);
            log("  [" + QString::number(i) + "] = " + QString::number(original) +
                "² = " + QString::number(squared));
        }

        log("\nПример для больших индексов:");
        log("  [10] = " + QString::number(infiniteSequence->Get(10)) +
            "² = " + QString::number(squaredSeq->Get(10)));
        log("  [50] = " + QString::number(infiniteSequence->Get(50)) +
            "² = " + QString::number(squaredSeq->Get(50)));

        delete squaredSeq;

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::infiniteFilterEvenOdd()
{
    QString filterType = getParamValue("paramFilter");
    if (filterType.isEmpty()) {
        log("ОШИБКА: Выберите тип фильтра");
        return;
    }

    bool filterEven = filterType.contains("Чётные");

    if (!infiniteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    log("Фильтр: " + filterType);
    log("Первые 10 подходящих элементов бесконечной последовательности:");

    try {
        int found = 0;
        int index = 0;

        while (found < 10) {
            int value = infiniteSequence->Get(index);
            if ((filterEven && value % 2 == 0) || (!filterEven && value % 2 != 0)) {
                log("  [" + QString::number(index) + "] = " + QString::number(value));
                found++;
            }
            index++;
        }

        log("Всего проверено элементов: " + QString::number(index));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::infiniteReduceSumProduct()
{
    QString operationType = getParamValue("paramOperation");
    QString countStr = getParamValue("paramCount");

    if (operationType.isEmpty() || countStr.isEmpty()) {
        log("ОШИБКА: Укажите операцию и количество элементов");
        return;
    }

    bool ok;
    int count = countStr.toInt(&ok);
    if (!ok || count <= 0) {
        log("ОШИБКА: Некорректное количество элементов. Должно быть положительное целое число");
        return;
    }

    bool isSum = operationType.contains("Сумма");

    if (!infiniteSequence) {
        log("ОШИБКА: Последовательность не инициализирована");
        return;
    }

    try {
        log("Операция: " + operationType + " первых " + QString::number(count) + " элементов");
        log("Первые 5 элементов:");
        for (int i = 0; i < min(5, count); i++) {
            log("  " + QString::number(infiniteSequence->Get(i)));
        }
        if (count > 5) {
            log("  ... и еще " + QString::number(count - 5) + " элементов");
        }

        // Вычисляем результат для ограниченного количества элементов
        long long result;
        QString opSymbol;

        if (isSum) {
            result = 0;
            opSymbol = "сумма";
            for (int i = 0; i < count; i++) {
                result += infiniteSequence->Get(i);
            }
        } else {
            result = 1;
            opSymbol = "произведение";
            for (int i = 0; i < count; i++) {
                result *= infiniteSequence->Get(i);
            }
        }

        log("\n" + operationType + " первых " + QString::number(count) +
            " элементов: " + QString::number(result));

        if (!isSum && count > 20) {
            log("Предупреждение: Произведение большого количества элементов может вызвать переполнение");
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

// =============== ОПЕРАЦИИ ДЛЯ ПОТОКА ===============

void MainWindow::streamReadOne()
{
    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        if (stream->IsEndOfStream()) {
            log("Достигнут конец потока");
            log("Текущая позиция: " + QString::number(stream->GetPosition()));
            return;
        }

        char value = stream->Read();
        QString charStr = (value == '\n') ? "\\n" :
                              (value == '\t') ? "\\t" :
                              (value == '\r') ? "\\r" :
                              (value == ' ') ? "[пробел]" : QString(QChar(value));

        log("Прочитан символ: " + charStr + " (код ASCII: " + QString::number((int)value) + ")");
        log("Текущая позиция: " + QString::number(stream->GetPosition()));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamReadAll()
{
    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        // Сохраняем текущую позицию
        size_t oldPos = stream->GetPosition();

        // Переходим в начало
        stream->Seek(0);

        QString result;
        size_t count = 0;
        const size_t MAX_CHARS = 500;

        while (!stream->IsEndOfStream() && count < MAX_CHARS) {
            char value = stream->Read();

            // Заменяем специальные символы для лучшей читаемости
            if (value == '\n') result += "\\n\n";
            else if (value == '\t') result += "\\t";
            else if (value == '\r') result += "\\r";
            else if (value == ' ') result += " ";
            else if (isprint(value)) result += QChar(value);
            else result += QString("[%1]").arg((int)value, 3, 10, QChar('0'));

            count++;
        }

        // Восстанавливаем позицию
        if (oldPos < count) {
            stream->Seek(oldPos);
        } else {
            stream->Seek(0);
        }

        log("Прочитано " + QString::number(count) + " символов:");
        log(result);

        if (count == MAX_CHARS && !stream->IsEndOfStream()) {
            log("... [поток обрезан, показано первых " + QString::number(MAX_CHARS) + " символов]");
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamWriteOne()
{
    QString charStr = getParamValue("paramChar");
    if (charStr.isEmpty()) {
        log("ОШИБКА: Укажите символ для записи");
        return;
    }

    if (charStr.length() != 1) {
        log("ОШИБКА: Должен быть указан ровно один символ");
        return;
    }

    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        char ch = charStr[0].toLatin1();
        size_t pos = stream->Write(ch);

        QString displayChar = (ch == '\n') ? "\\n" :
                                  (ch == '\t') ? "\\t" :
                                  (ch == '\r') ? "\\r" :
                                  (ch == ' ') ? "[пробел]" : QString(QChar(ch));

        log("Записан символ: " + displayChar + " (код ASCII: " + QString::number((int)ch) + ")");
        log("Новая позиция в потоке: " + QString::number(pos));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamWriteAll()
{
    QString data = getCurrentData();
    if (data.isEmpty()) {
        log("ОШИБКА: Нет данных для записи");
        return;
    }

    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        // Сохраняем текущую позицию
        size_t oldPos = stream->GetPosition();

        // Переходим в начало для записи
        stream->Seek(0);

        // Создаем последовательность символов
        auto array = make_shared<DynamicArray<char>>(data.length());
        for (int i = 0; i < data.length(); i++) {
            array->Set(i, data[i].toLatin1());
        }

        auto seq = make_shared<ArraySequence<char>>(*array);

        // Записываем все
        size_t newPos = stream->WriteAll(seq);

        log("Записано " + QString::number(data.length()) + " символов");
        log("Новая позиция в потоке: " + QString::number(newPos));

        // Показываем часть записанных данных
        QString preview = data.left(100);
        if (data.length() > 100) {
            preview += "... [еще " + QString::number(data.length() - 100) + " символов]";
        }
        log("Данные: " + preview);

        // Восстанавливаем позицию, если возможно
        if (oldPos < data.length()) {
            stream->Seek(oldPos);
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamIsEnd()
{
    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    bool isEnd = stream->IsEndOfStream();
    log(isEnd ? "Достигнут конец потока" : "Поток не закончился");
    log("Текущая позиция: " + QString::number(stream->GetPosition()));
}

void MainWindow::streamGetAt()
{
    QString indexStr = getParamValue("paramIndex");
    if (indexStr.isEmpty()) {
        log("ОШИБКА: Укажите индекс");
        return;
    }

    bool ok;
    int index = indexStr.toInt(&ok);
    if (!ok || index < 0) {
        log("ОШИБКА: Некорректный индекс. Должно быть неотрицательное целое число");
        return;
    }

    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        // Получаем данные потока
        auto readData = stream->GetReadData();
        if (!readData) {
            log("ОШИБКА: Нет данных для чтения");
            return;
        }

        char value = readData->Get(index);
        QString charStr = (value == '\n') ? "\\n" :
                              (value == '\t') ? "\\t" :
                              (value == '\r') ? "\\r" :
                              (value == ' ') ? "[пробел]" : QString(QChar(value));

        log("Символ с индексом " + QString::number(index) + ": " +
            charStr + " (код ASCII: " + QString::number((int)value) + ")");

    } catch (const out_of_range &) {
        log("ОШИБКА: Индекс " + QString::number(index) + " выходит за пределы данных");
    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamSeek()
{
    QString posStr = getParamValue("paramPosition");
    if (posStr.isEmpty()) {
        log("ОШИБКА: Укажите позицию");
        return;
    }

    bool ok;
    size_t position = posStr.toUInt(&ok);
    if (!ok) {
        log("ОШИБКА: Некорректная позиция. Должно быть неотрицательное целое число");
        return;
    }

    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        size_t newPos = stream->Seek(position);
        log("Позиция изменена на: " + QString::number(newPos));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamGetLength()
{
    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        size_t bufferSize = stream->GetWriteBufferSize();
        log("Длина буфера записи: " + QString::number(bufferSize));

        // Пытаемся получить информацию о данных чтения
        auto readData = stream->GetReadData();
        if (readData) {
            try {
                size_t dataLength = readData->GetLength();
                log("Длина данных для чтения: " + QString::number(dataLength));
            } catch (const runtime_error &) {
                log("Данные для чтения: бесконечный поток или неизвестная длина");
            }
        }

        log("Текущая позиция: " + QString::number(stream->GetPosition()));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamGetBuffer()
{
    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        auto buffer = stream->GetWriteBuffer();
        if (!buffer) {
            log("Буфер записи не инициализирован");
            return;
        }

        size_t bufferSize = buffer->GetSize();
        log("Содержимое буфера записи (размер: " + QString::number(bufferSize) + "):");

        if (bufferSize == 0) {
            log("  [буфер пуст]");
            return;
        }

        QString bufferStr;
        size_t displayCount = min(bufferSize, (size_t)100);

        for (size_t i = 0; i < displayCount; i++) {
            char ch = buffer->Get(i);

            // Заменяем специальные символы для лучшей читаемости
            if (ch == '\n') bufferStr += "\\n";
            else if (ch == '\t') bufferStr += "\\t";
            else if (ch == '\r') bufferStr += "\\r";
            else if (isprint(ch)) bufferStr += QChar(ch);
            else bufferStr += ".";
        }

        log("  " + bufferStr);

        if (bufferSize > displayCount) {
            log("  ... [еще " + QString::number(bufferSize - displayCount) + " символов]");
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamGetData()
{
    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        auto readData = stream->GetReadData();
        if (!readData) {
            log("Данные для чтения не инициализированы");
            return;
        }

        log("Данные для чтения из потока:");

        // Пытаемся получить длину
        try {
            size_t dataLength = readData->GetLength();
            log("  Длина: " + QString::number(dataLength));

            // Показываем первые 100 символов
            QString dataStr;
            size_t displayCount = min(dataLength, (size_t)100);

            for (size_t i = 0; i < displayCount; i++) {
                char ch = readData->Get(i);

                if (ch == '\n') dataStr += "\\n";
                else if (ch == '\t') dataStr += "\\t";
                else if (ch == '\r') dataStr += "\\r";
                else if (isprint(ch)) dataStr += QChar(ch);
                else dataStr += ".";
            }

            log("  " + dataStr);

            if (dataLength > displayCount) {
                log("  ... [еще " + QString::number(dataLength - displayCount) + " символов]");
            }

        } catch (const runtime_error &) {
            log("  Тип: бесконечный поток или поток неизвестной длины");
            log("  Материализовано символов: " + QString::number(readData->GetMaterializedCount()));

            // Показываем первые 100 символов
            QString dataStr;
            size_t displayCount = 100;

            for (size_t i = 0; i < displayCount; i++) {
                try {
                    char ch = readData->Get(i);

                    if (ch == '\n') dataStr += "\\n";
                    else if (ch == '\t') dataStr += "\\t";
                    else if (ch == '\r') dataStr += "\\r";
                    else if (isprint(ch)) dataStr += QChar(ch);
                    else dataStr += ".";
                } catch (const exception &) {
                    break;
                }
            }

            log("  Первые " + QString::number(displayCount) + " символов:");
            log("  " + dataStr);
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamEncode()
{
    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        // Получаем данные для кодирования
        auto readData = stream->GetReadData();
        if (!readData) {
            log("ОШИБКА: Нет данных для кодирования");
            return;
        }

        // Создаем поток для чтения
        auto readStream = make_shared<ReadOnlyStream<char>>(readData);
        readStream->Open();

        // Создаем буфер для результата
        auto outputBuffer = make_shared<DynamicArray<string>>(0);
        auto writeStream = make_shared<WriteOnlyStream<string>>(outputBuffer);
        writeStream->Open();

        // Выполняем RLE кодирование
        StreamEncoder::RLEEncode(readStream, writeStream);

        readStream->Close();
        writeStream->Close();

        // Выводим результат
        log("Результат RLE кодирования:");

        if (outputBuffer->GetSize() == 0) {
            log("  [нет данных]");
            return;
        }

        QString encodedResult;
        size_t groupCount = 0;
        const size_t MAX_GROUPS = 20;

        for (size_t i = 0; i < outputBuffer->GetSize() && groupCount < MAX_GROUPS; i++) {
            encodedResult += QString::fromStdString(outputBuffer->Get(i)) + " ";
            groupCount++;
        }

        if (outputBuffer->GetSize() > MAX_GROUPS) {
            encodedResult += "... [еще " + QString::number(outputBuffer->GetSize() - MAX_GROUPS) + " групп]";
        }

        log("  " + encodedResult);
        log("  Количество групп: " + QString::number(outputBuffer->GetSize()));

        // Расчет сжатия
        try {
            size_t originalLength = readData->GetLength();
            size_t encodedSize = outputBuffer->GetSize();

            if (originalLength > 0) {
                double compressionRatio = (1.0 - (double)encodedSize * 2 / originalLength) * 100;
                log("  Эффективность сжатия: " + QString::number(compressionRatio, 'f', 1) + "%");
                log("  Исходный размер: " + QString::number(originalLength) + " символов");
                log("  Закодированный размер: ~" + QString::number(encodedSize * 2) + " символов");
            }
        } catch (const runtime_error &) {
            log("  Примечание: невозможно рассчитать сжатие для потока неизвестной длины");
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamDecode()
{
    QString data = getCurrentData();
    if (data.isEmpty()) {
        log("ОШИБКА: Нет данных для декодирования");
        return;
    }

    try {
        // Парсим группы RLE
        QStringList groups;
        QString currentGroup;

        for (int i = 0; i < data.length(); i++) {
            QChar ch = data[i];

            if (ch.isDigit()) {
                currentGroup += ch;
            } else if (!ch.isSpace()) {
                currentGroup += ch;
                groups.append(currentGroup);
                currentGroup.clear();
            } else if (!currentGroup.isEmpty()) {
                // Если пробел и группа не пустая, сохраняем ее
                groups.append(currentGroup);
                currentGroup.clear();
            }
        }

        if (!currentGroup.isEmpty()) {
            groups.append(currentGroup);
        }

        if (groups.isEmpty()) {
            log("ОШИБКА: Не найдено групп RLE для декодирования");
            return;
        }

        log("Найдено групп RLE: " + QString::number(groups.size()));
        log("Первые 5 групп:");
        for (int i = 0; i < std::min(5, (int)groups.size()); i++) {
            log("  " + groups[i]);
        }
        if (groups.size() > 5) {
            log("  ... [еще " + QString::number(groups.size() - 5) + " групп]");
        }

        // Создаем последовательность строк
        auto array = make_shared<DynamicArray<string>>(groups.size());
        for (int i = 0; i < groups.size(); i++) {
            array->Set(i, groups[i].toStdString());
        }

        auto encodedSeq = make_shared<ArraySequence<string>>(*array);
        auto readStream = make_shared<ReadOnlyStream<string>>(encodedSeq);
        readStream->Open();

        // Создаем буфер для результата
        auto outputBuffer = make_shared<DynamicArray<char>>(0);
        auto writeStream = make_shared<WriteOnlyStream<char>>(outputBuffer);
        writeStream->Open();

        // Выполняем RLE декодирование
        StreamEncoder::RLEDecode(readStream, writeStream);

        readStream->Close();
        writeStream->Close();

        // Выводим результат
        log("\nРезультат RLE декодирования:");

        if (outputBuffer->GetSize() == 0) {
            log("  [нет данных]");
            return;
        }

        QString decodedResult;
        size_t displayCount = min(outputBuffer->GetSize(), (size_t)200);

        for (size_t i = 0; i < displayCount; i++) {
            char ch = outputBuffer->Get(i);

            if (ch == '\n') decodedResult += "\\n";
            else if (ch == '\t') decodedResult += "\\t";
            else if (ch == '\r') decodedResult += "\\r";
            else if (isprint(ch)) decodedResult += QChar(ch);
            else decodedResult += ".";
        }

        log("  " + decodedResult);

        if (outputBuffer->GetSize() > displayCount) {
            log("  ... [еще " + QString::number(outputBuffer->GetSize() - displayCount) + " символов]");
        }

        log("  Длина результата: " + QString::number(outputBuffer->GetSize()) + " символов");

        // Пример соответствия групп
        log("\nПример соответствия групп:");
        for (int i = 0; i < std::min(3, (int)groups.size()); i++) {
            QString group = groups[i];
            if (group.length() > 0) {
                // Извлекаем число и символ
                QString numStr;
                QChar symbol;

                for (int j = 0; j < group.length(); j++) {
                    QChar ch = group[j];
                    if (ch.isDigit()) {
                        numStr += ch;
                    } else {
                        symbol = ch;
                        break;
                    }
                }

                if (!numStr.isEmpty() && !symbol.isNull()) {
                    bool ok;
                    int count = numStr.toInt(&ok);
                    if (ok && count > 0) {
                        QString decoded = QString(count, symbol);
                        if (decoded.length() > 20) {
                            decoded = decoded.left(20) + "...";
                        }
                        log("  " + group + " → " + decoded + " (" + QString::number(count) + " раз)");
                    }
                }
            }
        }

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}

void MainWindow::streamStats()
{
    if (!stream) {
        log("ОШИБКА: Поток не инициализирован");
        return;
    }

    try {
        auto readData = stream->GetReadData();
        if (!readData) {
            log("ОШИБКА: Нет данных для анализа");
            return;
        }

        log("Статистика потока:");

        // Собираем статистику вручную
        size_t count = 0;
        long long sum = 0;
        char minChar = 0, maxChar = 0;
        long long sumSquares = 0;
        const size_t MAX_ELEMENTS = 1000;

        try {
            while (count < MAX_ELEMENTS) {
                try {
                    char ch = readData->Get(count);
                    int intValue = static_cast<int>(ch);

                    if (count == 0) {
                        minChar = maxChar = ch;
                    } else {
                        if (intValue < static_cast<int>(minChar)) minChar = ch;
                        if (intValue > static_cast<int>(maxChar)) maxChar = ch;
                    }

                    sum += intValue;
                    sumSquares += intValue * intValue;
                    count++;

                } catch (const out_of_range &) {
                    break;
                } catch (const runtime_error &) {
                    // Для бесконечных потоков
                    if (count == 0) {
                        log("  ОШИБКА: Не удалось получить данные из потока");
                        return;
                    }
                    break;
                }
            }
        } catch (...) {
            // Игнорируем другие исключения
        }

        if (count == 0) {
            log("  Нет данных для анализа");
            return;
        }

        log("  Проанализировано символов: " + QString::number(count));
        log("  Сумма кодов ASCII: " + QString::number(sum));
        log("  Средний код ASCII: " + QString::number(static_cast<double>(sum) / count, 'f', 2));

        QString minStr = (minChar == '\n') ? "\\n" :
                             (minChar == '\t') ? "\\t" :
                             (minChar == '\r') ? "\\r" :
                             (minChar == ' ') ? "[пробел]" : QString(QChar(minChar));

        QString maxStr = (maxChar == '\n') ? "\\n" :
                             (maxChar == '\t') ? "\\t" :
                             (maxChar == '\r') ? "\\r" :
                             (maxChar == ' ') ? "[пробел]" : QString(QChar(maxChar));

        log("  Минимальный символ: " + minStr + " (код: " + QString::number((int)minChar) + ")");
        log("  Максимальный символ: " + maxStr + " (код: " + QString::number((int)maxChar) + ")");

        if (count > 1) {
            double variance = static_cast<double>(sumSquares) / count -
                              pow(static_cast<double>(sum) / count, 2);
            double stdDev = sqrt(variance);
            log("  Стандартное отклонение: " + QString::number(stdDev, 'f', 2));
        }

        // Дополнительная информация
        log("\n  Дополнительная информация:");
        log("    Текущая позиция в потоке: " + QString::number(stream->GetPosition()));

        auto buffer = stream->GetWriteBuffer();
        if (buffer) {
            log("    Размер буфера записи: " + QString::number(buffer->GetSize()));
        }

        log("    Материализовано символов: " + QString::number(readData->GetMaterializedCount()));

    } catch (const exception &e) {
        log("ОШИБКА: " + QString::fromStdString(e.what()));
    }
}
