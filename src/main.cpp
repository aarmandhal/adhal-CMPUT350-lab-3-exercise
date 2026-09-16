#include <cassert>
#include <cstdlib>
#include <iostream>

#include "SharedPtr.h"

int main() {
    // 1. Default construction — empty
    SharedPtr<int> empty;
    assert(!empty);
    assert(empty.get() == nullptr);
    assert(empty.useCount() == 0);

    // 2. Construct from a raw pointer, dereference, refcount == 1
    SharedPtr<int> a(new int(5));
    assert(a);
    assert(*a == 5);
    assert(a.useCount() == 1);

    // 3. Copy construction shares ownership, bumps refcount
    SharedPtr<int> b = a;
    assert(a.useCount() == 2);
    assert(b.useCount() == 2);
    assert(a == b);
    *b = 10;
    assert(*a == 10);

    // 4. Destroying one owner drops the count, doesn't free the object
    {
        SharedPtr<int> c = a;
        assert(a.useCount() == 3);
    }  // c destroyed here
    assert(a.useCount() == 2);

    // 5. Move construction steals, doesn't touch refcount
    SharedPtr<int> d = std::move(b);
    assert(d.useCount() == 2);
    assert(!b);
    assert(*d == 10);

    // 6. Copy assignment
    SharedPtr<int> e;
    e = a;
    assert(a.useCount() == 3);
    assert(*e == 10);

    // 7. Move assignment
    SharedPtr<int> f;
    f = std::move(e);
    assert(!e);
    assert(*f == 10);

    // 8. Self-assignment doesn't blow up
    a = a;
    assert(*a == 10);

    // 9. reset() releases ownership
    SharedPtr<int> g = a;
    long before = a.useCount();
    g.reset();
    assert(!g);
    assert(a.useCount() == before - 1);

    // 10. reset(T*) replaces the managed object
    SharedPtr<int> h(new int(1));
    h.reset(new int(99));
    assert(*h == 99);
    assert(h.useCount() == 1);

    // 11. Self-reset with get() must be safe (no double delete / crash)
    int* rawH = h.get();
    h.reset(rawH);
    assert(*h == 99);
    assert(h.useCount() == 1);

    // 12. swap
    SharedPtr<int> x(new int(1));
    SharedPtr<int> y(new int(2));
    x.swap(y);
    assert(*x == 2);
    assert(*y == 1);

    // 13. makeSharedBasic
    SharedPtr<int> m = makeSharedBasic<int>(42);
    assert(*m == 42);
    assert(m.useCount() == 1);

    std::cout << "All tests passed!\n";
    return 0;
}
