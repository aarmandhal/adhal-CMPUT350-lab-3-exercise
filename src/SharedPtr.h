#ifndef SHARED_PTR_HEADER
#define SHARED_PTR_HEADER

#include <cassert>
#include <utility>

// Non-template base class: only responsible for refcounting.
// Knows nothing about T, so any SharedPtr<T> can point at it regardless of T.
class ControlBlockBase {
public:
    // Constructor: refcount starts at 1 (the creator is the first owner)
    ControlBlockBase() : count(1) {}

    // Virtual destructor: lets derived control blocks clean up their own resource
    virtual ~ControlBlockBase() = default;

    // Pure virtual: derived classes report the address of the resource they manage
    virtual void* managedAddress() = 0;

    // Copy operations: disallowed (control blocks aren't meant to be copied)
    ControlBlockBase(const ControlBlockBase&) = delete;
    ControlBlockBase& operator=(const ControlBlockBase&) = delete;

    // Increment refcount (a new owner is being added)
    long increment() { return ++count; }

    // Decrement refcount (an owner is giving up ownership)
    long decrement() {
        assert(count > 0);
        return --count;
    }

    // Get current refcount
    long refCount() const { return count; }

private:
    long count;
};

// Concrete control block that owns a T* handed to it from outside
template <typename T>
class ControlBlock : public ControlBlockBase {
public:
    // Constructor: takes ownership of an existing T*
    explicit ControlBlock(T* p) : managed(p) {}

    // Destructor: deletes the managed resource
    ~ControlBlock() override { delete managed; }

    // Report the address of the managed resource
    void* managedAddress() override { return managed; }

private:
    T* managed;
};

// The user-facing shared pointer
template <typename T>
class SharedPtr {
public:
    // Default constructor: empty SharedPtr, nothing managed
    SharedPtr() : stored(nullptr), ctrl(nullptr) {}

    // Constructor from a raw pointer: takes ownership, refcount starts at 1
    explicit SharedPtr(T* p) : stored(p), ctrl(p ? new ControlBlock<T>(p) : nullptr) {}

    // Copy constructor: shares ownership, bumps refcount
    SharedPtr(const SharedPtr& other) : stored(other.stored), ctrl(other.ctrl) {
        if (ctrl) ctrl->increment();
    }

    // Move constructor: steals ownership, refcount unchanged
    SharedPtr(SharedPtr&& other) noexcept : stored(other.stored), ctrl(other.ctrl) {
        other.stored = nullptr;
        other.ctrl = nullptr;
    }

    // Destructor: releases our share of ownership
    ~SharedPtr() { release(); }

    // Copy/move assignment via copy-and-swap: self-assignment safe by construction
    SharedPtr& operator=(SharedPtr other) noexcept {
        swap(other);
        return *this;
    }

    // Dereference operators
    T& operator*() const {
        assert(stored != nullptr);
        return *stored;
    }

    T* operator->() const {
        assert(stored != nullptr);
        return stored;
    }

    // Get the raw stored pointer without giving up ownership
    T* get() const { return stored; }

    // Comparison: compares the stored pointers
    bool operator==(const SharedPtr& other) const { return stored == other.stored; }
    bool operator!=(const SharedPtr& other) const { return !(*this == other); }

    // Check if non-empty
    explicit operator bool() const { return stored != nullptr; }

    // Swap managed pointers/control blocks (no refcount changes)
    void swap(SharedPtr& other) noexcept {
        std::swap(stored, other.stored);
        std::swap(ctrl, other.ctrl);
    }

    // Release ownership of the current resource, becoming empty
    void reset() {
        SharedPtr tmp;
        swap(tmp);
    }

    // Release ownership of the current resource, take ownership of a new one
    void reset(T* p) {
        if (p == stored) return;  // guard: avoid a second control block owning the same address
        SharedPtr tmp(p);
        swap(tmp);
    }

    // Get the current refcount
    long useCount() const { return ctrl ? ctrl->refCount() : 0; }

private:
    // Decrement refcount, delete the control block if we were the last owner
    void release() {
        if (ctrl && ctrl->decrement() == 0) {
            delete ctrl;
        }
        stored = nullptr;
        ctrl = nullptr;
    }

    T* stored;
    ControlBlockBase* ctrl;
};

// Allocates a new T on the heap and wraps it in a SharedPtr
template <typename T, typename... Args>
SharedPtr<T> makeSharedBasic(Args&&... args) {
    return SharedPtr<T>(new T(std::forward<Args>(args)...));
}

#endif