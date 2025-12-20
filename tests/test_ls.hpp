#ifndef TEST_LS_HPP
#define TEST_LS_HPP

#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <stdexcept>
#include "../LazySequence.hpp"
#include "../sequences/ArraySequence.hpp"
using namespace std;


// Тесты для LazySequence
class LazySequenceTest: public testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
    
    // Вспомогательная функция для создания последовательности чисел
    static LazySequence<int> CreateNumberSequence(int start, int count) {
        int* items = new int[count];
        for (int i = 0; i < count; i++) {
            items[i] = start + i;
        }
        LazySequence<int> seq(items, count);
        delete[] items;
        return seq;
    }
};

// Базовые тесты
TEST_F(LazySequenceTest, EmptySequence) {
    LazySequence<int> seq;
    EXPECT_EQ(seq.GetLength(), 0);
    EXPECT_EQ(seq.GetMaterializedCount(), 0);
    EXPECT_THROW(seq.GetFirst(), out_of_range);
    EXPECT_THROW(seq.GetLast(), out_of_range);
    EXPECT_THROW(seq.Get(0), out_of_range);
}

TEST_F(LazySequenceTest, SingleElementSequence) {
    LazySequence<int> seq = CreateNumberSequence(42, 1);
    
    EXPECT_EQ(seq.GetLength(), 1);
    EXPECT_EQ(seq.GetMaterializedCount(), 1);
    EXPECT_EQ(seq.GetFirst(), 42);
    EXPECT_EQ(seq.GetLast(), 42);
    EXPECT_EQ(seq.Get(0), 42);
    
    // operator[] const
    const LazySequence<int>& constSeq = seq;
    EXPECT_EQ(constSeq[0], 42);
}

TEST_F(LazySequenceTest, LargeSequence) {
    const int SIZE = 1000;
    LazySequence<int> seq = CreateNumberSequence(0, SIZE);
    
    EXPECT_EQ(seq.GetLength(), SIZE);
    EXPECT_EQ(seq.GetMaterializedCount(), SIZE);
    EXPECT_EQ(seq.GetFirst(), 0);
    EXPECT_EQ(seq.GetLast(), SIZE - 1);
    
    // Проверка нескольких элементов
    EXPECT_EQ(seq.Get(10), 10);
    EXPECT_EQ(seq.Get(500), 500);
    EXPECT_EQ(seq.Get(999), 999);
}

// Тесты генераторов
TEST_F(LazySequenceTest, InfiniteSequence_FromGenerator) {
    int counter = 0;
    auto generator = make_shared<Generator<int>>(
        [&counter]() { return counter++; },
        []() { return true; }
    );
    
    LazySequence<int> seq(generator, Cardinal::Infinite());
    
    // Нельзя получить длину бесконечной последовательности
    EXPECT_THROW(seq.GetLength(), runtime_error);
    EXPECT_THROW(seq.GetLast(), runtime_error);
    
    // Но можно получать элементы
    EXPECT_EQ(seq.GetMaterializedCount(), 0);
    EXPECT_EQ(seq.Get(0), 0);
    EXPECT_EQ(seq.GetMaterializedCount(), 1);
    EXPECT_EQ(seq.Get(100), 100);
    EXPECT_EQ(seq.GetMaterializedCount(), 101);
    EXPECT_EQ(seq.Get(50), 50);  // Уже в кэше
    EXPECT_EQ(seq.GetMaterializedCount(), 101);  // Не должно увеличиваться
}

TEST_F(LazySequenceTest, FiniteSequence_FromGenerator) {
    int counter = 0;
    auto generator = make_shared<Generator<int>>(
        [&counter]() { 
            int value = counter;
            counter++;
            return value;
        },
        [&counter]() { return counter < 5; }
    );
    
    LazySequence<int> seq(generator, Cardinal::Finite(5));
    
    EXPECT_EQ(seq.GetLength(), 5);
    EXPECT_EQ(seq.GetMaterializedCount(), 0);
    
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(seq.Get(i), i);
    }
    
    EXPECT_EQ(seq.GetMaterializedCount(), 5);
    EXPECT_THROW(seq.Get(5), out_of_range);
}

TEST_F(LazySequenceTest, Generator_WithCondition) {
    // Генератор, который генерирует только четные числа
    int counter = 0;
    auto generator = make_shared<Generator<int>>(
        [&counter]() { 
            int value = counter;
            counter += 2;
            return value;
        },
        [&counter]() { return counter < 20; }
    );
    
    LazySequence<int> seq(generator, Cardinal::Finite(10));
    
    EXPECT_EQ(seq.GetLength(), 10);
    for (int i = 0; i < 10; i++) {
        EXPECT_EQ(seq.Get(i), i * 2);
    }
}

// Тесты операций
TEST_F(LazySequenceTest, AppendOperations) {
    LazySequence<int> seq = CreateNumberSequence(1, 3);  // [1, 2, 3]
    
    auto seq2 = dynamic_cast<LazySequence<int>*>(seq.Append(4));
    ASSERT_NE(seq2, nullptr);
    
    EXPECT_EQ(seq2->GetLength(), 4);
    EXPECT_EQ(seq2->Get(0), 1);
    EXPECT_EQ(seq2->Get(1), 2);
    EXPECT_EQ(seq2->Get(2), 3);
    EXPECT_EQ(seq2->Get(3), 4);
    EXPECT_EQ(seq2->GetLast(), 4);
    
    delete seq2;
}

TEST_F(LazySequenceTest, PrependOperations) {
    LazySequence<int> seq = CreateNumberSequence(2, 3);  // [2, 3, 4]
    
    auto seq2 = dynamic_cast<LazySequence<int>*>(seq.Prepend(1));
    ASSERT_NE(seq2, nullptr);
    
    EXPECT_EQ(seq2->GetLength(), 4);
    EXPECT_EQ(seq2->GetFirst(), 1);
    EXPECT_EQ(seq2->Get(0), 1);
    EXPECT_EQ(seq2->Get(1), 2);
    EXPECT_EQ(seq2->Get(2), 3);
    EXPECT_EQ(seq2->Get(3), 4);
    
    delete seq2;
}

TEST_F(LazySequenceTest, RemoveOperations) {
    LazySequence<int> seq = CreateNumberSequence(1, 5);  // [1, 2, 3, 4, 5]
    
    // Удаление первого элемента
    auto seq1 = dynamic_cast<LazySequence<int>*>(seq.Remove(0));
    ASSERT_NE(seq1, nullptr);
    EXPECT_EQ(seq1->GetLength(), 4);
    EXPECT_EQ(seq1->GetFirst(), 2);
    delete seq1;
    
    // Удаление последнего элемента
    auto seq2 = dynamic_cast<LazySequence<int>*>(seq.Remove(4));
    ASSERT_NE(seq2, nullptr);
    EXPECT_EQ(seq2->GetLength(), 4);
    EXPECT_EQ(seq2->GetLast(), 4);
    delete seq2;
    
    // Удаление из середины
    auto seq3 = dynamic_cast<LazySequence<int>*>(seq.Remove(2));
    ASSERT_NE(seq3, nullptr);
    EXPECT_EQ(seq3->GetLength(), 4);
    EXPECT_EQ(seq3->Get(0), 1);
    EXPECT_EQ(seq3->Get(1), 2);
    EXPECT_EQ(seq3->Get(2), 4);  // Пропущен 3
    EXPECT_EQ(seq3->Get(3), 5);
    delete seq3;
}

TEST_F(LazySequenceTest, InsertAtOperations) {
    LazySequence<int> seq = CreateNumberSequence(1, 3);  // [1, 2, 3]
    
    // Вставка в начало
    auto seq1 = dynamic_cast<LazySequence<int>*>(seq.InsertAt(0, 0));
    ASSERT_NE(seq1, nullptr);
    EXPECT_EQ(seq1->GetLength(), 4);
    EXPECT_EQ(seq1->Get(0), 0);
    EXPECT_EQ(seq1->Get(1), 1);
    delete seq1;
    
    // Вставка в середину
    auto seq2 = dynamic_cast<LazySequence<int>*>(seq.InsertAt(99, 1));
    ASSERT_NE(seq2, nullptr);
    EXPECT_EQ(seq2->GetLength(), 4);
    EXPECT_EQ(seq2->Get(0), 1);
    EXPECT_EQ(seq2->Get(1), 99);
    EXPECT_EQ(seq2->Get(2), 2);
    delete seq2;
    
    // Вставка в конец
    auto seq3 = dynamic_cast<LazySequence<int>*>(seq.InsertAt(4, 3));
    ASSERT_NE(seq3, nullptr);
    EXPECT_EQ(seq3->GetLength(), 4);
    EXPECT_EQ(seq3->GetLast(), 4);
    delete seq3;
}

TEST_F(LazySequenceTest, ConcatOperations) {
    LazySequence<int> seq1 = CreateNumberSequence(1, 3);   // [1, 2, 3]
    LazySequence<int> seq2 = CreateNumberSequence(4, 3);   // [4, 5, 6]
    
    auto result = dynamic_cast<LazySequence<int>*>(seq1.Concat(&seq2));
    ASSERT_NE(result, nullptr);
    
    EXPECT_EQ(result->GetLength(), 6);
    for (int i = 0; i < 6; i++) {
        EXPECT_EQ(result->Get(i), i + 1);
    }
    
    delete result;
}

TEST_F(LazySequenceTest, GetSubsequenceOperations) {
    LazySequence<int> seq = CreateNumberSequence(1, 10);  // [1..10]
    
    // Подпоследовательность с начала
    auto sub1 = dynamic_cast<LazySequence<int>*>(seq.GetSubsequence(0, 2));
    ASSERT_NE(sub1, nullptr);
    EXPECT_EQ(sub1->GetLength(), 3);
    EXPECT_EQ(sub1->GetFirst(), 1);
    EXPECT_EQ(sub1->GetLast(), 3);
    delete sub1;
    
    // Подпоследовательность из середины
    auto sub2 = dynamic_cast<LazySequence<int>*>(seq.GetSubsequence(3, 6));
    ASSERT_NE(sub2, nullptr);
    EXPECT_EQ(sub2->GetLength(), 4);
    EXPECT_EQ(sub2->GetFirst(), 4);
    EXPECT_EQ(sub2->GetLast(), 7);
    delete sub2;
    
    // Подпоследовательность с одним элементом
    auto sub3 = dynamic_cast<LazySequence<int>*>(seq.GetSubsequence(5, 5));
    ASSERT_NE(sub3, nullptr);
    EXPECT_EQ(sub3->GetLength(), 1);
    EXPECT_EQ(sub3->GetFirst(), 6);
    delete sub3;
}

// Тесты функциональных операций
TEST_F(LazySequenceTest, MapOperation) {
    LazySequence<int> seq = CreateNumberSequence(1, 5);  // [1, 2, 3, 4, 5]
    
    // Умножение на 2
    auto mapped = seq.Map<int>([](int x) { return x * 2; });
    ASSERT_NE(mapped, nullptr);
    
    EXPECT_EQ(mapped->GetLength(), 5);
    for (int i = 0; i < 5; i++) {
        EXPECT_EQ(mapped->Get(i), (i + 1) * 2);
    }
    
    delete mapped;
}

TEST_F(LazySequenceTest, WhereOperation) {
    LazySequence<int> seq = CreateNumberSequence(1, 10);  // [1..10]
    
    // Только четные числа
    auto filtered = seq.Where([](int x) { return x % 2 == 0; });
    ASSERT_NE(filtered, nullptr);
    
    // Проверяем несколько элементов
    EXPECT_EQ(filtered->Get(0), 2);
    EXPECT_EQ(filtered->Get(1), 4);
    EXPECT_EQ(filtered->Get(2), 6);
    EXPECT_EQ(filtered->Get(3), 8);
    EXPECT_EQ(filtered->Get(4), 10);
    
    delete filtered;
}

TEST_F(LazySequenceTest, ReduceOperation) {
    LazySequence<int> seq = CreateNumberSequence(1, 5);  // [1, 2, 3, 4, 5]
    
    // Сумма всех элементов
    int sum = seq.Reduce<int>([](int acc, int x) { return acc + x; }, 0);
    EXPECT_EQ(sum, 15);  // 1+2+3+4+5 = 15
    
    // Произведение всех элементов
    int product = seq.Reduce<int>([](int acc, int x) { return acc * x; }, 1);
    EXPECT_EQ(product, 120);  // 1*2*3*4*5 = 120
    
    // С конкатенацией строк
    LazySequence<string> strSeq = LazySequence<string>();
    // Создаем последовательность строк
    string items[] = {"Hello", " ", "World", "!"};
    LazySequence<string> strSeq2(items, 4);
    
    string result = strSeq2.Reduce<string>(
        [](string acc, string x) { return acc + x; }, 
        ""
    );
    EXPECT_EQ(result, "Hello World!");
}

TEST_F(LazySequenceTest, MapWhereReduceChain) {
    LazySequence<int> seq = CreateNumberSequence(1, 10);  // [1..10]
    
    // Цепочка операций: фильтруем четные -> умножаем на 10 -> суммируем
    auto filtered = seq.Where([](int x) { return x % 2 == 0; });
    auto mapped = filtered->Map<int>([](int x) { return x * 10; });
    
    int result = mapped->Reduce<int>([](int acc, int x) { return acc + x; }, 0);
    
    // 2+4+6+8+10 = 30, умножаем на 10 = 300
    EXPECT_EQ(result, 300);
    
    delete filtered;
    delete mapped;
}

// Тесты граничных случаев
TEST_F(LazySequenceTest, EdgeCases_InvalidIndices) {
    LazySequence<int> seq = CreateNumberSequence(1, 5);
    
    EXPECT_THROW(seq.Get(-1), out_of_range);  // Отрицательный индекс
    EXPECT_THROW(seq.Get(5), out_of_range);   // За пределами
    EXPECT_THROW(seq.Get(100), out_of_range); // Далеко за пределами
    
    EXPECT_THROW(seq.Remove(5), out_of_range);
    EXPECT_THROW(seq.InsertAt(10, 6), out_of_range);
    EXPECT_THROW(seq.GetSubsequence(3, 2), out_of_range); // start > end
    EXPECT_THROW(seq.GetSubsequence(0, 5), out_of_range); // end за пределами
}

TEST_F(LazySequenceTest, EdgeCases_InfiniteSequenceLimitations) {
    auto generator = make_shared<Generator<int>>(
        []() { static int i = 0; return i++; },
        []() { return true; }
    );
    
    LazySequence<int> infiniteSeq(generator, Cardinal::Infinite());
    
    // Бесконечная последовательность не поддерживает некоторые операции
    EXPECT_THROW(infiniteSeq.Append(100), runtime_error);
    EXPECT_THROW(infiniteSeq.Remove(0), runtime_error);
    EXPECT_THROW(infiniteSeq.InsertAt(100, 0), runtime_error);
    EXPECT_THROW(infiniteSeq.GetLast(), runtime_error);
}

TEST_F(LazySequenceTest, EdgeCases_EmptySequenceOperations) {
    LazySequence<int> emptySeq;
    
    // Append к пустой последовательности
    auto seq1 = dynamic_cast<LazySequence<int>*>(emptySeq.Append(42));
    EXPECT_EQ(seq1->GetLength(), 1);
    EXPECT_EQ(seq1->GetFirst(), 42);
    delete seq1;
    
    // Prepend к пустой последовательности
    auto seq2 = dynamic_cast<LazySequence<int>*>(emptySeq.Prepend(42));
    EXPECT_EQ(seq2->GetLength(), 1);
    EXPECT_EQ(seq2->GetFirst(), 42);
    delete seq2;
    
    // Concat с пустой последовательностью
    LazySequence<int> seq3 = CreateNumberSequence(1, 3);
    auto result = dynamic_cast<LazySequence<int>*>(emptySeq.Concat(&seq3));
    EXPECT_EQ(result->GetLength(), 3);
    delete result;
    
    auto result2 = dynamic_cast<LazySequence<int>*>(seq3.Concat(&emptySeq));
    EXPECT_EQ(result2->GetLength(), 3);
    delete result2;
}

// Тесты копирования и присваивания
TEST_F(LazySequenceTest, CopyConstructor) {
    LazySequence<int> seq1 = CreateNumberSequence(1, 5);
    
    // Копируем
    LazySequence<int> seq2(seq1);
    
    EXPECT_EQ(seq1.GetLength(), seq2.GetLength());
    for (size_t i = 0; i < seq1.GetLength(); i++) {
        EXPECT_EQ(seq1.Get(i), seq2.Get(i));
    }
    
    // Изменения в копии не должны влиять на оригинал
    auto modified = dynamic_cast<LazySequence<int>*>(seq2.Append(6));
    EXPECT_EQ(seq1.GetLength(), 5);
    EXPECT_EQ(modified->GetLength(), 6);
    delete modified;
}

// Основная функция
inline int run_test_ls() {
    int argc = 1;
    char* argv[] = {(char*)"test_program"};
    testing::InitGoogleTest(&argc, argv);
    testing::GTEST_FLAG(filter) = "LazySequenceTest*";
    return RUN_ALL_TESTS();
}

#endif // TEST_LS_HPP
