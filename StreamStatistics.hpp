#ifndef STREAMSTATISTICS_HPP
#define STREAMSTATISTICS_HPP

#include <cmath>
#include <stdexcept>
#include "Stream.hpp"
using namespace std;


// Сбор статистики
template <typename T>
class StreamStatistics {
    private:
        size_t count;
        T sum;
        T min;
        T max;
        T sumSquares;
    public:
        StreamStatistics(): count(0), sum(0), min(T()), max(T()), sumSquares(0) {}

        // Обработка одного значения
        void Process(const T &value) {
            if (count == 0) {
                min = max = value;
            } else {
                if (value < min) min = value;
                if (value > max) max = value;
            }
            sum += value;
            sumSquares += value*value;
            count++;
        }
    
        // Базовые статистики
        size_t GetCount() const { return count; }
        T GetSum() const { return sum; }
        T GetMin() const { return count > 0 ? min : T(); }
        T GetMax() const { return count > 0 ? max : T(); }
        double GetAverage() const { return count > 0 ? static_cast<double>(sum) / count : 0.0; }
        double GetStandardDeviation() const { return sqrt(GetVariance()); }
        double GetVariance() const {
            if (count <= 1) return 0.0;
            double avg = GetAverage();
            return (static_cast<double>(sumSquares)/count)-(avg*avg);
        }

        // Сбор из Stream
        void CollectFromStream(shared_ptr<ReadableStream<T>> stream, size_t maxElements = 0) {
            if (!stream) throw invalid_argument("Пустой поток!");
            stream->Open();
            size_t processed = 0;
            while (!stream->IsEndOfStream()) {
                try {
                    Process(stream->Read());
                    processed++;
                    if (maxElements > 0 && processed >= maxElements) break;
                } catch (const exception&) {
                    break;
                }
            }
            stream->Close();
        }
    
        // Сбор из LazySequence
        void CollectFromSequence(shared_ptr<LazySequence<T>> seq, size_t maxElements = 0) {
            if (!seq) throw invalid_argument("Пустая ленивая последовательность!");
            if (maxElements == 0) {
                try {
                    maxElements = seq->GetLength();
                } catch (const runtime_error&) {
                    maxElements = 1000000;
                }
            }
            for (size_t i = 0; i < maxElements; i++) {
                try {
                    Process(seq->Get(i));
                } catch (const exception&) {
                    break;
                }
            }
        }
    
        // Сбор из Sequence
        void CollectFromSequence(shared_ptr<Sequence<T>> seq) {
            if (!seq) throw invalid_argument("Пустая последовательность!");
            size_t length = seq->GetLength();
            for (size_t i = 0; i < length; i++) {
                Process(seq->Get(i));
            }
        }
    
        // Получить все статистики
        string GetAllStatistics() const {
            if (count == 0) return "Нет данных!";
            string result;
            result += "Элементов: "+to_string(count)+"\n";
            result += "Сумма: "+to_string(sum)+"\n";
            result += "Среднее: "+to_string(GetAverage())+"\n";
            result += "Минимум: "+to_string(min)+"\n";
            result += "Максимум: "+to_string(max)+"\n";
            result += "Стандартное отклонение: "+to_string(GetStandardDeviation())+"\n";
            return result;
        }
    
        // Очистить
        void Reset() {
            count = 0;
            sum = T();
            min = T();
            max = T();
            sumSquares = T();
        }
};


// Специализация для строк
template <>
class StreamStatistics<string> {
    private:
        size_t count;
        size_t totalLength;
        string shortest;
        string longest;
    public:
        StreamStatistics(): count(0), totalLength(0) {}
    
        // Обработка одного значения
        void Process(const string &value) {
            if (count == 0) {
                shortest = longest = value;
            } else {
                if (value.length() < shortest.length()) shortest = value;
                if (value.length() > longest.length()) longest = value;
            }
            totalLength += value.length();
            count++;
        }
    
        // Базовые статистики
        size_t GetCount() const { return count; }
        size_t GetTotalLength() const { return totalLength; }
        string GetShortest() const { return shortest; }
        string GetLongest() const { return longest; }
        double GetAverageLength() const { return count > 0 ? static_cast<double>(totalLength) / count : 0.0; }

        // Сбор из Stream
        void CollectFromStream(shared_ptr<ReadableStream<string>> stream, size_t maxElements = 0) {
            if (!stream) throw invalid_argument("Пустой поток!");
            stream->Open();
            size_t processed = 0;
            while (!stream->IsEndOfStream()) {
                try {
                    Process(stream->Read());
                    processed++;
                    if (maxElements > 0 && processed >= maxElements) break;
                } catch (const exception&) {
                    break;
                }
            }
            stream->Close();
        }
    
        // Сбор из LazySequence
        void CollectFromSequence(shared_ptr<LazySequence<string>> seq, size_t maxElements = 0) {
            if (!seq) throw invalid_argument("Пустая ленивая последовательность!");
            if (maxElements == 0) {
                try {
                    maxElements = seq->GetLength();
                } catch (const runtime_error&) {
                    maxElements = 1000000;
                }
            }
            for (size_t i = 0; i < maxElements; i++) {
                try {
                    Process(seq->Get(i));
                } catch (const exception&) {
                    break;
                }
            }
        }
    
        // Получить все статистики
        string GetAllStatistics() const {
            if (count == 0) return "Нет данных!";
            string result;
            result += "Строк: "+to_string(count)+"\n";
            result += "Общая длина: "+to_string(totalLength)+"\n";
            result += "Средняя длина: "+to_string(GetAverageLength())+"\n";
            if (count > 0) {
                result += "Самая короткая ("+to_string(shortest.length())+"): "+
                            (shortest.length() > 50 ? shortest.substr(0, 50) + "..." : shortest)+"\n";
                result += "Самая длинная ("+to_string(longest.length())+"): "+
                            (longest.length() > 50 ? longest.substr(0, 50) + "..." : longest)+"\n";
            }
            return result;
        }
    
        // Очистить
        void Reset() {
            count = 0;
            totalLength = 0;
            shortest.clear();
            longest.clear();
        }
};

#endif // STREAMSTATISTICS_HPP
