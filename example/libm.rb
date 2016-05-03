module LibMath
  extend Fiddle::Importer

  dlload "libm.so"

  extern "double acos(double)"
  extern "double acosh(double)"
  extern "double asin(double)"
  extern "double asinh(double)"
  extern "double atan(double)"
  extern "double atan2(double, double)"
  extern "double cbrt(double)"
  extern "double cos(double)"
  extern "double cosh(double)"
  extern "double erf(double)"
  extern "double erfc(double)"
  extern "double exp(double)"
  extern "double frexp(double, int *)"
  extern "double hypot(double, double)"
  extern "double ldexp(double, int)"
  extern "double log(double)"
  extern "double log2(double)"
  extern "double log10(double)"
  extern "double sin(double)"
  extern "double sinh(double)"
  extern "double sqrt(double)"
  extern "double tan(double)"
  extern "double tanh(double)"
end
