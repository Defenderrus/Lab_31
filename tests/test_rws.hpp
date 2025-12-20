#ifndef TEST_RWS_HPP
#define TEST_RWS_HPP

#include <gtest/gtest.h>
#include <cstdio>
#include <sstream>
#include <sys/stat.h>
#include <random>
#include "../Stream.hpp"
#include "../sequences/ArraySequence.hpp"
using namespace std;

// Вспомогательная функция для проверки существования файла
inline bool fileExists(const string& filename) {
    struct stat buffer;
    return (stat(filename.c_str(), &buffer) == 0);
}

// Тесты для Stream
class StreamTest: public testing::Test {
protected:
    string testFilename = "test_stream.tmp";
    string testLargeFile = "test_large.tmp";
    string testWriteFile = "test_write.tmp";
    
    void SetUp() override {
        // Очистка файлов перед каждым тестом
        remove_if_exists(testFilename);
        remove_if_exists(testLargeFile);
        remove_if_exists(testWriteFile);
    }
    
    void TearDown() override {
        // Очистка файлов после каждого теста
        remove_if_exists(testFilename);
        remove_if_exists(testLargeFile);
        remove_if_exists(testWriteFile);
    }
    
    void remove_if_exists(const string& filename) {
        if (fileExists(filename)) {
            remove(filename.c_str());
        }
    }
    
    void CreateTestFile(const vector<string> &lines) {
        ofstream file(testFilename);
        for (const auto &line : lines) {
            file << line << endl;
        }
        file.close();
    }
    
    void CreateLargeTestFile(int lineCount) {
        ofstream file(testLargeFile);
        for (int i = 0; i < lineCount; i++) {
            file << i << endl;
        }
        file.close();
    }
    
    vector<string> ReadFileContents(const string &filename) {
        ifstream file(filename);
        vector<string> lines;
        string line;
        while (getline(file, line)) {
            lines.push_back(line);
        }
        file.close();
        return lines;
    }
    
    // Генерация случайных чисел для тестов
    int random_int(int min, int max) {
        static random_device rd;
        static mt19937 gen(rd());
        uniform_int_distribution<> dis(min, max);
        return dis(gen);
    }
};

// 1. Тест: Базовые операции ReadOnlyStream
TEST_F(StreamTest, ReadOnlyStream_BasicOperations) {
    auto seq = make_shared<ArraySequence<int>>();
    for (int i = 1; i <= 10; i++) {
        seq->Append(i * 10);
    }
    
    ReadOnlyStream<int> stream(seq);
    stream.Open();
    
    EXPECT_TRUE(stream.IsOpen());
    EXPECT_FALSE(stream.IsEndOfStream());
    EXPECT_TRUE(stream.IsCanSeek());
    EXPECT_TRUE(stream.IsCanGoBack());
    
    EXPECT_EQ(stream.Read(), 10);
    EXPECT_EQ(stream.Read(), 20);
    EXPECT_EQ(stream.GetPosition(), 2);
    
    EXPECT_EQ(stream.Peek(), 30);
    EXPECT_EQ(stream.GetPosition(), 2);
    EXPECT_EQ(stream.Read(), 30);
    
    EXPECT_EQ(stream.Seek(5), 5);
    EXPECT_EQ(stream.Read(), 60);
    EXPECT_EQ(stream.GetPosition(), 6);
    
    stream.Close();
    EXPECT_FALSE(stream.IsOpen());
}

// 2. Тест: Работа с файлами (ReadOnlyStream)
TEST_F(StreamTest, ReadOnlyStream_FileOperations) {
    CreateTestFile({"100", "200", "300", "400", "500"});
    
    auto deserializer = make_shared<IntDeserializer>();
    ReadOnlyStream<int> stream(testFilename, deserializer);
    stream.Open();
    
    EXPECT_EQ(stream.Read(), 100);
    EXPECT_EQ(stream.Read(), 200);
    EXPECT_EQ(stream.Read(), 300);
    
    stream.Seek(1);
    EXPECT_EQ(stream.Read(), 200);
    
    stream.Seek(4);
    EXPECT_EQ(stream.Read(), 500);
    EXPECT_TRUE(stream.IsEndOfStream());
    
    stream.Close();
}

// 3. Тест: Пустые последовательности и файлы
TEST_F(StreamTest, EdgeCases_EmptySequences) {
    // Пустая последовательность
    auto emptySeq = make_shared<ArraySequence<int>>();
    ReadOnlyStream<int> stream1(emptySeq);
    stream1.Open();
    
    EXPECT_TRUE(stream1.IsEndOfStream());
    EXPECT_THROW(stream1.Read(), runtime_error);
    EXPECT_THROW(stream1.Peek(), runtime_error);
    stream1.Close();
    
    // Пустой файл
    CreateTestFile({});
    auto deserializer = make_shared<IntDeserializer>();
    ReadOnlyStream<int> stream2(testFilename, deserializer);
    stream2.Open();
    
    EXPECT_TRUE(stream2.IsEndOfStream());
    stream2.Close();
    
    // Пустой буфер
    auto emptyBuffer = make_shared<DynamicArray<int>>(0);
    WriteOnlyStream<int> stream3(emptyBuffer);
    stream3.Open();
    
    stream3.Write(100);
    EXPECT_EQ(emptyBuffer->GetSize(), 1);
    EXPECT_EQ(emptyBuffer->Get(0), 100);
    stream3.Close();
}

// 4. Тест: Ошибки при некорректных операциях
TEST_F(StreamTest, EdgeCases_InvalidOperations) {
    auto seq = make_shared<ArraySequence<int>>();
    seq->Append(1);
    
    ReadOnlyStream<int> stream(seq);
    
    // Операции без открытия
    EXPECT_THROW(stream.Read(), runtime_error);
    EXPECT_THROW(stream.Peek(), runtime_error);
    EXPECT_THROW(stream.Seek(0), runtime_error);
    
    stream.Open();
    stream.Close();
    
    // Операции после закрытия
    EXPECT_THROW(stream.Read(), runtime_error);
    EXPECT_THROW(stream.Peek(), runtime_error);
}

// 5. Тест: Seek на границах
TEST_F(StreamTest, EdgeCases_SeekBoundaries) {
    auto seq = make_shared<ArraySequence<int>>();
    for (int i = 0; i < 10; i++) {
        seq->Append(i * 10);
    }
    
    ReadOnlyStream<int> stream(seq);
    stream.Open();
    
    EXPECT_EQ(stream.Seek(0), 0);
    EXPECT_EQ(stream.Read(), 0);
    
    EXPECT_EQ(stream.Seek(9), 9);
    EXPECT_EQ(stream.Read(), 90);
    EXPECT_TRUE(stream.IsEndOfStream());
    
    EXPECT_THROW(stream.Seek(10), out_of_range);
    EXPECT_THROW(stream.Seek(100), out_of_range);
    
    stream.Close();
}

// 6. Тест: Чтение строк с разделителями
TEST_F(StreamTest, ReadOnlyStream_StringOperations) {
    string data = "apple,banana,cherry,date,elderberry";
    auto deserializer = make_shared<StringDeserializer>();
    
    ReadOnlyStream<string> stream(data, deserializer, ',');
    stream.Open();
    
    EXPECT_EQ(stream.Read(), "apple");
    EXPECT_EQ(stream.Read(), "banana");
    EXPECT_EQ(stream.Read(), "cherry");
    EXPECT_EQ(stream.GetPosition(), 3);
    
    stream.Seek(4);
    EXPECT_EQ(stream.Read(), "elderberry");
    EXPECT_TRUE(stream.IsEndOfStream());
    
    stream.Close();
}

// 7. Тест: ReadBlock операции
TEST_F(StreamTest, ReadOnlyStream_ReadBlockOperations) {
    auto seq = make_shared<ArraySequence<int>>();
    for (int i = 0; i < 10; i++) {
        seq->Append(i * 10);
    }
    
    ReadOnlyStream<int> stream(seq);
    stream.Open();
    
    // Чтение блока
    auto block1 = stream.ReadBlock(5);
    EXPECT_EQ(block1->GetSize(), 5);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(block1->Get(i), i * 10);
    }
    EXPECT_EQ(stream.GetPosition(), 5);
    
    // Чтение оставшегося блока
    auto block2 = stream.ReadBlock(10);
    EXPECT_EQ(block2->GetSize(), 5);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(block2->Get(i), (i + 5) * 10);
    }
    EXPECT_TRUE(stream.IsEndOfStream());
    
    stream.Close();
}

// 8. Тест: WriteOnlyStream базовые операции
TEST_F(StreamTest, WriteOnlyStream_BasicOperations) {
    auto buffer = make_shared<DynamicArray<int>>(10);
    WriteOnlyStream<int> stream(buffer);
    stream.Open();
    
    EXPECT_TRUE(stream.IsOpen());
    EXPECT_EQ(stream.GetPosition(), 0);
    
    stream.Write(100);
    stream.Write(200);
    stream.Write(300);
    
    EXPECT_EQ(stream.GetPosition(), 3);
    EXPECT_EQ(buffer->GetSize(), 10);
    EXPECT_EQ(buffer->Get(0), 100);
    EXPECT_EQ(buffer->Get(1), 200);
    EXPECT_EQ(buffer->Get(2), 300);
    
    // Запись больше размера буфера
    for (int i = 4; i <= 15; i++) {
        stream.Write(i * 100);
    }
    
    EXPECT_EQ(stream.GetPosition(), 15);
    EXPECT_GE(buffer->GetSize(), 15);
    
    stream.Close();
    EXPECT_FALSE(stream.IsOpen());
}

// 9. Тест: Запись в файл
TEST_F(StreamTest, WriteOnlyStream_FileOperations) {
    auto serializer = make_shared<IntSerializer>();
    WriteOnlyStream<int> stream(testFilename, serializer);
    stream.Open();
    
    for (int i = 1; i <= 5; i++) {
        stream.Write(i * 100);
    }
    
    stream.Close();
    
    auto lines = ReadFileContents(testFilename);
    ASSERT_EQ(lines.size(), 5);
    EXPECT_EQ(lines[0], "100");
    EXPECT_EQ(lines[1], "200");
    EXPECT_EQ(lines[2], "300");
    EXPECT_EQ(lines[3], "400");
    EXPECT_EQ(lines[4], "500");
}

// 10. Тест: Пакетные операции записи
TEST_F(StreamTest, WriteOnlyStream_BatchOperations) {
    auto buffer = make_shared<DynamicArray<int>>(20);
    WriteOnlyStream<int> stream(buffer);
    stream.Open();
    
    // WriteAll
    auto seq = make_shared<ArraySequence<int>>();
    for (int i = 1; i <= 10; i++) {
        seq->Append(i * 10);
    }
    
    size_t pos = stream.WriteAll(seq);
    EXPECT_EQ(pos, 10);
    EXPECT_EQ(stream.GetPosition(), 10);
    
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(buffer->Get(i), (i + 1) * 10);
    }
    
    // WriteBlock
    auto block = make_shared<DynamicArray<int>>(5);
    for (int i = 0; i < 5; i++) {
        block->Set(i, (i + 1) * 100);
    }
    
    pos = stream.WriteBlock(block);
    EXPECT_EQ(pos, 15);
    EXPECT_EQ(stream.GetPosition(), 15);
    
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(buffer->Get(i + 10), (i + 1) * 100);
    }
    
    stream.Close();
}

// 11. Тест: ReadWriteStream базовые операции
TEST_F(StreamTest, ReadWriteStream_BasicOperations) {
    auto readSeq = make_shared<ArraySequence<int>>();
    for (int i = 1; i <= 6; i++) {
        readSeq->Append(i * 10);
    }
    
    auto lazySeq = make_shared<LazySequence<int>>(readSeq.get());
    auto writeBuffer = make_shared<DynamicArray<int>>(10);
    
    ReadWriteStream<int> stream(lazySeq, writeBuffer);
    stream.Open();
    
    // Чтение
    EXPECT_EQ(stream.Read(), 10);
    EXPECT_EQ(stream.Read(), 20);
    EXPECT_EQ(stream.GetPosition(), 2);
    
    // Запись
    stream.Write(100);
    stream.Write(200);
    EXPECT_EQ(stream.GetPosition(), 4);
    
    // Продолжение чтения
    stream.Seek(2);
    EXPECT_EQ(stream.Read(), 30);
    EXPECT_EQ(stream.Read(), 40);
    EXPECT_EQ(stream.Read(), 50);
    EXPECT_EQ(stream.Read(), 60);
    EXPECT_TRUE(stream.IsEndOfStream());
    
    // Продолжение записи
    stream.Seek(5);
    stream.Write(300);
    EXPECT_EQ(stream.GetPosition(), 6);
    
    // Проверка буфера
    EXPECT_GE(writeBuffer->GetSize(), 6);
    EXPECT_EQ(writeBuffer->Get(2), 100);
    EXPECT_EQ(writeBuffer->Get(3), 200);
    EXPECT_EQ(writeBuffer->Get(5), 300);
    
    stream.Close();
}

// 12. Тест: ReadWriteStream файловые операции (чтение и запись)
TEST_F(StreamTest, ReadWriteStream_FileOperations_ReadAndWrite) {
    CreateTestFile({"1", "2", "3", "4", "5"});
    
    auto deserializer = make_shared<IntDeserializer>();
    auto serializer = make_shared<IntSerializer>();
    
    ReadWriteStream<int> stream(testFilename, deserializer, serializer, ios::in | ios::out);
    stream.Open();
    
    // Чтение
    EXPECT_EQ(stream.Read(), 1);
    EXPECT_EQ(stream.Read(), 2);
    EXPECT_EQ(stream.GetPosition(), 2);
    
    // Переход и запись
    stream.Seek(2);
    stream.Write(300);  // Заменяем 3 на 300
    EXPECT_EQ(stream.GetPosition(), 3);
    
    // Продолжение чтения
    EXPECT_EQ(stream.Read(), 4);
    EXPECT_EQ(stream.Read(), 5);
    EXPECT_EQ(stream.GetPosition(), 5);
    EXPECT_TRUE(stream.IsEndOfStream());
    
    stream.Close();
    
    // Проверка файла
    auto lines = ReadFileContents(testFilename);
    ASSERT_EQ(lines.size(), 5);
    EXPECT_EQ(lines[0], "1");
    EXPECT_EQ(lines[1], "2");
    EXPECT_EQ(lines[2], "300");
    EXPECT_EQ(lines[3], "4");
    EXPECT_EQ(lines[4], "5");
}

// 13. Тест: ReadWriteStream добавление в файл
TEST_F(StreamTest, ReadWriteStream_FileOperations_Append) {
    CreateTestFile({"10", "20", "30"});
    
    auto deserializer = make_shared<IntDeserializer>();
    auto serializer = make_shared<IntSerializer>();
    
    ReadWriteStream<int> stream(testFilename, deserializer, serializer, ios::in | ios::out);
    stream.Open();
    
    // Читаем все
    EXPECT_EQ(stream.Read(), 10);
    EXPECT_EQ(stream.Read(), 20);
    EXPECT_EQ(stream.Read(), 30);
    EXPECT_TRUE(stream.IsEndOfStream());
    
    // Переходим в конец и добавляем
    stream.Seek(3);
    stream.Write(40);
    stream.Write(50);
    
    stream.Close();
    
    // Проверка
    auto lines = ReadFileContents(testFilename);
    ASSERT_EQ(lines.size(), 5);
    EXPECT_EQ(lines[0], "10");
    EXPECT_EQ(lines[1], "20");
    EXPECT_EQ(lines[2], "30");
    EXPECT_EQ(lines[3], "40");
    EXPECT_EQ(lines[4], "50");
}

// 14. Тест: Перемешанные операции чтения и записи
TEST_F(StreamTest, EdgeCases_ReadWriteMixed) {
    auto readSeq = make_shared<ArraySequence<int>>();
    for (int i = 0; i < 10; i++) {
        readSeq->Append(i * 10);
    }
    
    auto lazySeq = make_shared<LazySequence<int>>(readSeq.get());
    auto writeBuffer = make_shared<DynamicArray<int>>(10);
    
    ReadWriteStream<int> stream(lazySeq, writeBuffer);
    stream.Open();
    
    // Перемешиваем чтение и запись
    EXPECT_EQ(stream.Read(), 0);
    stream.Write(100);
    EXPECT_EQ(stream.Read(), 20);
    stream.Write(200);
    EXPECT_EQ(stream.Read(), 40);
    
    // Проверяем позицию
    EXPECT_EQ(stream.GetPosition(), 5);
    
    // Проверяем буфер
    EXPECT_EQ(writeBuffer->Get(1), 100);
    EXPECT_EQ(writeBuffer->Get(3), 200);
    
    stream.Close();
}

// 15. Тест: Интеграционный - чтение, преобразование, запись
TEST_F(StreamTest, Integration_ReadTransformWrite) {
    CreateTestFile({"1", "2", "3", "4", "5"});
    
    // Читаем, преобразуем, пишем в другой файл
    auto deserializer = make_shared<IntDeserializer>();
    auto serializer = make_shared<IntSerializer>();
    
    ReadOnlyStream<int> inputStream(testFilename, deserializer);
    WriteOnlyStream<int> outputStream(testWriteFile, serializer);
    
    inputStream.Open();
    outputStream.Open();
    
    while (!inputStream.IsEndOfStream()) {
        try {
            int value = inputStream.Read();
            outputStream.Write(value * 10);
        } catch (...) {
            break;
        }
    }
    
    inputStream.Close();
    outputStream.Close();
    
    // Проверка результата
    auto lines = ReadFileContents(testWriteFile);
    ASSERT_EQ(lines.size(), 5);
    EXPECT_EQ(lines[0], "10");
    EXPECT_EQ(lines[1], "20");
    EXPECT_EQ(lines[2], "30");
    EXPECT_EQ(lines[3], "40");
    EXPECT_EQ(lines[4], "50");
}

// 16. Тест: Фильтрация и обработка данных
TEST_F(StreamTest, Integration_FilterAndProcess) {
    auto seq = make_shared<ArraySequence<int>>();
    for (int i = 1; i <= 20; i++) {
        seq->Append(i);
    }

    auto buffer = make_shared<DynamicArray<int>>(20);
    for (size_t i = 0; i < 20; i++) {
        buffer->Set(i, -1);
    }
    
    ReadOnlyStream<int> stream(seq);
    WriteOnlyStream<int> writeStream(buffer);

    stream.Open();
    writeStream.Open();
    
    // Фильтрация: только четные числа, умноженные на 2
    while (!stream.IsEndOfStream()) {
        try {
            int value = stream.Read();
            if (value % 2 == 0) {
                writeStream.Write(value * 2);
            }
        } catch (...) {
            break;
        }
    }

    EXPECT_EQ(writeStream.GetPosition(), 10);
    
    stream.Close();
    writeStream.Close();
    
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(buffer->Get(i), (i + 1) * 4);
    }
}

// 17. Тест: Копирование файла
TEST_F(StreamTest, Integration_FileCopy) {
    // Создаем исходный файл
    CreateTestFile({"10", "20", "30", "40", "50", "60", "70", "80", "90", "100"});
    
    // Копируем через потоки
    auto deserializer = make_shared<IntDeserializer>();
    auto serializer = make_shared<IntSerializer>();
    
    ReadOnlyStream<int> sourceStream(testFilename, deserializer);
    WriteOnlyStream<int> destStream(testWriteFile, serializer);
    
    sourceStream.Open();
    destStream.Open();
    
    while (!sourceStream.IsEndOfStream()) {
        try {
            int value = sourceStream.Read();
            destStream.Write(value);
        } catch (...) {
            break;
        }
    }
    
    sourceStream.Close();
    destStream.Close();
    
    // Проверяем копию
    auto origLines = ReadFileContents(testFilename);
    auto copyLines = ReadFileContents(testWriteFile);
    
    EXPECT_EQ(origLines.size(), copyLines.size());
    for (size_t i = 0; i < origLines.size(); i++) {
        EXPECT_EQ(origLines[i], copyLines[i]);
    }
}

// 18. Тест: Производительность с большим файлом
TEST_F(StreamTest, Performance_LargeFileRead) {
    const int LINES_COUNT = 10000;
    CreateLargeTestFile(LINES_COUNT);
    
    auto deserializer = make_shared<IntDeserializer>();
    ReadOnlyStream<int> stream(testLargeFile, deserializer);
    stream.Open();
    
    int sum = 0;
    int count = 0;
    while (!stream.IsEndOfStream()) {
        try {
            sum += stream.Read();
            count++;
        } catch (...) {
            break;
        }
    }
    
    EXPECT_EQ(count, LINES_COUNT);
    // Сумма арифметической прогрессии 0..9999
    EXPECT_EQ(sum, (LINES_COUNT - 1) * LINES_COUNT / 2);
    
    stream.Close();
}

// 19. Тест: Производительность записи большого файла
TEST_F(StreamTest, Performance_LargeFileWrite) {
    const int LINES_COUNT = 10000;
    
    auto serializer = make_shared<IntSerializer>();
    WriteOnlyStream<int> stream(testLargeFile, serializer);
    stream.Open();
    
    for (int i = 0; i < LINES_COUNT; i++) {
        stream.Write(i);
    }
    
    stream.Close();
    
    // Проверяем, что файл создан и имеет правильный размер
    EXPECT_TRUE(fileExists(testLargeFile));
    
    auto lines = ReadFileContents(testLargeFile);
    EXPECT_EQ(lines.size(), LINES_COUNT);
    
    // Проверяем несколько значений
    if (lines.size() > 0) EXPECT_EQ(lines[0], "0");
    if (lines.size() > 100) EXPECT_EQ(lines[100], "100");
    if (lines.size() > 9999) EXPECT_EQ(lines[9999], "9999");
}

// 20. Тест: Замена элементов в середине файла
TEST_F(StreamTest, ReadWriteStream_FileOperations_OverwriteMiddle) {
    CreateTestFile({"100", "200", "300", "400", "500"});
    
    auto deserializer = make_shared<IntDeserializer>();
    auto serializer = make_shared<IntSerializer>();
    
    ReadWriteStream<int> stream(testFilename, deserializer, serializer, ios::in | ios::out);
    stream.Open();
    
    // Заменяем несколько элементов в середине
    stream.Seek(1);
    stream.Write(999);
    
    stream.Seek(3);
    stream.Write(888);
    
    stream.Close();
    
    auto lines = ReadFileContents(testFilename);
    ASSERT_EQ(lines.size(), 5);
    EXPECT_EQ(lines[0], "100");
    EXPECT_EQ(lines[1], "999");
    EXPECT_EQ(lines[2], "300");
    EXPECT_EQ(lines[3], "888");
    EXPECT_EQ(lines[4], "500");
}

// Основная функция
inline int run_test_rws() {
    int argc = 1;
    char* argv[] = {(char*)"test_program"};
    testing::InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(filter) = "StreamTest*";
    return RUN_ALL_TESTS();
}

#endif // TEST_RWS_HPP
