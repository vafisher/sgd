#ifndef IMPLICIT_BASEDEF_HPP
#define IMPLICIT_BASEDEF_HPP

#include "RcppArmadillo.h"
#include <boost/function.hpp>
#include <math.h>
#include <string>
#include <cstddef>

using namespace arma;

typedef boost::function<double (double)> uni_func_type;
typedef boost::function<mat (const mat&)> mmult_func_type;
typedef boost::function<double (const mat&, const mat&, const mat&)> deviance_type;

#endif