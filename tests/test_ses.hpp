#ifndef TEST_SES_HPP
#define TEST_SES_HPP

#include <gtest/gtest.h>
#include <sstream>
#include <chrono>
#include "StreamEncoder.hpp"
#include "StreamStatistics.hpp"
using namespace std;


// Тесты для StreamEncoder
class StreamEncoderTest: public ::testing::Test {
    protected:
        void SetUp() override {
            ofstream file("test_input.txt");
            if (file.is_open()) {
                file << "AAABBBCCCDDD";
                file.close();
            }
        }
        
        void TearDown() override {
            remove("test_input.txt");
            remove("test_output.txt");
            remove("test_decoded.txt");
        }
};

TEST_F(StreamEncoderTest, RLEEncodeSimple) {
    string inputStr = "AAABBBCCCDDD";
    auto array = make_shared<DynamicArray<char>>(inputStr.length());
    for (size_t i = 0; i < inputStr.length(); i++) {
        array->Set(i, inputStr[i]);
    }
    auto seq = make_shared<LazySequence<char>>(array);
    auto input = make_shared<ReadOnlyStream<char>>(seq);
    
    auto outputArray = make_shared<DynamicArray<string>>(0);
    auto output = make_shared<WriteOnlyStream<string>>(outputArray);
    
    StreamEncoder::RLEEncode(input, output);
    
    auto buffer = output->GetBuffer();
    ASSERT_EQ(buffer->GetSize(), 4);
    
    EXPECT_EQ(buffer->Get(0), "3A");
    EXPECT_EQ(buffer->Get(1), "3B");
    EXPECT_EQ(buffer->Get(2), "3C");
    EXPECT_EQ(buffer->Get(3), "3D");
}

TEST_F(StreamEncoderTest, RLEEncodeSingleChar) {
    string inputStr = "A";
    auto array = make_shared<DynamicArray<char>>(1);
    array->Set(0, 'A');
    auto seq = make_shared<LazySequence<char>>(array);
    auto input = make_shared<ReadOnlyStream<char>>(seq);
    
    auto outputArray = make_shared<DynamicArray<string>>(0);
    auto output = make_shared<WriteOnlyStream<string>>(outputArray);
    
    StreamEncoder::RLEEncode(input, output);
    
    auto buffer = output->GetBuffer();
    ASSERT_EQ(buffer->GetSize(), 1);
    EXPECT_EQ(buffer->Get(0), "1A");
}

TEST_F(StreamEncoderTest, RLEEncodeEmpty) {
    auto array = make_shared<DynamicArray<char>>(0);
    auto seq = make_shared<LazySequence<char>>(array);
    auto input = make_shared<ReadOnlyStream<char>>(seq);
    
    auto outputArray = make_shared<DynamicArray<string>>(0);
    auto output = make_shared<WriteOnlyStream<string>>(outputArray);
    
    StreamEncoder::RLEEncode(input, output);
    
    auto buffer = output->GetBuffer();
    EXPECT_EQ(buffer->GetSize(), 0);
}

TEST_F(StreamEncoderTest, RLEEncodeLongRun) {
    string longRun(15, 'X');
    auto array = make_shared<DynamicArray<char>>(longRun.length());
    for (size_t i = 0; i < longRun.length(); i++) {
        array->Set(i, longRun[i]);
    }
    auto seq = make_shared<LazySequence<char>>(array);
    auto input = make_shared<ReadOnlyStream<char>>(seq);
    
    auto outputArray = make_shared<DynamicArray<string>>(0);
    auto output = make_shared<WriteOnlyStream<string>>(outputArray);
    
    StreamEncoder::RLEEncode(input, output);
    
    auto buffer = output->GetBuffer();
    ASSERT_EQ(buffer->GetSize(), 1);
    EXPECT_EQ(buffer->Get(0), "15X");
}

TEST_F(StreamEncoderTest, RLEDecodeSimple) {
    auto encodedArray = make_shared<DynamicArray<string>>(4);
    encodedArray->Set(0, "3A");
    encodedArray->Set(1, "3B");
    encodedArray->Set(2, "3C");
    encodedArray->Set(3, "3D");
    
    auto seq = make_shared<LazySequence<string>>(encodedArray);
    auto input = make_shared<ReadOnlyStream<string>>(seq);
    
    auto outputArray = make_shared<DynamicArray<char>>(0);
    auto output = make_shared<WriteOnlyStream<char>>(outputArray);
    
    StreamEncoder::RLEDecode(input, output);
    
    auto buffer = output->GetBuffer();
    ASSERT_EQ(buffer->GetSize(), 12);
    
    string decoded;
    for (size_t i = 0; i < buffer->GetSize(); i++) {
        decoded += buffer->Get(i);
    }
    
    EXPECT_EQ(decoded, "AAABBBCCCDDD");
}

TEST_F(StreamEncoderTest, RLEDecodeSingle) {
    auto encodedArray = make_shared<DynamicArray<string>>(1);
    encodedArray->Set(0, "5X");
    
    auto seq = make_shared<LazySequence<string>>(encodedArray);
    auto input = make_shared<ReadOnlyStream<string>>(seq);
    
    auto outputArray = make_shared<DynamicArray<char>>(0);
    auto output = make_shared<WriteOnlyStream<char>>(outputArray);
    
    StreamEncoder::RLEDecode(input, output);
    
    auto buffer = output->GetBuffer();
    ASSERT_EQ(buffer->GetSize(), 5);
    
    for (size_t i = 0; i < 5; i++) {
        EXPECT_EQ(buffer->Get(i), 'X');
    }
}

TEST_F(StreamEncoderTest, RLEEncodeDecodeRoundtrip) {
    string original = "ABBCCCDDDDEEEEEFFFFFF";
    
    auto inputArray = make_shared<DynamicArray<char>>(original.length());
    for (size_t i = 0; i < original.length(); i++) {
        inputArray->Set(i, original[i]);
    }
    auto inputSeq = make_shared<LazySequence<char>>(inputArray);
    auto input1 = make_shared<ReadOnlyStream<char>>(inputSeq);
    
    auto encodedArray = make_shared<DynamicArray<string>>(0);
    auto encodedOutput = make_shared<WriteOnlyStream<string>>(encodedArray);

    StreamEncoder::RLEEncode(input1, encodedOutput);
    
    auto encodedSeq = make_shared<LazySequence<string>>(encodedArray);
    auto input2 = make_shared<ReadOnlyStream<string>>(encodedSeq);
    
    auto decodedArray = make_shared<DynamicArray<char>>(0);
    auto decodedOutput = make_shared<WriteOnlyStream<char>>(decodedArray);

    StreamEncoder::RLEDecode(input2, decodedOutput);
    
    auto buffer = decodedOutput->GetBuffer();
    string decoded;
    for (size_t i = 0; i < buffer->GetSize(); i++) {
        decoded += buffer->Get(i);
    }
    
    EXPECT_EQ(decoded, original);
}

TEST_F(StreamEncoderTest, RLEDecodeInvalidFormat) {
    auto encodedArray = make_shared<DynamicArray<string>>(1);
    encodedArray->Set(0, "123");
    
    auto seq = make_shared<LazySequence<string>>(encodedArray);
    auto input = make_shared<ReadOnlyStream<string>>(seq);
    
    auto outputArray = make_shared<DynamicArray<char>>(0);
    auto output = make_shared<WriteOnlyStream<char>>(outputArray);
    
    EXPECT_NO_THROW(StreamEncoder::RLEDecode(input, output));
    
    auto buffer = output->GetBuffer();
    EXPECT_EQ(buffer->GetSize(), 0);
}

TEST_F(StreamEncoderTest, StreamEncoderLargeCount) {
    string largeRun(1000, 'Z');
    
    auto array = make_shared<DynamicArray<char>>(largeRun.length());
    for (size_t i = 0; i < largeRun.length(); i++) {
        array->Set(i, largeRun[i]);
    }
    auto seq = make_shared<LazySequence<char>>(array);
    auto input = make_shared<ReadOnlyStream<char>>(seq);
    
    auto outputArray = make_shared<DynamicArray<string>>(0);
    auto output = make_shared<WriteOnlyStream<string>>(outputArray);
    
    StreamEncoder::RLEEncode(input, output);
    
    auto buffer = output->GetBuffer();
    ASSERT_EQ(buffer->GetSize(), 1);
    EXPECT_EQ(buffer->Get(0), "1000Z");
}

// Тесты для StreamStatistics
class StreamStatisticsTest: public ::testing::Test {
    protected:
        void SetUp() override {
            numbers = {1.0, 2.0, 3.0, 4.0, 5.0};
            strings = {"apple", "banana", "cherry", "date", "elderberry"};
        }

        vector<double> numbers;
        vector<string> strings;
};

TEST_F(StreamStatisticsTest, BasicStatisticsForNumbers) {
    StreamStatistics<double> stats;
    
    for (double num : numbers) {
        stats.Process(num);
    }
    
    EXPECT_EQ(stats.GetCount(), 5);
    EXPECT_DOUBLE_EQ(stats.GetSum(), 15.0);
    EXPECT_DOUBLE_EQ(stats.GetMin(), 1.0);
    EXPECT_DOUBLE_EQ(stats.GetMax(), 5.0);
    EXPECT_DOUBLE_EQ(stats.GetAverage(), 3.0);
    
    EXPECT_NEAR(stats.GetVariance(), 2.0, 0.0001);
    EXPECT_NEAR(stats.GetStandardDeviation(), sqrt(2.0), 0.0001);
}

TEST_F(StreamStatisticsTest, StatisticsFromStream) {
    stringstream data;
    for (double num : numbers) {
        data << num << "\n";
    }
    
    auto deserializer = make_shared<DoubleDeserializer>();
    auto input = make_shared<ReadOnlyStream<double>>(data.str(), deserializer, '\n');
    
    StreamStatistics<double> stats;
    stats.CollectFromStream(input);
    
    EXPECT_EQ(stats.GetCount(), 5);
    EXPECT_DOUBLE_EQ(stats.GetSum(), 15.0);
    EXPECT_DOUBLE_EQ(stats.GetAverage(), 3.0);
}

TEST_F(StreamStatisticsTest, StatisticsFromLazySequence) {
    auto array = make_shared<DynamicArray<double>>(numbers.size());
    for (size_t i = 0; i < numbers.size(); i++) {
        array->Set(i, numbers[i]);
    }
    
    auto lazySeq = make_shared<LazySequence<double>>(array);
    
    StreamStatistics<double> stats;
    stats.CollectFromSequence(lazySeq);
    
    EXPECT_EQ(stats.GetCount(), 5);
    EXPECT_DOUBLE_EQ(stats.GetSum(), 15.0);
    EXPECT_DOUBLE_EQ(stats.GetAverage(), 3.0);
}

TEST_F(StreamStatisticsTest, StatisticsFromInfiniteSequence) {
    auto generator = make_shared<Generator<double>>(
        []() { return 1.0; },
        []() { return true; }
    );
    auto infiniteSeq = make_shared<LazySequence<double>>(generator, Cardinal::Infinite());
    
    StreamStatistics<double> stats;
    stats.CollectFromSequence(infiniteSeq, 100);
    
    EXPECT_EQ(stats.GetCount(), 100);
    EXPECT_DOUBLE_EQ(stats.GetSum(), 100.0);
    EXPECT_DOUBLE_EQ(stats.GetAverage(), 1.0);
    EXPECT_DOUBLE_EQ(stats.GetMin(), 1.0);
    EXPECT_DOUBLE_EQ(stats.GetMax(), 1.0);
    EXPECT_DOUBLE_EQ(stats.GetVariance(), 0.0);
}

TEST_F(StreamStatisticsTest, EmptyStatistics) {
    StreamStatistics<double> stats;
    
    EXPECT_EQ(stats.GetCount(), 0);
    EXPECT_DOUBLE_EQ(stats.GetSum(), 0.0);
    EXPECT_DOUBLE_EQ(stats.GetAverage(), 0.0);
    
    EXPECT_DOUBLE_EQ(stats.GetMin(), 0.0);
    EXPECT_DOUBLE_EQ(stats.GetMax(), 0.0);
    EXPECT_DOUBLE_EQ(stats.GetVariance(), 0.0);
    EXPECT_DOUBLE_EQ(stats.GetStandardDeviation(), 0.0);
    
    EXPECT_EQ(stats.GetAllStatistics(), "Нет данных!");
}

TEST_F(StreamStatisticsTest, ResetStatistics) {
    StreamStatistics<double> stats;
    
    stats.Process(10.0);
    stats.Process(20.0);
    
    EXPECT_EQ(stats.GetCount(), 2);
    EXPECT_DOUBLE_EQ(stats.GetSum(), 30.0);
    
    stats.Reset();
    
    EXPECT_EQ(stats.GetCount(), 0);
    EXPECT_DOUBLE_EQ(stats.GetSum(), 0.0);
    EXPECT_DOUBLE_EQ(stats.GetAverage(), 0.0);
}

TEST_F(StreamStatisticsTest, StringStatisticsBasic) {
    StreamStatistics<string> stats;
    
    for (const auto &str : strings) {
        stats.Process(str);
    }
    
    EXPECT_EQ(stats.GetCount(), 5);
    EXPECT_EQ(stats.GetTotalLength(), 31);
    EXPECT_NEAR(stats.GetAverageLength(), 6.2, 0.0001);
    EXPECT_EQ(stats.GetShortest(), "date");
    EXPECT_EQ(stats.GetLongest(), "elderberry");
}

TEST_F(StreamStatisticsTest, StringStatisticsFromStream) {
    stringstream data;
    for (const auto& str : strings) {
        data << str << "\n";
    }
    
    auto deserializer = make_shared<StringDeserializer>();
    auto input = make_shared<ReadOnlyStream<string>>(data.str(), deserializer, '\n');
    
    StreamStatistics<string> stats;
    stats.CollectFromStream(input);
    
    EXPECT_EQ(stats.GetCount(), 5);
    EXPECT_EQ(stats.GetShortest(), "date");
    EXPECT_EQ(stats.GetLongest(), "elderberry");
}

TEST_F(StreamStatisticsTest, StringStatisticsEmpty) {
    StreamStatistics<string> stats;
    
    EXPECT_EQ(stats.GetCount(), 0);
    EXPECT_EQ(stats.GetTotalLength(), 0);
    EXPECT_EQ(stats.GetAverageLength(), 0.0);
    EXPECT_EQ(stats.GetShortest(), "");
    EXPECT_EQ(stats.GetLongest(), "");
    
    EXPECT_EQ(stats.GetAllStatistics(), "Нет данных!");
}

TEST_F(StreamStatisticsTest, StatisticsWithMaxElementsLimit) {
    auto array = make_shared<DynamicArray<int>>(1000);
    for (int i = 0; i < 1000; i++) {
        array->Set(i, i + 1);
    }
    
    auto lazySeq = make_shared<LazySequence<int>>(array);
    
    StreamStatistics<int> stats;
    stats.CollectFromSequence(lazySeq, 100);
    
    EXPECT_EQ(stats.GetCount(), 100);
    EXPECT_EQ(stats.GetMin(), 1);
    EXPECT_EQ(stats.GetMax(), 100);
    
    EXPECT_EQ(stats.GetSum(), 5050);
    EXPECT_DOUBLE_EQ(stats.GetAverage(), 50.5);
}

TEST_F(StreamStatisticsTest, StatisticsWithNegativeNumbers) {
    vector<int> negativeNumbers = {-5, -3, -1, 0, 2, 4};
    
    StreamStatistics<int> stats;
    for (int num : negativeNumbers) {
        stats.Process(num);
    }
    
    EXPECT_EQ(stats.GetCount(), 6);
    EXPECT_EQ(stats.GetSum(), -3);
    EXPECT_EQ(stats.GetMin(), -5);
    EXPECT_EQ(stats.GetMax(), 4);
    EXPECT_NEAR(stats.GetAverage(), -0.5, 0.0001);
}

TEST_F(StreamStatisticsTest, GetAllStatisticsOutput) {
    StreamStatistics<double> stats;
    stats.Process(10.0);
    stats.Process(20.0);
    stats.Process(30.0);
    
    string result = stats.GetAllStatistics();
    
    EXPECT_NE(result.find("Элементов: 3"), string::npos);
    EXPECT_NE(result.find("Сумма: 60"), string::npos);
    EXPECT_NE(result.find("Среднее: 20"), string::npos);
    EXPECT_NE(result.find("Минимум: 10"), string::npos);
    EXPECT_NE(result.find("Максимум: 30"), string::npos);
    EXPECT_NE(result.find("Стандартное отклонение"), string::npos);
}

TEST_F(StreamStatisticsTest, StatisticsPrecision) {
    StreamStatistics<double> stats;

    for (int i = 0; i < 1000; i++) {
        stats.Process(0.1);
    }
    
    EXPECT_EQ(stats.GetCount(), 1000);
    EXPECT_NEAR(stats.GetSum(), 100.0, 0.0001);
    EXPECT_NEAR(stats.GetAverage(), 0.1, 0.0000001);
}

TEST_F(StreamStatisticsTest, StreamStatisticsZeroVariance) {
    StreamStatistics<int> stats;
    
    for (int i = 0; i < 100; i++) {
        stats.Process(42);
    }
    
    EXPECT_EQ(stats.GetCount(), 100);
    EXPECT_EQ(stats.GetSum(), 4200);
    EXPECT_EQ(stats.GetMin(), 42);
    EXPECT_EQ(stats.GetMax(), 42);
    EXPECT_DOUBLE_EQ(stats.GetAverage(), 42.0);
    EXPECT_DOUBLE_EQ(stats.GetVariance(), 0.0);
    EXPECT_DOUBLE_EQ(stats.GetStandardDeviation(), 0.0);
}

TEST_F(StreamStatisticsTest, NullStreamThrows) {
    StreamStatistics<double> stats;
    
    EXPECT_THROW(stats.CollectFromStream(nullptr), invalid_argument);
    EXPECT_THROW(stats.CollectFromSequence(shared_ptr<LazySequence<double>>(nullptr)), invalid_argument);
}

TEST_F(StreamStatisticsTest, StreamStatisticsLargeDataset) {
    auto array = make_shared<DynamicArray<int>>(100000);
    for (int i = 0; i < 100000; i++) {
        array->Set(i, i % 100);
    }
    
    auto lazySeq = make_shared<LazySequence<int>>(array);
    
    StreamStatistics<int> stats;
    
    auto start = chrono::high_resolution_clock::now();
    stats.CollectFromSequence(lazySeq);
    auto end = chrono::high_resolution_clock::now();
    
    auto duration = chrono::duration_cast<chrono::milliseconds>(end - start);
    
    EXPECT_EQ(stats.GetCount(), 100000);
    EXPECT_LT(duration.count(), 1000);
}

// Финальный тест
TEST(FinalTest, CompleteWorkflow) {
    string text = "AAAABBBCCCDDDDEEEEE";
    
    auto inputArray = make_shared<DynamicArray<char>>(text.length());
    for (size_t i = 0; i < text.length(); i++) {
        inputArray->Set(i, text[i]);
    }
    auto inputSeq = make_shared<LazySequence<char>>(inputArray);
    auto input1 = make_shared<ReadOnlyStream<char>>(inputSeq);
    
    auto encodedArray = make_shared<DynamicArray<string>>(0);
    auto encodedOutput = make_shared<WriteOnlyStream<string>>(encodedArray);

    StreamEncoder::RLEEncode(input1, encodedOutput);
    
    StreamStatistics<string> stats1;
    auto encodedSeq = make_shared<LazySequence<string>>(encodedArray);
    auto encodedStream = make_shared<ReadOnlyStream<string>>(encodedSeq);
    stats1.CollectFromStream(encodedStream);
    
    EXPECT_GT(stats1.GetCount(), 0);
    
    auto input2 = make_shared<ReadOnlyStream<string>>(encodedSeq);
    auto decodedArray = make_shared<DynamicArray<char>>(0);
    auto decodedOutput = make_shared<WriteOnlyStream<char>>(decodedArray);

    StreamEncoder::RLEDecode(input2, decodedOutput);
    
    StreamStatistics<char> stats2;
    auto decodedSeq = make_shared<LazySequence<char>>(decodedArray);
    auto decodedStream = make_shared<ReadOnlyStream<char>>(decodedSeq);
    stats2.CollectFromStream(decodedStream);
    
    EXPECT_EQ(stats2.GetCount(), text.length());
}

// Основные функции
inline int run_test_se() {
    int argc = 1;
    char* argv[] = {(char*)"test_program"};
    testing::InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(filter) = "StreamEncoderTest*";
    return RUN_ALL_TESTS();
}

inline int run_test_ss() {
    int argc = 1;
    char* argv[] = {(char*)"test_program"};
    testing::InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(filter) = "StreamStatisticsTest*";
    return RUN_ALL_TESTS();
}

inline int run_test_final() {
    int argc = 1;
    char* argv[] = {(char*)"test_program"};
    testing::InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(filter) = "FinalTest*";
    return RUN_ALL_TESTS();
}

#endif // TEST_SES_HPP
