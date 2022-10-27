#include <iostream>
#include <algorithm>
#include <numeric>
#include <vector>
#include <array>
#include <iterator>
#include <functional>
#include <chrono>
#include <future>
#include <thread>
#include <fstream>


namespace utility
{

    template <class Function, class... Args>
    auto time_function_execution(Function &&callback, Args &&...args) noexcept
    {
        auto t1 = std::chrono::high_resolution_clock::now();
        auto const result{callback(std::forward<Args>(args)...)};
        auto t2 = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> ms_double = t2 - t1;
        // std::cout << ms_double.count() << "ms\n";
        return std::make_pair(result, ms_double.count());
    }

    constexpr static auto print{[](auto const &v) noexcept
    {
        using A = std::remove_reference_t<decltype(v)>;
        using B = std::remove_const_t<A>;
        using C = typename B::value_type;
        std::cout << '[';
        std::copy(std::cbegin(v), std::cend(v) - 1, std::ostream_iterator<C>(std::cout, ", "));
        std::cout << v.back() << "]\n";
    }};

    constexpr static auto print_all{[](auto const &all_results) noexcept
    {
        for (auto const &vec : all_results)
        {
            print(vec);
        }
    }};

    constexpr static auto print_range{[](auto const &arr, auto const start, auto const end) noexcept
    {
        auto const &a{arr.get()};
        using A = std::remove_reference_t<decltype(a)>;
        using B = std::remove_const_t<A>;
        using C = typename B::value_type;
        std::cout << '[';
        std::copy(std::cbegin(a) + start, std::cbegin(a) + end - 1 , std::ostream_iterator<C>(std::cout, ", "));
        std::cout << a.at(end) << "]\n";
    }};

} // namespace utility

namespace etkin
{
    constexpr static auto get_indices{[](auto const processor_count, auto const size) noexcept
    {
        auto const dv{std::ldiv(size, processor_count)};
        std::vector<size_t> indices(processor_count, dv.quot); // n,n,n,n,n,n,n,n... size=PROC_COUNT
        std::partial_sum(std::begin(indices), std::end(indices), std::begin(indices)); // n,2n,3n,4n.... size=PROC_COUNT
        indices.back() += dv.rem;
        indices.insert(std::begin(indices), 0);                                        // 0,n,2n,3n,4n.... size=PROC_COUNT+1
        return indices;
    }};

    constexpr static auto range_sum{[](auto const &in1, auto const &in2, auto const start, auto const end) noexcept
    {
        auto const &a{in1.get()};
        auto const &b{in2.get()};
        using A = std::remove_reference_t<decltype(a)>;
        using B = std::remove_const_t<A>;
        using C = typename B::value_type;
        B results(end - start);
        std::transform(std::cbegin(a) + start, std::cbegin(a) + end, std::cbegin(b) + start, std::begin(results), std::plus<C>{});
        return results;
    }};
    
    constexpr static auto async_sum{[](auto const &in1, auto const &in2, auto const processor_count) noexcept
    {
        using A = std::remove_reference_t<decltype(in1)>;
        using B = std::remove_const_t<A>;
        using C = typename B::value_type;
        auto const size{in1.size()};
        if (size < processor_count)
        {
            return std::vector<B>{range_sum(std::cref(in1), std::cref(in2), 0, size)};
        }
        std::vector<std::unique_ptr<std::future<B>>> fupv(processor_count);
        auto const indices{get_indices(processor_count, size)};
        std::transform(std::cbegin(indices), std::cend(indices) - 1, std::cbegin(indices) + 1, std::begin(fupv),
        [&](auto const start, auto const end) noexcept
        {
            return std::make_unique<std::future<B>>(std::async(std::launch::async, range_sum, std::cref(in1), std::cref(in2), start, end));
        });
        std::vector<B> all_results(processor_count);
        std::transform(std::begin(fupv), std::end(fupv), std::begin(all_results), [&](auto &future)
        {
            return future->get();
        });
        return all_results;
    }};
} // namespace etkin

int main(int argc, char const *argv[])
{
    static auto const processor_count{std::thread::hardware_concurrency()}; // 12 on my system
    std::cout << "processor_count: " <<processor_count << std::endl;
    std::ofstream file{"data.csv"};
    file << "length,async_sum,range_sum\n";
    for (auto i{12}; i < 12'000'000; i *= 10)
    {
        for (auto j{1}; j < 9; ++j)
        {
            auto const length{i * j};
            std::cout << "length: " << length << std::endl;
            std::vector<double> in1(length);
            std::vector<double> in2(length);
            std::iota(std::begin(in1), std::end(in1), 0);   // 0,1,2,3,4,5,6,7...n-2, n-1
            std::iota(std::rbegin(in2), std::rend(in2), 0); // n-1, n-2,n-3... 2,1,0
            auto const res1{utility::time_function_execution(etkin::async_sum, in1, in2, processor_count)};
            auto const res2{utility::time_function_execution(etkin::range_sum, std::cref(in1), std::cref(in2), 0, length)};
            // utility::print_all(res1.first);
            // utility::print(res2.first);
            file << length << ',' << res1.second << ',' << res2.second << '\n';
            std::cout << res1.second << " ms" << std::endl;
            std::cout << res2.second << " ms" << std::endl;
            std::cout << std::endl;
        }
    }
    for (auto i{12'000'000}; i < 76'000'000; i += 1'000'000)
    {
        auto const length{i};
        std::cout << "length: " << length << std::endl;
        std::vector<double> in1(length);
        std::vector<double> in2(length);
        std::iota(std::begin(in1), std::end(in1), 0);   // 0,1,2,3,4,5,6,7...n-2, n-1
        std::iota(std::rbegin(in2), std::rend(in2), 0); // n-1, n-2,n-3... 2,1,0
        auto const res1{utility::time_function_execution(etkin::async_sum, in1, in2, processor_count)};
        auto const res2{utility::time_function_execution(etkin::range_sum, std::cref(in1), std::cref(in2), 0, length)};
        // utility::print_all(res1.first);
        // utility::print(res2.first);
        file << length << ',' << res1.second << ',' << res2.second << '\n';
        std::cout << res1.second << " ms" << std::endl;
        std::cout << res2.second << " ms" << std::endl;
        std::cout << std::endl;
    }
    file.close();
    return 0;
}
