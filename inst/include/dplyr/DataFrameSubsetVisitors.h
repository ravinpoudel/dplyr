#ifndef dplyr_DataFrameSubsetVisitors_H
#define dplyr_DataFrameSubsetVisitors_H

#include <tools/pointer_vector.h>
#include <tools/match.h>
#include <tools/utils.h>

#include <dplyr/tbl_cpp.h>
#include <dplyr/subset_visitor.h>

namespace dplyr {

  class DataFrameSubsetVisitors {
  private:

    const Rcpp::DataFrame& data;
    pointer_vector<SubsetVectorVisitor> visitors;
    Rcpp::CharacterVector visitor_names;
    int nvisitors;

  public:
    typedef SubsetVectorVisitor visitor_type;

    DataFrameSubsetVisitors(const Rcpp::DataFrame& data_) :
      data(data_),
      visitors(),
      visitor_names(data.names()),
      nvisitors(visitor_names.size())
    {

      for (int i=0; i<nvisitors; i++) {
        SubsetVectorVisitor* v = subset_visitor(data[i]);
        visitors.push_back(v);
      }
    }

    DataFrameSubsetVisitors(const Rcpp::DataFrame& data_, const Rcpp::CharacterVector& names) :
      data(data_),
      visitors(),
      visitor_names(names),
      nvisitors(visitor_names.size())
    {

      IntegerVector indx = r_match(names, data.names());

      int n = indx.size();
      for (int i=0; i<n; i++) {

        int pos = indx[i];
        if (pos == NA_INTEGER) {
          stop("unknown column '%s' ", CHAR(names[i]));
        }

        SubsetVectorVisitor* v = subset_visitor(data[pos - 1]);
        visitors.push_back(v);

      }

    }

    template <typename Container>
    DataFrame subset_impl(const Container& index, const CharacterVector& classes, Rcpp::traits::false_type) const {
      List out(nvisitors);
      for (int k=0; k<nvisitors; k++) {
        out[k] = get(k)->subset(index);
      }
      copy_most_attributes(out, data);
      structure(out, Rf_length(out[0]) , classes);
      return out;
    }

    template <typename Container>
    DataFrame subset_impl(const Container& index, const CharacterVector& classes, Rcpp::traits::true_type) const {
      int n = index.size();
      int n_out = std::count(index.begin(), index.end(), TRUE);
      IntegerVector idx = no_init(n_out);
      for (int i=0, k=0; i<n; i++) {
        if (index[i] == TRUE) {
          idx[k++] = i;
        }
      }
      return subset_impl(idx, classes, Rcpp::traits::false_type());
    }

    template <typename Container>
    inline DataFrame subset(const Container& index, const CharacterVector& classes) const {
      return
        subset_impl(
          index, classes,
          typename Rcpp::traits::same_type<Container, LogicalVector>::type()
        );
    }

    inline int size() const {
      return nvisitors;
    }
    inline SubsetVectorVisitor* get(int k) const {
      return visitors[k];
    }

    Rcpp::String name(int k) const {
      return visitor_names[k];
    }

    inline int nrows() const {
      return data.nrows();
    }

  private:

    inline void structure(List& x, int nrows, CharacterVector classes) const {
      x.attr("class") = classes;
      set_rownames(x, nrows);
      x.names() = visitor_names;
      SEXP vars = data.attr("vars");
      if (!Rf_isNull(vars))
        x.attr("vars") = vars;
    }

  };

  template <typename Index>
  DataFrame subset(DataFrame df, const Index& indices, CharacterVector columns, CharacterVector classes) {
    return DataFrameSubsetVisitors(df, columns).subset(indices, classes);
  }

  template <typename Index>
  DataFrame subset(DataFrame df, const Index& indices, CharacterVector classes) {
    return DataFrameSubsetVisitors(df).subset(indices, classes);
  }

} // namespace dplyr

#include <dplyr/subset_visitor_impl.h>

#endif
