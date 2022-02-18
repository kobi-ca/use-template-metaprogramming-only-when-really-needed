#include <iostream>
#include <array>

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
        auto v = a + b + c;
        std::clog << v.size() << '\n';
    }
    return 0;
}
