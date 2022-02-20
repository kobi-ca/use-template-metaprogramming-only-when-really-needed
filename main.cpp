#include <iostream>
#include <array>
#include <memory>

// To explain runtime dynamic polymorphism vs CRTP, compile time
namespace runtime_poly {

    class Base {
    public:
        virtual ~Base() = default;
        virtual void do_something() const = 0;
    };

    class Derived1 : public Base {
    public:
        void do_something() const override {
            std::clog << "Derived1::do_something\n";
        }
    };

    class Derived2 : public Base {
    public:
        void do_something() const override {
            std::clog << "Derived2::do_something\n";
        }
    };

    void using_poly_example () {
        constexpr auto NUM = 2U;
        std::array<std::unique_ptr<Base>, NUM> arr {std::make_unique<Derived1>(), std::make_unique<Derived2>()};
        for (const auto& p : arr) {
            p->do_something();
        }
    }

}

namespace explaining_crtp {

    template <typename Derived>
    class Base {
    public:
        // Nothing is virtual, no virtual dtor please ...
        void do_something() const {
            static_cast<const Derived*>(this)->do_something();
        }
    };

    // curious reoccurring pattern
    class Derived1 : public Base<Derived1> { // putting "Self"
    public:
        void do_something() const {
            std::clog << "crtp Derived1::do_something\n";
        }
    };

    class Derived2 : public Base<Derived2> { // putting "Self"
    public:
        void do_something() const {
            std::clog << "crtp Derived2::do_something\n";
        }
    };

    template <typename T>
    void do_something(const Base<T>& b) {
        b.do_something();
    }

    void using_crtp_example () {
        Derived1 derived1;
        Derived2 derived2;
        do_something(derived1);
        do_something(derived2);
    }
}

namespace Original {
    template<size_t N>
    class vector_f {
    public:
        vector_f() = default;

        explicit vector_f(std::initializer_list<float> init) : data_(init) {}

        float operator[](size_t i) const { return data_[i]; } // read-only accessor
        float &operator[](size_t i) { return data_[i]; } // read-write accessor
        [[nodiscard]] // book has const size_t which is a clang-tidy error
        size_t size() { return data_.size(); } // extract the size parameter
    private:
        std::array<float, N> data_{};
    };

    template<size_t N>
    vector_f<N> operator+(vector_f<N> const &u, vector_f<N> const &v) {
        vector_f<N> sum;
        for (size_t i = 0; i < N; i++) {
            sum[i] = u[i] + v[i];
        }
        return sum;
    }
}

namespace CRTP {

    // with CRTP
    // book has struct E which does not compile.
    //  it's either class or typename
    template<typename E>
    class vector_expression {
    public:
        float operator[](size_t i) const {
            return static_cast<E const &>(*this)[i];
        }
        [[nodiscard]]
        size_t size() const {
            return static_cast<E const &>(*this).size(); // error was missing () for the .size
        }
    };

    template<size_t N>
    class vector_f
            : public vector_expression<vector_f<N>> {
    public:
        vector_f() = default;
        vector_f(std::initializer_list<float> init) : data_(init) {}
        template<class E>
        explicit vector_f(vector_expression<E> const &e);
        // clang saying this is hiding the base class's op[]const
        float operator[](size_t i) const { return data_[i]; } // read-only accessor
        float &operator[](size_t i) { return data_[i]; } // read-write accessor
        // book had missing const and instead had const on the size_t
        size_t size() const { return data_.size(); } // extract the size parameter
    private:
        std::array<float, N> data_{}; // error - should be <float,N>
    };

    template <size_t N>
    template <class E>
    vector_f<N>::vector_f(vector_expression<E> const& e)
            : data_(e.size()) {
        for (size_t i = 0; i != e.size(); ++i) {
            data_[i] = e[i]; //(1)
        }
    }

    template<class E1, class E2>
    class vector_sum
            : public vector_expression<vector_sum<E1, E2>> {
        E1 const &u_;
        E2 const &v_;
    public:
        vector_sum(E1 const &u, E2 const &v) : u_(u), v_(v) {}
        float operator[](size_t i) const { return u_[i] + v_[i]; } //(2)
        size_t size() const { return v_.size(); }
    };

    template<typename E1, typename E2>
    vector_sum<E1, E2> operator+(vector_expression<E1> const &u,
                                 vector_expression<E2> const &v) {
        return vector_sum<E1, E2>(*static_cast<E1 const *>(&u),
                                  *static_cast<E2 const *>(&v));
    }
}

int main() {
    {
        Original::vector_f<3> a, b, c;
        Original::vector_f<3> v = a + b + c;
        std::clog << v.size() << '\n';
    }

    {
        CRTP::vector_f<3> a, b, c;
        // Book has typo here. it is missing the last vector_f<3>
        auto v1 = a + b + c;
        auto v2 = a + b;
        std::clog << v1.size() << ' ' << v2.size() <<  '\n';
        const auto v2_0 = v2[0];
        const auto v1_0 = v1[0];
        std::clog << v1_0 << ' ' << v2_0 << '\n';
    }

    // runtime example
    runtime_poly::using_poly_example();

    // crtp example
    explaining_crtp::using_crtp_example();
    return 0;
}
