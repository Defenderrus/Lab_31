#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <memory>

template<typename T> class LazySequence;
template<typename T> class ReadWriteStream;

QT_BEGIN_NAMESPACE
namespace Ui {
class MainWindow;
}
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // Слоты для переключения режимов
    void onModeChanged();

    // Слоты для работы с данными
    void onLoadFileClicked();
    void onInitializeClicked();
    void onClearInputClicked();
    void onClearOutputClicked();
    void onSaveOutputClicked();

    // Слоты для операций
    void onOperationChanged(int index);
    void onExecuteClicked();

    // Слоты для динамических параметров
    void onParam1Changed();
    void onParam2Changed();

private:
    Ui::MainWindow *ui;

    // Текущий режим работы
    enum class Mode { Finite, Infinite, Stream };
    Mode currentMode;

    // Объекты для каждого режима
    std::shared_ptr<LazySequence<int>> finiteSequence;
    std::shared_ptr<LazySequence<int>> infiniteSequence;
    std::shared_ptr<ReadWriteStream<char>> stream;

    // Вспомогательные методы
    void log(const QString &message);
    void clearOutput();
    void updateOperationsList();
    void clearParams();
    void showParams();
    void updateParamHints();

    // Методы для работы с последовательностями
    void initFiniteSequence();
    void initInfiniteSequence();
    void initStream();

    // Методы инициализации объектов из входных данных
    void initializeFiniteSequence();
    void initializeInfiniteSequence();
    void initializeStream();

    // Валидация данных
    bool validateInputData();
    std::vector<int> parseNumbers();
    QString getCurrentData();
    QString getParamValue(const QString &paramName);

    // Операции для конечной последовательности
    void finiteGetFirst();
    void finiteGetLast();
    void finiteGetAt();
    void finiteGetLength();
    void finiteAppend();
    void finitePrepend();
    void finiteInsertAt();
    void finiteRemove();
    void finiteConcat();
    void finiteSubsequence();
    void finiteMapSquare();
    void finiteFilterEvenOdd();
    void finiteReduceSumProduct();

    // Операции для бесконечной последовательности
    void infiniteGetFirst();
    void infiniteGetAt();
    void infiniteGetLength();
    void infiniteSubsequence();
    void infiniteMapSquare();
    void infiniteFilterEvenOdd();
    void infiniteReduceSumProduct();

    // Операции для потока
    void streamReadOne();
    void streamReadAll();
    void streamWriteOne();
    void streamWriteAll();
    void streamIsEnd();
    void streamGetAt();
    void streamSeek();
    void streamGetLength();
    void streamGetBuffer();
    void streamGetData();
    void streamEncode();
    void streamDecode();
    void streamStats();
};

#endif // MAINWINDOW_H
