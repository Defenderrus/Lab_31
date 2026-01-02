#ifndef LAZYSEQUENCE_HPP
#define LAZYSEQUENCE_HPP

#include <memory>
#include <functional>
#include "sequences/Sequence.hpp"
#include "sequences/DynamicArray.hpp"
using namespace std;


// Класс генератора последовательности
template <class T>
class Generator {
    private:
        function<T()> next;
        function<bool()> hasNext;
        
    public:
        Generator(function<T()> func, function<bool()> flag = [](){ return true; }):
            next(func), hasNext(flag) {}

        T GetNext() {
            return next();
        }
        
        bool HasNext() const {
            return hasNext();
        }
        
        bool TryGetNext(T& result) {
            if (HasNext()) {
                result = GetNext();
                return true;
            }
            return false;
        }
};


// Класс представления длины последовательности
class Cardinal {
    private:
        enum class Type { Finite, Infinite, Unknown };
        Type type;
        size_t finite_value;
    public:
        Cardinal(): type(Type::Unknown), finite_value(0) {}

        static Cardinal Finite(size_t value) {
            Cardinal c;
            c.type = Type::Finite;
            c.finite_value = value;
            return c;
        }

        static Cardinal Infinite() {
            Cardinal c;
            c.type = Type::Infinite;
            return c;
        }

        static Cardinal Unknown() {
            return Cardinal();
        }

        bool IsFinite() const { return type == Type::Finite; }

        size_t GetFiniteValue() const {
            if (!IsFinite()) throw runtime_error("Последовательность неизвестной длины или бесконечна!");
            return finite_value;
        }

        bool operator==(const Cardinal &other) const {
            if (type != other.type) return false;
            if (type == Type::Finite) {
                return finite_value == other.finite_value;
            }
            return true;
        }
        
        bool operator!=(const Cardinal &other) const {
            return !(*this == other);
        }
};


// Основной класс ленивой последовательности
template <typename T>
class LazySequence: public Sequence<T> {
    private:
        mutable DynamicArray<T> sequence;
        shared_ptr<Generator<T>> generator;
        Cardinal length;

        // Кеширование
        void Cache(size_t index) const {
            if (index < sequence.GetSize()) return;
            const size_t MAX_CACHE_SIZE = 10000;
            if (length.IsFinite()) {
                if (index >= length.GetFiniteValue()) throw out_of_range("Индекс выходит за пределы последовательности!");
            } else if (index >= MAX_CACHE_SIZE) throw out_of_range("Запрошенный индекс слишком велик для бесконечной последовательности!");
            
            size_t old_size = sequence.GetSize();
            size_t new_size = index+1;
            if (!generator) {
                if (length.IsFinite() && index < length.GetFiniteValue()) {
                    const_cast<LazySequence<T>*>(this)->sequence.Resize(new_size);
                    for (size_t i = old_size; i < new_size; i++) sequence[i] = T();
                    return;
                }
                throw runtime_error("Отсутствует генератор и невозможно создать элементы!");
            } else {
                const_cast<LazySequence<T>*>(this)->sequence.Resize(new_size);
                for (size_t i = old_size; i < new_size; i++) {
                    if (!generator->HasNext()) {
                        if (length.IsFinite()) throw runtime_error("Генератор произвел меньше элементов, чем ожидалось!");
                        else throw runtime_error("Генератор бесконечной последовательности неожиданно завершился!");
                    }
                    sequence[i] = generator->GetNext();
                }
            }
        }
    public:
        // Конструкторы
        ~LazySequence() override = default;

        LazySequence(): length(Cardinal::Finite(0)) {}

        LazySequence(T *items, size_t count): sequence(items, count), length(Cardinal::Finite(count)) {}

        LazySequence(const DynamicArray<T> &arr): sequence(arr), length(Cardinal::Finite(arr.GetSize())) {}

        LazySequence(shared_ptr<DynamicArray<T>> arr): sequence(*arr), length(Cardinal::Finite(arr->GetSize())) {}

        LazySequence(const Sequence<T> &seq): sequence(seq.GetLength()), length(Cardinal::Finite(seq.GetLength())) {
            for (size_t i = 0; i < seq.GetLength(); i++) sequence[i] = seq.Get(i);
        }

        LazySequence(shared_ptr<Sequence<T>> seq): sequence(seq->GetLength()), length(Cardinal::Finite(seq->GetLength())) {
            for (size_t i = 0; i < seq->GetLength(); i++) sequence[i] = seq->Get(i);
        }

        LazySequence(Sequence<T> *seq): sequence(seq->GetLength()), length(Cardinal::Finite(seq->GetLength())) {
            for (size_t i = 0; i < seq->GetLength(); i++) sequence[i] = seq->Get(i);
        }

        LazySequence(function<T(DynamicArray<T>*)> func, Sequence<T> *seq): sequence(seq->GetLength()), length(Cardinal::Infinite()) {
            for (size_t i = 0; i < seq->GetLength(); i++) sequence[i] = seq->Get(i);
            generator = make_shared<Generator<T>>(
                [this, func]() {
                    return func(&sequence);
                },
                []() { return true; }
            );
        }

        LazySequence(shared_ptr<Generator<T>> gen, Cardinal len = Cardinal::Infinite()): sequence(0), generator(gen), length(len) {}

        LazySequence(const LazySequence<T> &other): sequence(other.sequence), generator(other.generator), length(other.length) {}

        LazySequence(LazySequence<T> &&other) noexcept: sequence(move(other.sequence)), generator(move(other.generator)), length(move(other.length)) {}

        // Декомпозиция
        size_t GetLength() const override {
            if (length.IsFinite()) return length.GetFiniteValue();
            throw runtime_error("Последовательность неизвестной длины или бесконечна!");
        }

        T Get(size_t index) const override {
            if (length.IsFinite() && length.GetFiniteValue() == 0 && index == 0) throw out_of_range("Последовательность пуста!");
            Cache(index);
            return sequence.Get(index);
        }

        T GetFirst() const override {
            if (length.IsFinite() && length.GetFiniteValue() == 0) throw out_of_range("Последовательность пуста!");
            Cache(0);
            return sequence.Get(0);
        }

        T GetLast() const override {
            if (!length.IsFinite()) throw runtime_error("Последовательность неизвестной длины или бесконечна!");
            Cache(length.GetFiniteValue()-1);
            return sequence.Get(length.GetFiniteValue()-1);
        }

        size_t GetMaterializedCount() const {
            return sequence.GetSize();
        }

        // Перегрузка операторов
        T& operator[](size_t index) override {
            throw runtime_error("Прямое изменение элементов LazySequence не поддерживается!");
        }

        const T& operator[](size_t index) const override {
            Cache(index);
            if (index < sequence.GetSize()) return sequence[index];
            throw out_of_range("Индекс за пределами последовательности");
        }

        LazySequence& operator=(LazySequence<T> &&other) noexcept {
            if (this != &other) {
                sequence = move(other.sequence);
                generator = move(other.generator);
                length = move(other.length);
            }
            return *this;
        }
        
        // Операции
        Sequence<T>* Append(T item) override {
            if (!length.IsFinite()) throw runtime_error("Нельзя добавить элемент в конец неконечной последовательности!");
            auto new_seq = new LazySequence<T>();
            auto emitted = make_shared<bool>(false);
            auto current = make_shared<size_t>(0);
            auto temp_length = make_shared<size_t>(GetLength());
            auto temp_seq = make_shared<LazySequence<T>>(*this);
            new_seq->generator = make_shared<Generator<T>>(
                [temp_seq, temp_length, item, current, emitted]() mutable -> T {
                    if (*current < *temp_length) {
                        T value = temp_seq->Get(*current);
                        (*current)++;
                        return value;
                    }
                    if (!(*emitted)) {
                        *emitted = true;
                        return item;
                    }
                    throw runtime_error("Нет больше элементов!");
                },
                [temp_length, current, emitted]() mutable -> bool {
                    return *current < *temp_length || !(*emitted);
                }
            );
            new_seq->length = Cardinal::Finite(*temp_length+1);
            return new_seq;
        }

        Sequence<T>* Prepend(T item) override {
            auto new_seq = new LazySequence<T>();
            auto emitted = make_shared<bool>(false);
            auto current = make_shared<size_t>(0);
            auto temp_seq = make_shared<LazySequence<T>>(*this);
            new_seq->generator = make_shared<Generator<T>>(
                [temp_seq, item, current, emitted]() mutable -> T {
                    if (!(*emitted)) {
                        *emitted = true;
                        return item;
                    } else {
                        T value = temp_seq->Get(*current);
                        (*current)++;
                        return value;
                    }
                },
                [temp_seq, current, emitted]() mutable -> bool {
                    if (!(*emitted)) return true;
                    try {
                        temp_seq->Get(*current);
                        return true;
                    } catch (const out_of_range&) {
                        return false;
                    } catch (const runtime_error&) {
                        return true;
                    }
                }
            );
            if (length.IsFinite()) new_seq->length = Cardinal::Finite(GetLength()+1);
            else new_seq->length = Cardinal::Infinite();
            return new_seq;
        }

        Sequence<T>* Remove(size_t index) override {
            if (!length.IsFinite()) throw runtime_error("Нельзя удалить элемент из неконечной последовательности!");
            if (index >= length.GetFiniteValue()) throw out_of_range("Индекс выходит за пределы последовательности!");
            auto new_seq = new LazySequence<T>();
            auto current = make_shared<size_t>(0);
            auto temp_length = make_shared<size_t>(GetLength());
            auto temp_seq = make_shared<LazySequence<T>>(*this);
            new_seq->generator = make_shared<Generator<T>>(
                [temp_seq, temp_length, index, current]() mutable -> T {
                    while (true) {
                        T value = temp_seq->Get(*current);
                        (*current)++;
                        if (*current-1 != index) {
                            return value;
                        }
                    }
                    throw runtime_error("Нет больше элементов!");
                },
                [temp_length, index, current]() mutable -> bool {
                    size_t temp_current = *current;
                    if (*current > index) {
                        temp_current = *current-1;
                    }
                    return temp_current < (*temp_length-1);
                }
            );
            new_seq->length = Cardinal::Finite(*temp_length-1);
            return new_seq;
        }

        Sequence<T>* InsertAt(T item, size_t index) override {
            if (!length.IsFinite()) throw runtime_error("Нельзя вставить элемент в неконечную последовательность!");
            if (index > length.GetFiniteValue()) throw out_of_range("Индекс выходит за пределы последовательности!");
            auto new_seq = new LazySequence<T>();
            auto inserted = make_shared<bool>(false);
            auto current = make_shared<size_t>(0);
            auto source_index = make_shared<size_t>(0);
            auto temp_length = make_shared<size_t>(GetLength());
            auto temp_seq = make_shared<LazySequence<T>>(*this);
            new_seq->generator = make_shared<Generator<T>>(
                [temp_seq, temp_length, item, index, current, source_index, inserted]() mutable -> T {
                    if (!(*inserted) && *current == index) {
                        *inserted = true;
                        (*current)++;
                        return item;
                    } else {
                        T value = temp_seq->Get(*source_index);
                        (*source_index)++;
                        (*current)++;
                        return value;
                    }
                },
                [temp_length, current, inserted]() mutable -> bool {
                    if (!(*inserted)) return true;
                    return (*current-1) < (*temp_length+1);
                }
            );
            new_seq->length = Cardinal::Finite(*temp_length+1);
            return new_seq;
        }

        Sequence<T>* PutAt(T item, size_t index) override {
            throw runtime_error("PutAt() не поддерживается для LazySequence!");
        }

        Sequence<T>* Concat(Sequence<T> *other) override {
            auto new_seq = new LazySequence<T>();
            auto finished = make_shared<bool>(false);
            auto current_this = make_shared<size_t>(0);
            auto current_other = make_shared<size_t>(0);
            auto temp_seq_this = make_shared<LazySequence<T>>(*this);
            auto temp_seq_other = make_shared<LazySequence<T>>(other);
            new_seq->generator = make_shared<Generator<T>>(
                [temp_seq_this, temp_seq_other, current_this, current_other, finished]() mutable -> T {
                    if (!(*finished)) {
                        T value = temp_seq_this->Get(*current_this);
                        (*current_this)++;
                        return value;
                    } else {
                        T value = temp_seq_other->Get(*current_other);
                        (*current_other)++;
                        return value;
                    }
                },
                [temp_seq_this, temp_seq_other, current_this, current_other, finished]() mutable -> bool {
                    if (!(*finished)) {
                        try {
                            temp_seq_this->Get(*current_this);
                            return true;
                        } catch (const out_of_range&) {
                            *finished = true;
                            *current_other = 0;
                            try {
                                temp_seq_other->Get(0);
                                return true;
                            } catch (const out_of_range&) {
                                return false;
                            }
                        }
                    } else {
                        try {
                            temp_seq_other->Get(*current_other);
                            return true;
                        } catch (const out_of_range&) {
                            return false;
                        }
                    }
                }
            );
            try {
                size_t temp_length_this = GetLength();
                size_t temp_length_other = other->GetLength();
                new_seq->length = Cardinal::Finite(temp_length_this+temp_length_other);
            } catch (const runtime_error&) {
                new_seq->length = Cardinal::Infinite();
            }
            return new_seq;
        }

        Sequence<T>* GetSubsequence(size_t startIndex, size_t endIndex) override {
            if (startIndex > endIndex) throw out_of_range("Начальный индекс больше конечного!");
            if (length.IsFinite() && endIndex >= length.GetFiniteValue()) throw out_of_range("Индекс выходит за пределы последовательности!");
            auto new_seq = new LazySequence<T>();
            auto current = make_shared<size_t>(startIndex);
            auto temp_seq = make_shared<LazySequence<T>>(*this);
            new_seq->generator = make_shared<Generator<T>>(
                [temp_seq, current, endIndex]() mutable -> T {
                    if (*current > endIndex) throw runtime_error("Нет больше элементов!");
                    T value = temp_seq->Get(*current);
                    (*current)++;
                    return value;
                },
                [current, endIndex]() mutable -> bool {
                    return *current <= endIndex;
                }
            );
            new_seq->length = Cardinal::Finite(endIndex-startIndex+1);
            return new_seq;
        }

        // Дополнительные операции
        template <typename U>
        LazySequence<U>* Map(function<U(T)> func) {
            auto new_seq = new LazySequence<U>();
            auto current = make_shared<size_t>(0);
            auto temp_seq = make_shared<LazySequence<T>>(*this);
            new_seq->generator = make_shared<Generator<U>>(
                [temp_seq, func, current]() mutable -> U {
                    T value = temp_seq->Get(*current);
                    (*current)++;
                    return func(value);
                },
                [temp_seq, current]() mutable -> bool {
                    try {
                        temp_seq->Get(*current);
                        return true;
                    } catch (const out_of_range&) {
                        return false;
                    } catch (const runtime_error&) {
                        return true;
                    }
                }
            );
            new_seq->length = length;
            return new_seq;
        }

        LazySequence<T>* Where(function<bool(T)> func) {
            auto new_seq = new LazySequence<T>();
            auto current = make_shared<size_t>(0);
            auto temp_seq = make_shared<LazySequence<T>>(*this);
            new_seq->generator = make_shared<Generator<T>>(
                [temp_seq, current]() mutable -> T {
                    T value = temp_seq->Get(*current);
                    (*current)++;
                    return value;
                },
                [temp_seq, func, current]() mutable -> bool {
                    const size_t MAX_ITERATIONS = 1000;
                    try {
                        while (*current < MAX_ITERATIONS) {
                            T value = temp_seq->Get(*current);
                            if (func(value)) {
                                return true;
                            }
                            (*current)++;
                        }
                        return false;
                    } catch (const out_of_range&) {
                        return false;
                    }
                }
            );
            new_seq->length = Cardinal::Unknown();
            return new_seq;
        }

        template <typename U>
        U Reduce(function<U(U, T)> func, U start) {
            U result = start;
            size_t iterations = 0;
            const size_t MAX_ITERATIONS = 1000000;
            if (!generator) {
                if (length.IsFinite()) {
                    for (size_t i = 0; i < length.GetFiniteValue() && iterations < MAX_ITERATIONS; i++) {
                        T value = Get(i);
                        result = func(result, value);
                        iterations++;
                    }
                }
            } else {
                auto temp_generator = make_shared<Generator<T>>(*generator);
                while (iterations < MAX_ITERATIONS) {
                    try {
                        T value = temp_generator->GetNext();
                        result = func(result, value);
                        iterations++;
                    } catch (const runtime_error&) {
                        break;
                    }
                }
            }
            return result;
        }
};

#endif // LAZYSEQUENCE_HPP
