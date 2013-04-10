#include <mie/fp_generator.hpp>

int main()
{
	mie::FpGenerator fpGen;
	int (*f)(int, int) = fpGen.getCode<int (*)(int, int)>();
	printf("%d\n", f(5, 9));
}
