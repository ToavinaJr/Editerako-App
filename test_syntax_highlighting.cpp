#include <iostream>
#include <vector>
#include <string>
#include <memory>
#ifndef EXAMPLE_TEST_H
#define EXAMPLE_TEST_H

// This file demonstrates the enhanced C++ syntax highlighting
// Load this file in Editerako-App to see the improvements

namespace ExampleNamespace {
    
    // Template class with various C++ features
    template<typename T>
    class ExampleClass {
    private:
        int privateValue;
        std::string name;
        std::vector<T> data;
        mutable int counter;
        
    public:
        static constexpr double PI = 3.14159;
        
        // Constructor with member initializer list
        explicit ExampleClass(const std::string& n = "default") 
            : privateValue(0), name(n), counter(0) {
            // Constructor body with various syntax elements
            data.reserve(100);
        }
        
        // Virtual destructor
        virtual ~ExampleClass() = default;
        
        // Const member function
        int getValue() const noexcept { 
            return privateValue; 
        }
        
        // Member function with various keywords and operators
        void processData() {
            try {
                for (auto& item : data) {
                    if (item > T{}) {
                        item *= 2;
                    } else if (item < T{}) {
                        item = -item;
                    } else {
                        // Handle zero case
                        continue;
                    }
                }
                
                // Range-based for loop with structured binding (C++17)
                std::vector<std::pair<int, double>> pairs = {{1, 1.5}, {2, 2.5}};
                for (const auto& [first, second] : pairs) {
                    std::cout << "Pair: " << first << ", " << second << std::endl;
                }
                
            } catch (const std::exception& e) {
                std::cerr << "Error: " << e.what() << std::endl;
                throw;
            }
        }
        
        // Template member function
        template<typename U>
        auto convert(U&& value) -> decltype(static_cast<T>(value)) {
            return static_cast<T>(std::forward<U>(value));
        }
        
        // Operator overloading
        ExampleClass& operator+=(const ExampleClass& other) {
            this->privateValue += other.privateValue;
            return *this;
        }
        
        friend bool operator==(const ExampleClass& lhs, const ExampleClass& rhs) {
            return lhs.privateValue == rhs.privateValue;
        }
    };
    
    // Free function with various features
    template<typename Container>
    void printContainer(const Container& container) {
        std::cout << "Container contents: ";
        for (const auto& element : container) {
            std::cout << element << " ";
        }
        std::cout << std::endl;
    }
    
    // Enum class (C++11)
    enum class Color : int {
        Red = 0xFF0000,
        Green = 0x00FF00,
        Blue = 0x0000FF
    };
    
    // Struct with constructor
    struct Point {
        double x, y;
        Point(double x_val = 0.0, double y_val = 0.0) : x(x_val), y(y_val) {}
    };
    
} // namespace ExampleNamespace

// Preprocessor conditional compilation
#ifdef DEBUG
    #define DBG_PRINT(x) std::cout << "DEBUG: " << x << std::endl
#else
    #define DBG_PRINT(x) do {} while(0)
#endif

// Main function demonstrating various C++ features
int main() {
    using namespace ExampleNamespace;
    
    // Smart pointers
    auto example = std::make_unique<ExampleClass<int>>("test");
    std::shared_ptr<Point> point = std::make_shared<Point>(3.14, 2.71);
    
    // Lambda expressions
    auto lambda = [&](int value) -> bool {
        return value > 0 && value < 100;
    };
    
    // Modern C++ features
    std::vector<int> numbers{1, 2, 3, 4, 5};
    
    // Range-based for loop
    for (const int& num : numbers) {
        if (lambda(num)) {
            std::cout << "Number " << num << " passes lambda test" << std::endl;
        }
    }
    
    // Conditional operator
    int result = numbers.size() > 3 ? 1 : 0;
    
    // Various operators and expressions
    int a = 10, b = 20;
    int sum = a + b;
    int diff = a - b;
    int product = a * b;
    int quotient = b / a;
    int remainder = b % a;
    
    // Bitwise operations
    int bitwiseAnd = a & b;
    int bitwiseOr = a | b;
    int bitwiseXor = a ^ b;
    int leftShift = a << 2;
    int rightShift = b >> 1;
    
    // Logical operations
    bool condition = (a < b) && (b > 0) || false;
    
    // Assignment operators
    a += 5;
    b -= 3;
    a *= 2;
    b /= 2;
    
    // Increment/decrement
    ++a;
    b--;
    
    // Pointer arithmetic
    int array[10] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
    int* ptr = array;
    int value = *(ptr + 5);
    
    // Type casting
    double pi = static_cast<double>(sum) / 3.14159;
    
    // sizeof and alignof
    std::cout << "Size of int: " << sizeof(int) << std::endl;
    std::cout << "Alignment of double: " << alignof(double) << std::endl;
    
    // Exception handling
    try {
        if (result == 0) {
            throw std::runtime_error("Something went wrong!");
        }
    } catch (const std::runtime_error& e) {
        std::cerr << "Caught exception: " << e.what() << std::endl;
        return 1;
    }
    
    DBG_PRINT("Program finished successfully");
    return 0;
}

#endif // EXAMPLE_TEST_H

/* 
 * Multi-line comment block
 * This demonstrates comment highlighting
 * with multiple lines and special characters: @#$%^&*()
 */