# async_sum

Adding two vectors asynchronously element-wise

## Requirements
gcc 9.4.0 

More info on [compiler](#compiler)

## How to compile
```console
root@docker-desktop:/# cd /path/to/async_sum
root@docker-desktop:/async_sum# bash clean_build.sh
```

## How to run
```console
root@docker-desktop:/# cd /path/to/async_sum
root@docker-desktop:/async_sum# ./build/async_sum
```

## clean_build.sh
A trivial shell script that checks and deletes if `build` folder exists.
Calls the cmake and make to build and compile the source code

## CMakeLists.txt
A basic cmake file to specify C++ standard, build type and compile options

## async_sum.cpp

```cpp
template <class Function, class... Args>
auto time_function_execution(Function &&callback, Args &&...args) noexcept
```
Returns the results of the function call and how much time did it take in terms of milliseconds as a [std::pair](https://en.cppreference.com/w/cpp/utility/pair). The arguments are [perfectly forwarded](https://www.modernescpp.com/index.php/perfect-forwarding) using [std::forward](https://en.cppreference.com/w/cpp/utility/forward)

```cpp
constexpr static auto print{[](auto const &v) noexcept
```
prints elements of the given vector using [std::copy](https://en.cppreference.com/w/cpp/algorithm/copy).

```cpp
constexpr static auto print_all{[](auto const &all_results) noexcept
```
takes a vector of vector as an argument and calls `print` for every vector.

```cpp
constexpr static auto print_range{[](auto const &arr, auto const start, auto const end) noexcept
```
prints the specified range of the given vector. The vector should be used with [std::reference_wrapper](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper) using [std::ref or std::cref](https://en.cppreference.com/w/cpp/utility/functional/ref). It is assumed that the given start and end positions are in the valid range. `get()` function is used to get the underlying vector wrapped by the [std::reference_wrapper](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper)

```cpp
namespace etkin
```

The namespace `etkin` contains 3 lambdas. These are:
```cpp
constexpr static auto get_indices{[](auto const processor_count, auto const size) noexcept
```
`processor_count` specifies how many individual cores are present in the system. The value of the processor count is retrieved via [std:: thread::hardware_concurrency](https://en.cppreference.com/w/cpp/thread/thread/hardware_concurrency). On my system, the return value is 12. The `size` is the total number of the elements in the vector. The indices vector constructed in such a way that every adjacent value in the vector represents a range in the vector to be independently added up. First, the range is calculated via dividing the `size` to the `processor_count`. The division is done via [std::ldiv](https://en.cppreference.com/w/cpp/numeric/math/div) that returns the quotient and the reminder in a struct called [std::div_t](https://en.cppreference.com/w/cpp/numeric/math/div#std::div_t). If reminder is not 0, it is added to the last index. For example if the processor_count is 4 and the size is 14, the quotient is 3 and the reminder becomes 2. The ranges are [0,3), [3,6), [6,9), [9,14). The vector of indices is initialized with the quotient first, [3,3,3,3]. Then using [std::partial_sum](https://en.cppreference.com/w/cpp/algorithm/partial_sum) the indices are converted to [3, 6, 9, 12]. Since the reminder is not zero, it gets added to the last element, so the indices become [3, 6, 9, 14]. Lastly, a 0 is inserted at the beginning of the indices to cover the first range. [0, 3, 6, 9, 14]

```cpp
constexpr static auto range_sum{[](auto const &in1, auto const &in2, auto const start, auto const end) noexcept
```
Adds the specified range of the given two vectors element-wise. The vectors should be used with [std::reference_wrapper](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper) using [std::ref or std::cref](https://en.cppreference.com/w/cpp/utility/functional/ref). It is assumed that the given start and end positions are in the valid range. `get()` function is used in order to get the underlying vector wrapped by the [std::reference_wrapper](https://en.cppreference.com/w/cpp/utility/functional/reference_wrapper)

```cpp
constexpr static auto async_sum{[](auto const &in1, auto const &in2, auto const processor_count) noexcept
```
The sizes and the data types of the vectors are assumed to be same. The `processor_count` is assumed to be greater than or equal to 1. If the sizes of the vectors are lesser than the `processor_count` the `range_sum` lambda is directly called. If not, a vector of [std::future](https://en.cppreference.com/w/cpp/thread/future) unique pointers is created. with the size of `processor_count` to acces the results of the asynchronous calls to the `range_sum`. Then every subrange of this vector is passed to the `range_sum` via [std::tranform](https://en.cppreference.com/w/cpp/algorithm/transform) asynchronously.

Call to [std::async](https://en.cppreference.com/w/cpp/thread/async) with enumeration [std::launch::async](https://en.cppreference.com/w/cpp/thread/launch) means that launch the passed function as soon as possible. 

Two iterators given to [std::tranform](https://en.cppreference.com/w/cpp/algorithm/transform) points to the same vector, indices. The first iterator starts from the beginning of the vector and goes up to the second last element. The second iterator starts from the second element from the beginning. So every in every iteration, if the first iterator is pointing to the ith element, the second iterator points to the i+1 th element.

1st iteration
$$[\underset{\substack{\textstyle\uparrow}}{0}, \\ \underset{\substack{\textstyle\uparrow}}{3}, \\ 6, \\ 9, \\ 14]$$

2nd iteration
$$[0, \\ \underset{\substack{\textstyle\uparrow}}{3}, \\ \underset{\substack{\textstyle\uparrow}}{6}, \\ 9, \\ 14]$$

3rd iteration
$$[0, \\ 3, \\ \underset{\substack{\textstyle\uparrow}}{6}, \\ \underset{\substack{\textstyle\uparrow}}{9}, \\ 14]$$

4th iteration
$$[0, \\ 3, \\ 6, \\ \underset{\substack{\textstyle\uparrow}}{9}, \\ \underset{\substack{\textstyle\uparrow}}{14}]$$

So the `range_sum` lambda is called with the [0,3), [3,6), [6,9), [9,14) parameters. 

The results are stacked to std::vector of the results type of the `range_sum` in a variable called `all_results` (std::vector<std::vector<double>> in this repo).

```cpp
int main(int argc, char const *argv[])
```
Nested for loop generates vector lengths. Starts from 12. goes up to 12 million and covers integer multiples of the number 12. The example vectors are generated using [std::iota](https://en.cppreference.com/w/cpp/algorithm/iota). The generated vectors are  $[0,1,2,3,4,5,6,7...n-2, n-1]$ and $[n-1, n-2,n-3... 2,1,0]$. The length and the execution time values are gets written to a file in csv format.

## Compiler
```console
root@docker-desktop:/async_sum$ g++ -v
Using built-in specs.
COLLECT_GCC=g++
COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-linux-gnu/9/lto-wrapper
OFFLOAD_TARGET_NAMES=nvptx-none:hsa
OFFLOAD_TARGET_DEFAULT=1
Target: x86_64-linux-gnu
Configured with: ../src/configure -v --with-pkgversion='Ubuntu 9.4.0-1ubuntu1~20.04.1' --with-bugurl=file:///usr/share/doc/gcc-9/README.Bugs --enable-languages=c,ada,c++,go,brig,d,fortran,objc,obj-c++,gm2 --prefix=/usr --with-gcc-major-version-only --program-suffix=-9 --program-prefix=x86_64-linux-gnu- --enable-shared --enable-linker-build-id --libexecdir=/usr/lib --without-included-gettext --enable-threads=posix --libdir=/usr/lib --enable-nls --enable-clocale=gnu --enable-libstdcxx-debug --enable-libstdcxx-time=yes --with-default-libstdcxx-abi=new --enable-gnu-unique-object --disable-vtable-verify --enable-plugin --enable-default-pie --with-system-zlib --with-target-system-zlib=auto --enable-objc-gc=auto --enable-multiarch --disable-werror --with-arch-32=i686 --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --with-tune=generic --enable-offload-targets=nvptx-none=/build/gcc-9-Av3uEd/gcc-9-9.4.0/debian/tmp-nvptx/usr,hsa --without-cuda-driver --enable-checking=release --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu
Thread model: posix
gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1)
```

```console
root@docker-desktop:/async_sum$ gcc -v
Using built-in specs.
COLLECT_GCC=gcc
COLLECT_LTO_WRAPPER=/usr/lib/gcc/x86_64-linux-gnu/9/lto-wrapper
OFFLOAD_TARGET_NAMES=nvptx-none:hsa
OFFLOAD_TARGET_DEFAULT=1
Target: x86_64-linux-gnu
Configured with: ../src/configure -v --with-pkgversion='Ubuntu 9.4.0-1ubuntu1~20.04.1' --with-bugurl=file:///usr/share/doc/gcc-9/README.Bugs --enable-languages=c,ada,c++,go,brig,d,fortran,objc,obj-c++,gm2 --prefix=/usr --with-gcc-major-version-only --program-suffix=-9 --program-prefix=x86_64-linux-gnu- --enable-shared --enable-linker-build-id --libexecdir=/usr/lib --without-included-gettext --enable-threads=posix --libdir=/usr/lib --enable-nls --enable-clocale=gnu --enable-libstdcxx-debug --enable-libstdcxx-time=yes --with-default-libstdcxx-abi=new --enable-gnu-unique-object --disable-vtable-verify --enable-plugin --enable-default-pie --with-system-zlib --with-target-system-zlib=auto --enable-objc-gc=auto --enable-multiarch --disable-werror --with-arch-32=i686 --with-abi=m64 --with-multilib-list=m32,m64,mx32 --enable-multilib --with-tune=generic --enable-offload-targets=nvptx-none=/build/gcc-9-Av3uEd/gcc-9-9.4.0/debian/tmp-nvptx/usr,hsa --without-cuda-driver --enable-checking=release --build=x86_64-linux-gnu --host=x86_64-linux-gnu --target=x86_64-linux-gnu
Thread model: posix
gcc version 9.4.0 (Ubuntu 9.4.0-1ubuntu1~20.04.1)
```
