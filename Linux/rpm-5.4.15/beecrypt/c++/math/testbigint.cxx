#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "beecrypt/c++/math/BigInteger.h"
using beecrypt::math::BigInteger;

int main()
{
	int failures = 0;

	std::cout << BigInteger::TEN.add(BigInteger::TEN).negate().toString() << std::endl;
	std::cout << BigInteger::ONE.add(BigInteger::TEN.negate()).toString() << std::endl;
	std::cout << BigInteger::TEN.add(BigInteger::ONE.negate()).toString() << std::endl;
	std::cout << BigInteger::TEN.subtract(BigInteger::TEN).toString() << std::endl;
	std::cout << BigInteger::ZERO.subtract(BigInteger::TEN).toString() << std::endl;
	std::cout << BigInteger::ONE.subtract(BigInteger::TEN).toString() << std::endl;
	std::cout << BigInteger::TEN.subtract(BigInteger::ONE).toString() << std::endl;
	std::cout << BigInteger::TEN.multiply(BigInteger::TEN).subtract(BigInteger::ONE).toString() << std::endl;
	std::cout << BigInteger::valueOf(97).toString() << std::endl;
	BigInteger n = BigInteger::ONE.negate();
	BigInteger m = n.add(BigInteger::ONE);
	if (m == BigInteger::ZERO)
		printf("success\n");
	else
		failures++;

	return failures;
}
