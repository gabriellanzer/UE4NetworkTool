#ifndef __MATH__H__
#define __MATH__H__

#define RV_PI 3.141592653589793
#define RV_GAMA_IT 60

namespace rv
{
	inline double gamma(double z)
	{
		const int a = RV_GAMA_IT;
		static double c_space[RV_GAMA_IT];
		static double* c = nullptr;
		int k;
		double accm;

		if (c == nullptr)
		{
			double k1_factrl = 1.0; /* (k - 1)!*(-1)^k with 0!==1*/
			c = c_space;
			c[0] = sqrt(2.0 * RV_PI);
			for (k = 1; k < a; k++)
			{
				c[k] = exp(a - k) * pow(a - k, k - 0.5) / k1_factrl;
				k1_factrl *= -k;
			}
		}
		accm = c[0];
		for (k = 1; k < a; k++)
		{
			accm += c[k] / (z + k);
		}
		accm *= exp(-(z + a)) * pow(z + a, z + 0.5); /* Gamma(z+1) */
		return accm / z;
	}
} // namespace rv

#endif //!__MATH__H__