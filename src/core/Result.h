/**
 * @file Result.h
 * @brief Result monad for functional error handling
 *
 * Provides a type-safe way to handle operations that may fail,
 * avoiding exceptions and making error handling explicit.
 *
 * @author ESP32 CAM Webserver v5.0
 * @date 2025
 */

#ifndef RESULT_H
#define RESULT_H

#include <Arduino.h>
#include <functional>

/**
 * @brief Result type representing either success with a value or failure with an error message
 *
 * This is a monad implementation that allows for safe error handling and
 * composable operations without exceptions.
 *
 * @tparam T The type of the success value
 *
 * @example
 * Result<int> divide(int a, int b) {
 *     if (b == 0) {
 *         return Result<int>::error("Division by zero");
 *     }
 *     return Result<int>::ok(a / b);
 * }
 *
 * auto result = divide(10, 2)
 *     .map([](int x) { return x * 2; })
 *     .flatMap([](int x) { return divide(x, 4); });
 *
 * if (result.isOk()) {
 *     Serial.println(result.getValue());
 * } else {
 *     Serial.println(result.getError());
 * }
 */
template<typename T>
class Result {
private:
    bool _success;
    T _value;
    String _errorMessage;
    int _errorCode;

    /**
     * @brief Private constructor for internal use
     */
    Result(bool success, T value, String error, int code = 0)
        : _success(success), _value(value), _errorMessage(error), _errorCode(code) {}

public:
    /**
     * @brief Create a successful result
     *
     * @param value The success value
     * @return Result<T> A successful result containing the value
     */
    static Result<T> ok(T value) {
        return Result(true, value, "", 0);
    }

    /**
     * @brief Create a failed result with error message
     *
     * @param message Error message describing the failure
     * @param code Optional error code (default: -1)
     * @return Result<T> A failed result containing the error
     */
    static Result<T> error(String message, int code = -1) {
        return Result(false, T(), message, code);
    }

    /**
     * @brief Check if result is successful
     *
     * @return true if successful, false otherwise
     */
    bool isOk() const { return _success; }

    /**
     * @brief Check if result is an error
     *
     * @return true if error, false otherwise
     */
    bool isError() const { return !_success; }

    /**
     * @brief Get the success value
     *
     * @warning Only call this if isOk() returns true
     * @return const T& Reference to the value
     */
    const T& getValue() const { return _value; }

    /**
     * @brief Get the success value (non-const version)
     *
     * @warning Only call this if isOk() returns true
     * @return T& Reference to the value
     */
    T& getValue() { return _value; }

    /**
     * @brief Get the error message
     *
     * @return const String& Error message
     */
    const String& getError() const { return _errorMessage; }

    /**
     * @brief Get the error code
     *
     * @return int Error code
     */
    int getErrorCode() const { return _errorCode; }

    /**
     * @brief Get value or default if error
     *
     * @param defaultValue Value to return if result is error
     * @return T The value or default
     */
    T getOrElse(T defaultValue) const {
        return _success ? _value : defaultValue;
    }

    /**
     * @brief Transform the value if successful
     *
     * @tparam U The return type of the transformation
     * @param fn Transformation function
     * @return Result<U> Result with transformed value or original error
     */
    template<typename U>
    Result<U> map(std::function<U(T)> fn) const {
        if (_success) {
            return Result<U>::ok(fn(_value));
        }
        return Result<U>::error(_errorMessage, _errorCode);
    }

    /**
     * @brief Chain operations that return Results
     *
     * @tparam U The return type of the chained operation
     * @param fn Function that takes T and returns Result<U>
     * @return Result<U> Result of the chained operation or original error
     */
    template<typename U>
    Result<U> flatMap(std::function<Result<U>(T)> fn) const {
        if (_success) {
            return fn(_value);
        }
        return Result<U>::error(_errorMessage, _errorCode);
    }

    /**
     * @brief Execute a side effect if successful
     *
     * @param fn Function to execute with the value
     * @return Result<T>& Reference to this result for chaining
     */
    Result<T>& onSuccess(std::function<void(const T&)> fn) {
        if (_success) {
            fn(_value);
        }
        return *this;
    }

    /**
     * @brief Execute a side effect if error
     *
     * @param fn Function to execute with the error message
     * @return Result<T>& Reference to this result for chaining
     */
    Result<T>& onError(std::function<void(const String&, int)> fn) {
        if (!_success) {
            fn(_errorMessage, _errorCode);
        }
        return *this;
    }

    /**
     * @brief Unwrap value or call panic function
     *
     * @param panicFn Function to call if result is error
     * @return T& Reference to the value
     */
    T& expect(std::function<void(const String&)> panicFn) {
        if (!_success) {
            panicFn(_errorMessage);
        }
        return _value;
    }
};

/**
 * @brief Specialization for void results (operations that don't return a value)
 */
template<>
class Result<void> {
private:
    bool _success;
    String _errorMessage;
    int _errorCode;

    Result(bool success, String error, int code)
        : _success(success), _errorMessage(error), _errorCode(code) {}

public:
    static Result<void> ok() {
        return Result(true, "", 0);
    }

    static Result<void> error(String message, int code = -1) {
        return Result(false, message, code);
    }

    bool isOk() const { return _success; }
    bool isError() const { return !_success; }
    const String& getError() const { return _errorMessage; }
    int getErrorCode() const { return _errorCode; }

    template<typename U>
    Result<U> map(std::function<U()> fn) const {
        if (_success) {
            return Result<U>::ok(fn());
        }
        return Result<U>::error(_errorMessage, _errorCode);
    }

    template<typename U>
    Result<U> flatMap(std::function<Result<U>()> fn) const {
        if (_success) {
            return fn();
        }
        return Result<U>::error(_errorMessage, _errorCode);
    }

    Result<void>& onSuccess(std::function<void()> fn) {
        if (_success) {
            fn();
        }
        return *this;
    }

    Result<void>& onError(std::function<void(const String&, int)> fn) {
        if (!_success) {
            fn(_errorMessage, _errorCode);
        }
        return *this;
    }
};

#endif // RESULT_H
