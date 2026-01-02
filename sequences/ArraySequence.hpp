#ifndef ARRAYSEQUENCE_HPP
#define ARRAYSEQUENCE_HPP

#include "Sequence.hpp"
#include "DynamicArray.hpp"


template <typename T>
class ArraySequence: public Sequence<T> {
    protected:
        DynamicArray<T> array;
        size_t length;

        // Реаллокация
        void increase(size_t capacity) {
            size_t oldCapacity = array.GetSize();
            if (capacity > oldCapacity) {
                size_t newCapacity = oldCapacity ? oldCapacity : 1;
                while (newCapacity < capacity) {
                    newCapacity *= 2;
                }
                array.Resize(newCapacity);
            }
        }

        void decrease() {
            size_t oldCapacity = array.GetSize();
            if (oldCapacity >= length*2 && oldCapacity > 1) {
                size_t newCapacity = oldCapacity/2;
                if (newCapacity < length) {
                    newCapacity = length;
                }
                array.Resize(newCapacity);
            }
        }
    public:
        // Создание объекта
        ArraySequence(): length(0), array(0) {}

        ~ArraySequence() override = default;

        ArraySequence(T *items, size_t count): length(count), array(items, count) {}

        ArraySequence(const DynamicArray<T> &other): length(other.GetSize()), array(other) {}

        ArraySequence(const ArraySequence<T> &other): length(other.length), array(other.array) {}
        ArraySequence<T>& operator=(const ArraySequence<T> &other) {
            if (this != &other) {
                length = other.length;
                array = other.array;
            }
            return *this;
        }

        // Декомпозиция
        size_t GetLength() const override { return length; }

        T GetFirst() const override { return Get(0); }

        T GetLast() const override { return Get(length-1); }

        T Get(size_t index) const override {
            if (length > index) {
                return array.Get(index);
            }
            throw std::out_of_range("Некорректный индекс!");
        }

        // Перегрузка операторов
        T& operator[](size_t index) override {
            if (length > index) {
                return array[index];
            }
            throw std::out_of_range("Некорректный индекс!");
        }

        const T& operator[](size_t index) const override {
            if (length > index) {
                return array[index];
            }
            throw std::out_of_range("Некорректный индекс!");
        }

        // Операции
        Sequence<T>* Append(T item) override {
            increase(length+1);
            array.Set(length, item);
            length++;
            return this;
        }

        Sequence<T>* Prepend(T item) override {
            increase(length+1);
            for (size_t i = length; i > 0; i--) {
                array.Set(i, array.Get(i-1));
            }
            array.Set(0, item);
            length++;
            return this;
        }

        Sequence<T>* Remove(size_t index) override {
            if (length <= index) {
                throw std::out_of_range("Некорректный индекс!");
            }
            for (size_t i = index; i < length-1; i++) {
                array.Set(i, array.Get(i+1));
            }
            length--;
            decrease();
            return this;
        }

        Sequence<T>* InsertAt(T item, size_t index) override {
            if (length < index) {
                throw std::out_of_range("Некорректный индекс!");
            }
            increase(length+1);
            for (size_t i = length; i > index; i--) {
                array.Set(i, array.Get(i-1));
            }
            array.Set(index, item);
            length++;
            return this;
        }

        Sequence<T>* PutAt(T item, size_t index) override {
            if (length <= index) {
                throw std::out_of_range("Некорректный индекс!");
            }
            array.Set(index, item);
            return this;
        }

        Sequence<T>* Concat(Sequence<T> *other) override {
            size_t otherLength = other->GetLength();
            increase(length+otherLength);
            for (size_t i = 0; i < otherLength; i++) {
                array.Set(length+i, other->Get(i));
            }
            length += otherLength;
            return this;
        }

        Sequence<T>* GetSubsequence(size_t startIndex, size_t endIndex) override {
            if (length <= endIndex || startIndex > endIndex) {
                throw std::out_of_range("Некорректные индексы!");
            }
            size_t subLength = endIndex-startIndex+1;
            auto subSequence = new ArraySequence<T>();
            subSequence->increase(subLength);
            for (size_t i = 0; i < subLength; i++) {
                subSequence->array.Set(i, array.Get(startIndex+i));
            }
            subSequence->length = subLength;
            return subSequence;
        }

        // Дополнительные операции
        template <typename U>
        Sequence<U>* Map(std::function<U(T)> func) {
            auto result = new ArraySequence<U>();
            result->increase(length);
            for (size_t i = 0; i < length; i++) {
                result->array.Set(i, func(array.Get(i)));
            }
            result->length = length;
            return result;
        }

        Sequence<T>* Where(std::function<bool(T)> func) {
            auto result = new ArraySequence<T>();
            for (size_t i = 0; i < length; i++) {
                T value = array.Get(i);
                if (func(value)) {
                    result->increase(result->length+1);
                    result->array.Set(result->length, value);
                    result->length++;
                }
            }
            return result;
        }

        T Reduce(std::function<T(T, T)> func, T start) {
            T result = start;
            for (size_t i = 0; i < length; i++) {
                result = func(result, array.Get(i));
            }
            return result;
        }

        template <typename U>
        Sequence<std::pair<T, U>>* Zip(Sequence<U> *other) {
            size_t minLength = std::min(length, other->GetLength());
            auto result = new ArraySequence<std::pair<T, U>>();
            result->increase(minLength);
            for (size_t i = 0; i < minLength; i++) {
                result->array.Set(i, std::make_pair(array.Get(i), other->Get(i)));
            }
            result->length = minLength;
            return result;
        }

        template <typename U>
        static std::pair<Sequence<T>*, Sequence<U>*> Unzip(Sequence<std::pair<T, U>> *sequence) {
            auto first = new ArraySequence<T>();
            auto second = new ArraySequence<U>();
            size_t seqLength = sequence->GetLength();
            first->increase(seqLength);
            second->increase(seqLength);
            for (size_t i = 0; i < seqLength; i++) {
                auto pair = sequence->Get(i);
                first->array.Set(i, pair.first);
                second->array.Set(i, pair.second);
            }
            first->length = seqLength;
            second->length = seqLength;
            return std::make_pair(first, second);
        }
};

#endif // ARRAYSEQUENCE_HPP
