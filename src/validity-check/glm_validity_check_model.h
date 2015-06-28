#ifndef VALIDITY_CHECK_GLM_VALIDITY_CHECK_MODEL_H
#define VALIDITY_CHECK_GLM_VALIDITY_CHECK_MODEL_H

#include "basedef.h"
#include "data/data_set.h"
#include "model/glm_model.h"

bool validity_check_model(const data_set& data, const mat& theta, unsigned t,
  const glm_model& model) {
  // Check if eta is in the support.
  double eta = dot(data.get_data_point(t).x, theta);
  if (!model.valideta(eta)) {
    Rcpp::Rcout << "no valid set of coefficients has been found: please supply starting values" << t << std::endl;
    return false;
  }

  // Check the variance of the expectation of Y.
  double mu_var = model.variance(model.h_transfer(eta));
  if (!is_finite(mu_var)) {
    Rcpp::Rcout << "NA in V(mu) in iteration " << t << std::endl;
    Rcpp::Rcout << "current theta: " << theta << std::endl;
    Rcpp::Rcout << "current eta: " << eta << std::endl;
    return false;
  }
  // if (mu_var == 0) {
  //   Rcpp::Rcout << "0 in V(mu) in iteration" << t << std::endl;
  //   Rcpp::Rcout << "current theta: " << theta << std::endl;
  //   Rcpp::Rcout << "current eta: " << eta << std::endl;
  //   return false;
  // }
  double deviance = 0;
  mat mu;
  mat eta_mat;

  // Check the deviance.
  if (model.dev) {
    eta_mat = data.X * theta;
    mu = model.h_transfer(eta_mat);
    deviance = model.deviance(data.Y, mu, model.weights);
    if(!is_finite(deviance)) {
      Rcpp::Rcout << "Deviance is non-finite" << std::endl;
      return false;
    }
  }

  // Print if trace.
  if (model.trace) {
    if (!model.dev) {
      eta_mat = data.X * theta;
      mu = model.h_transfer(eta_mat);
      deviance = model.deviance(data.Y, mu, model.weights);
    }
    Rcpp::Rcout << "Deviance = " << deviance << " , Iterations - " << t << std::endl;
  }
  return true;
}

#endif
