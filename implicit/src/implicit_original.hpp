#ifndef IMPLICIT_H
#define IMPLICIT_H

#include "RcppArmadillo.h"
#include <boost/math/tools/roots.hpp>
#include <boost/function.hpp>
#include <boost/bind/bind.hpp>
#include <boost/ref.hpp>
#include <math.h>
#include <string>
#include <cstddef>
#include <iostream>

using namespace arma;

/*
#if __cplusplus == 199711L
  #define nullptr NULL
#endif
#define DEBUG 1
*/

struct Imp_DataPoint;
struct Imp_Dataset;
struct Imp_OnlineOutput;
struct Imp_Experiment;
struct Imp_Size;

struct Imp_Identity_Transfer;
struct Imp_Exp_Transfer;
struct Imp_Logistic_Transfer;

struct Imp_Unidim_Learn_Rate;
struct Imp_Pxdim_Learn_Rate;

typedef boost::function<double (double)> uni_func_type;
typedef boost::function<mat (const mat&)> mmult_func_type;
typedef boost::function<mat (const mat&, const Imp_DataPoint&, double)> score_func_type;
typedef boost::function<mat (const mat&, const Imp_DataPoint&, double, unsigned, unsigned)> learning_rate_type;
typedef boost::function<double (const mat&, const mat&, const mat&)> deviance_type;

struct Imp_DataPoint {
  Imp_DataPoint(): x(mat()), y(0) {}
  Imp_DataPoint(mat xin, double yin):x(xin), y(yin) {}
//@members
  mat x;
  double y;
};

struct Imp_Dataset
{
  Imp_Dataset():X(mat()), Y(mat()) {}
  Imp_Dataset(mat xin, mat yin):X(xin), Y(yin) {}
//@members
  mat X;
  mat Y;
//@methods
  mat covariance() const {
    return cov(X);
  }

  friend std::ostream& operator<<(std::ostream& os, const Imp_Dataset& dataset) {
    os << "  Dataset:\n" << "    X has " << dataset.X.n_cols << " features\n" << 
          "    Total of " << dataset.X.n_rows << " data points" << std::endl;
    return os;
  }
};

struct Imp_OnlineOutput{
  //Construct Imp_OnlineOutput compatible with
  //the shape of data
  Imp_OnlineOutput(const Imp_Dataset& data, const mat& init)
   :estimates(mat(data.X.n_cols, data.X.n_rows)), initial(init) {}

  Imp_OnlineOutput(){}
//@members
  mat estimates;
  mat initial;
//@methods
  mat last_estimate(){
    return estimates.col(estimates.n_cols-1);
  }
};

/* 1 dimension (scalar) learning rate, suggested in Xu's paper
 */
struct Imp_Unidim_Learn_Rate
{
  static mat learning_rate(const mat& theta_old, const Imp_DataPoint& data_pt, double offset,
                          unsigned t, unsigned p,
                          double gamma, double alpha, double c, double scale) {
    double lr = scale * gamma * pow(1 + alpha * gamma * t, -c);
    mat lr_mat = mat(p, p, fill::eye) * lr;
    return lr_mat;
  }
};

// p dimension learning rate
struct Imp_Pxdim_Learn_Rate
{
  static mat Idiag;

  static mat learning_rate(const mat& theta_old, const Imp_DataPoint& data_pt, double offset,
                          unsigned t, unsigned p,
                          score_func_type score_func) {
    mat Gi = score_func(theta_old, data_pt, offset);
    Idiag = Idiag + diagmat(Gi * Gi.t());
    mat Idiag_inv(Idiag);

    for (unsigned i = 0; i < p; ++i) {
      if (abs(Idiag_inv.at(i, i)) > 1e-8) {
        Idiag_inv.at(i, i) = 1. / Idiag_inv.at(i, i);
      }
    }
    return Idiag_inv;
  }

  static void reinit(unsigned p) {
    Idiag = mat(p, p, fill::eye);
  }
};

mat Imp_Pxdim_Learn_Rate::Idiag = mat();

// Identity transfer function
struct Imp_Identity_Transfer {
  static double transfer(double u) {
    return u;
  }

  static mat transfer(const mat& u) {
    return u;
  }

  static double first_derivative(double u) {
    return 1.;
  }

  static double second_derivative(double u) {
    return 0.;
  }

  static bool valideta(double eta){
    return true;
  }
};

// Exponentional transfer function
struct Imp_Exp_Transfer {
  static double transfer(double u) {
    return exp(u);
  }

  static mat transfer(const mat& u) {
    mat result = mat(u);
    for (unsigned i = 0; i < result.n_rows; ++i) {
      result(i, 0) = transfer(u(i, 0));
    }
    return result;
  }

  static double first_derivative(double u) {
    return exp(u);
  }

  static double second_derivative(double u) {
    return exp(u);
  }

  static bool valideta(double eta){
    return true;
  }
};

// Logistic transfer function
struct Imp_Logistic_Transfer {
  static double transfer(double u) {
    return sigmoid(u);
  }

  static mat transfer(const mat& u) {
    mat result = mat(u);
    for (unsigned i = 0; i < result.n_rows; ++i) {
      result(i, 0) = transfer(u(i, 0));
    }
    return result;
  }

  static double first_derivative(double u) {
    double sig = sigmoid(u);
    return sig * (1. - sig);
  }

  static double second_derivative(double u) {
    double sig = sigmoid(u);
    return 2*pow(sig, 3) - 3*pow(sig, 2) + 2*sig;
  }

  static bool valideta(double eta){
    return true;
  }

private:
  // sigmoid function
  static double sigmoid(double u) {
      return 1. / (1. + exp(-u));
  }
};

// gaussian model family
struct Imp_Gaussian {
  static std::string family;
  
  static double variance(double u) {
    return 1.;
  }

  static double deviance(const mat& y, const mat& mu, const mat& wt) {
    return sum(vec(wt % ((y-mu) % (y-mu))));
  }
};

std::string Imp_Gaussian::family = "gaussian";

// poisson model family
struct Imp_Poisson {
  static std::string family;
  
  static double variance(double u) {
    return u;
  }

  static double deviance(const mat& y, const mat& mu, const mat& wt) {
    vec r = vec(mu % wt);
    for (unsigned i = 0; i < r.n_elem; ++i) {
      if (y(i) > 0.) {
        r(i) = wt(i) * (y(i) * log(y(i)/mu(i)) - (y(i) - mu(i)));
      }
    }
    return sum(2. * r);
  }
};

std::string Imp_Poisson::family = "poisson";

// binomial model family
struct Imp_Binomial {
  static std::string family;
  
  static double variance(double u) {
    return u * (1. - u);
  }

  // In R the dev.resids of Binomial family is not exposed.
  // Found one [here](http://pages.stat.wisc.edu/~st849-1/lectures/GLMDeviance.pdf)
  static double deviance(const mat& y, const mat& mu, const mat& wt) {
    vec r(y.n_elem);
    for (unsigned i = 0; i < r.n_elem; ++i) {
      r(i) = 2. * wt(i) * (y_log_y(y(i), mu(i)) + y_log_y(1.-y(i), 1.-mu(i)));
    }
    return sum(r);
  }
private:
  static double y_log_y(double y, double mu) {
    return (y) ? (y * log(y/mu)) : 0.;
  }
};

std::string Imp_Binomial::family = "binomial";

struct Imp_Experiment {
//@members
  unsigned p;
  unsigned n_iters;
  std::string model_name;
  std::string transfer_name;
  std::string lr_type;
  mat offset;
  mat weights;
  mat start;
  double epsilon;
  bool trace;
  bool dev;
  bool convergence;

//@methods
  Imp_Experiment(std::string m_name, std::string tr_name)
  :model_name(m_name), transfer_name(tr_name) {
    if (model_name == "gaussian") {
      variance_ = boost::bind(&Imp_Gaussian::variance, _1);
      deviance_ = boost::bind(&Imp_Gaussian::deviance, _1, _2, _3);
    }
    else if (model_name == "poisson") {
      variance_ = boost::bind(&Imp_Poisson::variance, _1);
      deviance_ = boost::bind(&Imp_Poisson::deviance, _1, _2, _3);
    }
    else if (model_name == "binomial") {
      variance_ = boost::bind(&Imp_Binomial::variance, _1);
      deviance_ = boost::bind(&Imp_Binomial::deviance, _1, _2, _3);
    }

    if (transfer_name == "identity") {
      // transfer() 's been overloaded, have to specify the function signature
      transfer_ = boost::bind(static_cast<double (*)(double)>(
                      &Imp_Identity_Transfer::transfer), _1);
      mat_transfer_ = boost::bind(static_cast<mat (*)(const mat&)>(
                      &Imp_Identity_Transfer::transfer), _1);
      transfer_first_deriv_ = boost::bind(
                      &Imp_Identity_Transfer::first_derivative, _1);
      transfer_second_deriv_ = boost::bind(
                      &Imp_Identity_Transfer::second_derivative, _1);
      valideta_ = boost::bind(&Imp_Identity_Transfer::valideta, _1);
    }
    else if (transfer_name == "exp") {
      transfer_ = boost::bind(static_cast<double (*)(double)>(
                      &Imp_Exp_Transfer::transfer), _1);
      mat_transfer_ = boost::bind(static_cast<mat (*)(const mat&)>(
                      &Imp_Exp_Transfer::transfer), _1);
      transfer_first_deriv_ = boost::bind(
                      &Imp_Exp_Transfer::first_derivative, _1);
      transfer_second_deriv_ = boost::bind(
                      &Imp_Exp_Transfer::second_derivative, _1);
      valideta_ = boost::bind(&Imp_Exp_Transfer::valideta, _1);
    }
    else if (transfer_name == "logistic") {
      transfer_ = boost::bind(static_cast<double (*)(double)>(
                      &Imp_Logistic_Transfer::transfer), _1);
      mat_transfer_ = boost::bind(static_cast<mat (*)(const mat&)>(
                      &Imp_Logistic_Transfer::transfer), _1);
      transfer_first_deriv_ = boost::bind(
                      &Imp_Logistic_Transfer::first_derivative, _1);
      transfer_second_deriv_ = boost::bind(
                      &Imp_Logistic_Transfer::second_derivative, _1);
      valideta_ = boost::bind(&Imp_Logistic_Transfer::valideta, _1);
    }
  }

  void init_uni_dim_learning_rate(double gamma, double alpha, double c, double scale) {
    lr_ = boost::bind(&Imp_Unidim_Learn_Rate::learning_rate, 
                      _1, _2, _3, _4, _5, gamma, alpha, c, scale);
    lr_type = "Uni-dimension learning rate";
  }

  void init_px_dim_learning_rate() {
    score_func_type score_func = boost::bind(&Imp_Experiment::score_function, this, _1, _2, _3);
    lr_ = boost::bind(&Imp_Pxdim_Learn_Rate::learning_rate,
                      _1, _2, _3, _4, _5, score_func);
    lr_type = "Px-dimension learning rate";
  }

  mat learning_rate(const mat& theta_old, const Imp_DataPoint& data_pt, double offset, unsigned t) const {
    //return lr(t);
    return lr_(theta_old, data_pt, offset, t, p);
  }

  mat score_function(const mat& theta_old, const Imp_DataPoint& datapoint, double offset) const {
    return ((datapoint.y - h_transfer(as_scalar(datapoint.x * theta_old))+offset)*datapoint.x).t();
  }

  double h_transfer(double u) const {
    return transfer_(u);
  }

  mat h_transfer(const mat& u) const {
    return mat_transfer_(u);
  }

  //YKuang
  double h_first_derivative(double u) const{
    return transfer_first_deriv_(u);
  }
  //YKuang
  double h_second_derivative(double u) const{
    return transfer_second_deriv_(u);
  }

  double variance(double u) const {
    return variance_(u);
  }

  double deviance(const mat& y, const mat& mu, const mat& wt) const {
    return deviance_(y, mu, wt);
  }

  bool valideta(double eta) const{
    return valideta_(eta);
  }

  friend std::ostream& operator<<(std::ostream& os, const Imp_Experiment& exprm) {
    os << "  Experiment:\n" << "    Family: " << exprm.model_name << "\n" <<
          "    Transfer function: " << exprm.transfer_name <<  "\n" <<
          "    Learning rate: " << exprm.lr_type << "\n\n" <<
          "    Trace: " << (exprm.trace ? "On" : "Off") << "\n" <<
          "    Deviance: " << (exprm.dev ? "On" : "Off") << "\n" <<
          "    Convergence: " << (exprm.convergence ? "On" : "Off") << "\n" <<
          "    Epsilon: " << exprm.epsilon << "\n" << std::endl;
    return os;
  }

private:
  uni_func_type transfer_;
  mmult_func_type mat_transfer_;
  uni_func_type transfer_first_deriv_;
  uni_func_type transfer_second_deriv_;

  learning_rate_type lr_;

  uni_func_type variance_;
  deviance_type deviance_;
  boost::function<bool (double)> valideta_;
};

struct Imp_Size{
  Imp_Size():nsamples(0), p(0){}
  Imp_Size(unsigned nin, unsigned pin):nsamples(nin), p(pin) {}
  unsigned nsamples;
  unsigned p;
};

// Compute score function coeff and its derivative for Implicit-SGD update
struct Get_score_coeff{

  //Get_score_coeff(const Imp_Experiment<TRANSFER>& e, const Imp_DataPoint& d,
  Get_score_coeff(const Imp_Experiment& e, const Imp_DataPoint& d,
      const mat& t, double n, double off) : experiment(e), datapoint(d), theta_old(t), normx(n), offset(off) {}

  double operator() (double ksi) const{
    return datapoint.y-experiment.h_transfer(dot(theta_old, datapoint.x)
                     + normx * ksi +offset);
  }

  double first_derivative (double ksi) const{
    return experiment.h_first_derivative(dot(theta_old, datapoint.x)
           + normx * ksi + offset)*normx;
  }

  double second_derivative (double ksi) const{
    return experiment.h_second_derivative(dot(theta_old, datapoint.x)
             + normx * ksi + offset)*normx*normx;
  }

  const Imp_Experiment& experiment;
  const Imp_DataPoint& datapoint;
  const mat& theta_old;
  double normx;
  double offset;
};

// Root finding functor for Implicit-SGD update
struct Implicit_fn{
  typedef boost::math::tuple<double, double, double> tuple_type;

  //Implicit_fn(double a, const Get_score_coeff<TRANSFER>& get_score): at(a), g(get_score){}
  Implicit_fn(double a, const Get_score_coeff& get_score): at(a), g(get_score){}
  tuple_type operator() (double u) const{
    double value = u - at * g(u);
    double first = 1 + at * g.first_derivative(u);
    double second = at * g.second_derivative(u);
    tuple_type result(value, first, second);
    return result;
  }
  
  double at;
  //const Get_score_coeff<TRANSFER>& g;
  const Get_score_coeff& g;
};

#endif