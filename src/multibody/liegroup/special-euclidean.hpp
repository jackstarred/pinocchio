//
// Copyright (c) 2016-2018 CNRS
//
// This file is part of Pinocchio
// Pinocchio is free software: you can redistribute it
// and/or modify it under the terms of the GNU Lesser General Public
// License as published by the Free Software Foundation, either version
// 3 of the License, or (at your option) any later version.
//
// Pinocchio is distributed in the hope that it will be
// useful, but WITHOUT ANY WARRANTY; without even the implied warranty
// of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// General Lesser Public License for more details. You should have
// received a copy of the GNU Lesser General Public License along with
// Pinocchio If not, see
// <http://www.gnu.org/licenses/>.

#ifndef __se3_special_euclidean_operation_hpp__
#define __se3_special_euclidean_operation_hpp__

#include <limits>

#include <pinocchio/macros.hpp>
#include "pinocchio/spatial/fwd.hpp"
#include "pinocchio/spatial/se3.hpp"
#include "pinocchio/multibody/liegroup/operation-base.hpp"

#include "pinocchio/multibody/liegroup/vector-space.hpp"
#include "pinocchio/multibody/liegroup/cartesian-product.hpp"
#include "pinocchio/multibody/liegroup/special-orthogonal.hpp"

namespace se3
{
  template<int Dim> struct SpecialEuclideanOperationTpl {};
  template<int Dim> struct traits<SpecialEuclideanOperationTpl<Dim> > {};

  template<> struct traits<SpecialEuclideanOperationTpl<2> > {
    typedef double Scalar;
    enum {
      NQ = 4,
      NV = 3
    };
  };

  // SE(2)
  template<>
  struct SpecialEuclideanOperationTpl<2> : public LieGroupBase <SpecialEuclideanOperationTpl<2> >
  {
    typedef VectorSpaceOperationTpl<2>       R2_t;
    typedef SpecialOrthogonalOperationTpl<2> SO2_t;
    typedef CartesianProductOperation <R2_t, SO2_t> R2crossSO2_t;

    SE3_LIE_GROUP_PUBLIC_INTERFACE(SpecialEuclideanOperationTpl);
    typedef Eigen::Matrix<Scalar,2,2> Matrix2;
    typedef Eigen::Matrix<Scalar,2,1> Vector2;

    template<typename TangentVector, typename Matrix2Like, typename Vector2Like>
    static void exp(const Eigen::MatrixBase<TangentVector> & v,
                    const Eigen::MatrixBase<Matrix2Like> & R,
                    const Eigen::MatrixBase<Vector2Like> & t)
    {
      EIGEN_STATIC_ASSERT_SAME_VECTOR_SIZE(TangentVector_t,TangentVector);
      EIGEN_STATIC_ASSERT_MATRIX_SPECIFIC_SIZE(Matrix2Like, 2, 2);
      EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(Vector2Like, 2);

      typedef typename Matrix2Like::Scalar Scalar;
      const Scalar omega = v(2);
      Scalar cv,sv; SINCOS(omega, &sv, &cv);
      const_cast<Matrix2Like &>(R.derived()) << cv, -sv, sv, cv;

      if (std::fabs(omega) > 1e-14)
      {
        typename EIGEN_PLAIN_TYPE(Vector2Like) vcross(-v(1), v(0));
        vcross /= omega;
        const_cast<Vector2Like &>(t.derived()).noalias() = vcross - R * vcross;
      }
      else
      {
        const_cast<Vector2Like &>(t.derived()) = v.template head<2>();
      }
    }

    template<typename Matrix2Like, typename Vector2Like, typename Matrix3Like>
    static void toInverseActionMatrix(const Eigen::MatrixBase<Matrix2Like> & R,
                                      const Eigen::MatrixBase<Vector2Like> & t,
                                      const Eigen::MatrixBase<Matrix3Like> & M)
    {
      EIGEN_STATIC_ASSERT_MATRIX_SPECIFIC_SIZE(Matrix2Like, 2, 2);
      EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(Vector2Like, 2);
      EIGEN_STATIC_ASSERT_MATRIX_SPECIFIC_SIZE(Matrix3Like, 3, 3);
      
      typedef typename Matrix3Like::Scalar Scalar;
      
      Matrix3Like & Mout = const_cast<Matrix3Like&>(M.derived());
      Mout.template topLeftCorner<2,2>().noalias() = R.transpose();
      typename EIGEN_PLAIN_TYPE(Vector2Like) tinv(R.transpose() * t);
      Mout.template topRightCorner<2,1>() << - tinv(1), tinv(0);
      Mout.template bottomLeftCorner<1,2>().setZero();
      Mout(2,2) = (Scalar)1;
    }

    template<typename Matrix2Like, typename Vector2Like, typename TangentVector>
    static void log(const Eigen::MatrixBase<Matrix2Like> & R,
                    const Eigen::MatrixBase<Vector2Like> & p,
                    const Eigen::MatrixBase<TangentVector> & v)
    {
      EIGEN_STATIC_ASSERT_MATRIX_SPECIFIC_SIZE(Matrix2Like, 2, 2);
      EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(Vector2Like, 2);
      EIGEN_STATIC_ASSERT_SAME_VECTOR_SIZE(TangentVector_t,TangentVector);
      
      TangentVector & vout = const_cast<TangentVector &>(v.derived());

      typedef typename Matrix2Like::Scalar Scalar1;
      
      Scalar1 t = SO2_t::log(R);
      const Scalar1 tabs = std::fabs(t);
      const Scalar1 t2 = t*t;
      Scalar1 alpha;
      if (tabs < 1e-4)
      {
        alpha = 1 - t2/12 - t2*t2/720;
      }
      else
      {
        Scalar1 st,ct; SINCOS(tabs, &st, &ct);
        alpha = tabs*st/(2*(1-ct));
      }

      vout.template head<2>().noalias() = alpha * p;
      vout(0) += t/2 * p(1);
      vout(1) += -t/2 * p(0);
      vout(2) = t;
    }

    template<typename Matrix2Like, typename Vector2Like, typename JacobianOutLike>
    static void Jlog(const Eigen::MatrixBase<Matrix2Like> & R,
                     const Eigen::MatrixBase<Vector2Like> & p,
                     const Eigen::MatrixBase<JacobianOutLike> & J)
    {
      EIGEN_STATIC_ASSERT_MATRIX_SPECIFIC_SIZE(Matrix2Like, 2, 2);
      EIGEN_STATIC_ASSERT_VECTOR_SPECIFIC_SIZE(Vector2Like, 2);
      EIGEN_STATIC_ASSERT_SAME_MATRIX_SIZE(JacobianOutLike, JacobianMatrix_t);
      
      JacobianOutLike & Jout = const_cast<JacobianOutLike &>(J.derived());

      typedef typename Matrix2Like::Scalar Scalar1;
      
      Scalar1 t = SO2_t::log(R);
      const Scalar1 tabs = std::fabs(t);
      Scalar1 alpha, alpha_dot;
      if (tabs < 1e-4)
      {
        Scalar1 t2 = t*t;
        alpha = 1 - t2/12;
        alpha_dot = - t / 6 - t2*t / 180;
      }
      else
      {
        Scalar1 st,ct; SINCOS(t, &st, &ct);
        Scalar1 inv_2_1_ct = 0.5 / (1-ct);
        // t * sin(t) / (2 * (1 - cos(t)) )
        alpha = t*st*inv_2_1_ct;
        // [ ( 1 - cos(t) ) * sin(t) + t * cos(t) - 1 ] / (2 * (1 - cos(t))^2 )
        alpha_dot = (st-t) * inv_2_1_ct;
      }

      typename EIGEN_PLAIN_TYPE(Matrix2Like) V;
      V(0,0) = V(1,1) = alpha;
      V(1,0) = - t / 2;
      V(0,1) = - V(1,0);

      Jout.template topLeftCorner <2,2>().noalias() = V * R;
      Jout.template topRightCorner<2,1>() << alpha_dot*p[0] + p[1]/2, -p(0)/2 + alpha_dot*p(1);
      Jout.template bottomLeftCorner<1,2>().setZero();
      Jout(2,2) = 1;
    }

    /// Get dimension of Lie Group vector representation
    ///
    /// For instance, for SO(3), the dimension of the vector representation is
    /// 4 (quaternion) while the dimension of the tangent space is 3.
    Index nq () const
    {
      return NQ;
    }
    /// Get dimension of Lie Group tangent space
    Index nv () const
    {
      return NV;
    }

    ConfigVector_t neutral () const
    {
      ConfigVector_t n; n.setZero (); n [2] = 1;
      return n;
    }

    std::string name () const
    {
      return std::string ("SE(2)");
    }

    template <class ConfigL_t, class ConfigR_t, class Tangent_t>
    static void difference_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                const Eigen::MatrixBase<ConfigR_t> & q1,
                                const Eigen::MatrixBase<Tangent_t> & d)
    {
      if (q0 == q1) {
        (const_cast <Eigen::MatrixBase<Tangent_t> &> (d)).setZero ();
        return;
      }
      Matrix2 R0, R1; Vector2 t0, t1;
      forwardKinematics(R0, t0, q0);
      forwardKinematics(R1, t1, q1);
      Matrix2 R (R0.transpose() * R1);
      Vector2 t (R0.transpose() * (t1 - t0));

      log (R, t, d);
    }

    template <class ConfigL_t, class ConfigR_t, class JacobianLOut_t, class JacobianROut_t>
    static void Jdifference_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                 const Eigen::MatrixBase<ConfigR_t> & q1,
                                 const Eigen::MatrixBase<JacobianLOut_t>& J0,
                                 const Eigen::MatrixBase<JacobianROut_t>& J1)
    {
      Matrix2 R0, R1; Vector2 t0, t1;
      forwardKinematics(R0, t0, q0);
      forwardKinematics(R1, t1, q1);
      Matrix2 R (R0.transpose() * R1);
      Vector2 t (R0.transpose() * (t1 - t0));

      Jlog (R, t, J1);

      // pcross = [ y1-y0, - (x1 - x0) ]
      Vector2 pcross (q1(1) - q0(1), q0(0) - q1(0));

      JacobianLOut_t& J0v = const_cast< JacobianLOut_t& > (J0.derived());
      J0v.template topLeftCorner <2,2> ().noalias() = - R.transpose();
      J0v.template topRightCorner<2,1> ().noalias() = R1.transpose() * pcross;
      J0v.template bottomLeftCorner <1,2> ().setZero();
      J0v (2,2) = -1;
      J0v.applyOnTheLeft(J1);
    }

    template <class ConfigIn_t, class Velocity_t, class ConfigOut_t>
    static void integrate_impl(const Eigen::MatrixBase<ConfigIn_t> & q,
                               const Eigen::MatrixBase<Velocity_t> & v,
                               const Eigen::MatrixBase<ConfigOut_t> & qout)
    {
      ConfigOut_t& out = const_cast< ConfigOut_t& >(qout.derived());

      Matrix2 R0, R;
      Vector2 t0, t;
      forwardKinematics(R0, t0, q);
      exp(v, R, t);

      out.template head<2>().noalias() = R0 * t + t0;
      out.template tail<2>().noalias() = R0 * R.col(0);
    }

    template <class Config_t, class Tangent_t, class JacobianOut_t>
    static void dIntegrate_dq_impl(const Eigen::MatrixBase<Config_t >  & /*q*/,
                                   const Eigen::MatrixBase<Tangent_t>  & v,
                                   const Eigen::MatrixBase<JacobianOut_t>& J)
    {
      JacobianOut_t& Jout = const_cast< JacobianOut_t& >(J.derived());

      Matrix2 R;
      Vector2 t;
      exp(v, R, t);

      toInverseActionMatrix (R, t, Jout);
    }

    template <class Config_t, class Tangent_t, class JacobianOut_t>
    static void dIntegrate_dv_impl(const Eigen::MatrixBase<Config_t >  & /*q*/,
                                   const Eigen::MatrixBase<Tangent_t>  & v,
                                   const Eigen::MatrixBase<JacobianOut_t>& J)
    {
      JacobianOut_t& Jout = const_cast< JacobianOut_t& >(J.derived());
      // TODO sparse version
      MotionTpl<Scalar,0> nu; nu.toVector() << v.template head<2>(), 0, 0, 0, v[2]; 
      Eigen::Matrix<Scalar,6,6> Jtmp6;
      Jexp6(nu, Jtmp6);
      Jout << Jtmp6.   topLeftCorner<2,2>(), Jtmp6.   topRightCorner<2,1>(),
              Jtmp6.bottomLeftCorner<1,2>(), Jtmp6.bottomRightCorner<1,1>();
    }

    // interpolate_impl use default implementation.
    // template <class ConfigL_t, class ConfigR_t, class ConfigOut_t>
    // static void interpolate_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                 // const Eigen::MatrixBase<ConfigR_t> & q1,
                                 // const Scalar& u,
                                 // const Eigen::MatrixBase<ConfigOut_t>& qout)

    // template <class ConfigL_t, class ConfigR_t>
    // static double squaredDistance_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                       // const Eigen::MatrixBase<ConfigR_t> & q1)
    template <class Config_t>
    static void normalize_impl (const Eigen::MatrixBase<Config_t>& qout)
    {
      Config_t& qout_ = (const_cast< Eigen::MatrixBase<Config_t>& >(qout)).derived();
      qout_.template tail<2>().normalize();
    }

    template <class Config_t>
    void random_impl (const Eigen::MatrixBase<Config_t>& qout) const
    {
      R2crossSO2_t().random(qout);
    }

    template <class ConfigL_t, class ConfigR_t, class ConfigOut_t>
    void randomConfiguration_impl(const Eigen::MatrixBase<ConfigL_t> & lower,
                                  const Eigen::MatrixBase<ConfigR_t> & upper,
                                  const Eigen::MatrixBase<ConfigOut_t> & qout)
      const
    {
      R2crossSO2_t ().randomConfiguration(lower, upper, qout);
    }

    template <class ConfigL_t, class ConfigR_t>
    static bool isSameConfiguration_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                         const Eigen::MatrixBase<ConfigR_t> & q1,
                                         const Scalar & prec)
    {
      return R2crossSO2_t().isSameConfiguration(q0, q1, prec);
    }

    private:
    template<typename V>
    static void forwardKinematics(Matrix2 & R, Vector2 & t, const Eigen::MatrixBase<V>& q)
    {
      EIGEN_STATIC_ASSERT_SAME_VECTOR_SIZE(ConfigVector_t,V);

      const double& c_theta = q(2),
                    s_theta = q(3);

      R << c_theta, -s_theta, s_theta, c_theta;
      t = q.template head<2>();
    }
  }; // struct SpecialEuclideanOperationTpl<2>

  template<> struct traits<SpecialEuclideanOperationTpl<3> > {
    typedef double Scalar;
    enum {
      NQ = 7,
      NV = 6
    };
  };
  
  /// SE(3)
  template<>
  struct SpecialEuclideanOperationTpl<3> : public LieGroupBase <SpecialEuclideanOperationTpl<3> >
  {
    typedef CartesianProductOperation <VectorSpaceOperationTpl<3>, SpecialOrthogonalOperationTpl<3> > R3crossSO3_t;

    SE3_LIE_GROUP_PUBLIC_INTERFACE(SpecialEuclideanOperationTpl);

    typedef Eigen::Quaternion<Scalar> Quaternion_t;
    typedef Eigen::Map<      Quaternion_t> QuaternionMap_t;
    typedef Eigen::Map<const Quaternion_t> ConstQuaternionMap_t;
    typedef SE3 Transformation_t;

    /// Get dimension of Lie Group vector representation
    ///
    /// For instance, for SO(3), the dimension of the vector representation is
    /// 4 (quaternion) while the dimension of the tangent space is 3.
    Index nq () const
    {
      return NQ;
    }
    /// Get dimension of Lie Group tangent space
    Index nv () const
    {
      return NV;
    }

    ConfigVector_t neutral () const
    {
      ConfigVector_t n; n.setZero (); n [6] = 1;
      return n;
    }

    std::string name () const
    {
      return std::string ("SE(3)");
    }

    template <class ConfigL_t, class ConfigR_t, class Tangent_t>
    static void difference_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                const Eigen::MatrixBase<ConfigR_t> & q1,
                                const Eigen::MatrixBase<Tangent_t> & d)
    {
      if (q0 == q1) {
        (const_cast < Eigen::MatrixBase<Tangent_t>& > (d)).setZero ();
        return;
      }
      ConstQuaternionMap_t p0 (q0.derived().template tail<4>().data());
      ConstQuaternionMap_t p1 (q1.derived().template tail<4>().data());
      const_cast < Eigen::MatrixBase<Tangent_t>& > (d)
        = log6(  SE3(p0.matrix(), q0.derived().template head<3>()).inverse()
               * SE3(p1.matrix(), q1.derived().template head<3>())).toVector();
    }

    template <class ConfigL_t, class ConfigR_t, class JacobianLOut_t, class JacobianROut_t>
    static void Jdifference_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                 const Eigen::MatrixBase<ConfigR_t> & q1,
                                 const Eigen::MatrixBase<JacobianLOut_t>& J0,
                                 const Eigen::MatrixBase<JacobianROut_t>& J1)
    {
      ConstQuaternionMap_t p0 (q0.derived().template tail<4>().data());
      ConstQuaternionMap_t p1 (q1.derived().template tail<4>().data());
      SE3::Matrix3 R0 (p0.matrix()),
                   R1 (p1.matrix());
      SE3 M (  SE3(R0, q0.derived().template head<3>()).inverse()
             * SE3(R1, q1.derived().template head<3>()));

      Jlog6 (M, J1);

      SE3::Vector3 p1_p0 (q1.derived().template head<3>()
                        - q0.derived().template head<3>());

      JacobianLOut_t& J0v = const_cast< JacobianLOut_t& > (J0.derived());
      J0v.template topLeftCorner <3,3> ().noalias() = - M.rotation().transpose();
      J0v.template topRightCorner<3,3> ().noalias() = R1.transpose() * skew (p1_p0) * R0;
      J0v.template bottomLeftCorner <3,3> ().setZero();
      J0v.template bottomRightCorner<3,3> ().noalias() = - M.rotation().transpose();
      J0v.applyOnTheLeft(J1);
    }

    template <class ConfigIn_t, class Velocity_t, class ConfigOut_t>
    static void integrate_impl(const Eigen::MatrixBase<ConfigIn_t> & q,
                               const Eigen::MatrixBase<Velocity_t> & v,
                               const Eigen::MatrixBase<ConfigOut_t> & qout)
    {
      ConfigOut_t& out = (const_cast< Eigen::MatrixBase<ConfigOut_t>& >(qout)).derived();
      ConstQuaternionMap_t quat(q.derived().template tail<4>().data());
      QuaternionMap_t res_quat (out.template tail<4>().data());

      SE3 M0 (quat.matrix(), q.derived().template head<3>());
      SE3 M1 (M0 * exp6(Motion(v)));

      out.template head<3>() = M1.translation();
      res_quat = M1.rotation();
      // Norm of qs might be epsilon-different to 1, so M1.rotation might be epsilon-different to a rotation matrix.
      // It is then safer to re-normalized after converting M1.rotation to quaternion.
      firstOrderNormalize(res_quat);
    }

    template <class Config_t, class Tangent_t, class JacobianOut_t>
    static void dIntegrate_dq_impl(const Eigen::MatrixBase<Config_t >  & /*q*/,
                                   const Eigen::MatrixBase<Tangent_t>  & v,
                                   const Eigen::MatrixBase<JacobianOut_t>& J)
    {
      JacobianOut_t& Jout = const_cast< JacobianOut_t& >(J.derived());
      Jout = exp6(v).inverse().toActionMatrix();
    }

    template <class Config_t, class Tangent_t, class JacobianOut_t>
    static void dIntegrate_dv_impl(const Eigen::MatrixBase<Config_t >  & /*q*/,
                                   const Eigen::MatrixBase<Tangent_t>  & v,
                                   const Eigen::MatrixBase<JacobianOut_t>& J)
    {
      Jexp6(MotionRef<Tangent_t>(v), J.derived());
    }

    // interpolate_impl use default implementation.
    // template <class ConfigL_t, class ConfigR_t, class ConfigOut_t>
    // static void interpolate_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                 // const Eigen::MatrixBase<ConfigR_t> & q1,
                                 // const Scalar& u,
                                 // const Eigen::MatrixBase<ConfigOut_t>& qout)
    // {
    // }

    template <class ConfigL_t, class ConfigR_t>
    static double squaredDistance_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                       const Eigen::MatrixBase<ConfigR_t> & q1)
    {
      TangentVector_t t;
      difference_impl(q0, q1, t);
      return t.squaredNorm();
    }
    
    template <class Config_t>
    static void normalize_impl (const Eigen::MatrixBase<Config_t>& qout)
    {
      Config_t& qout_ = (const_cast< Eigen::MatrixBase<Config_t>& >(qout)).derived();
      qout_.template tail<4>().normalize();
    }

    template <class Config_t>
    void random_impl (const Eigen::MatrixBase<Config_t>& qout) const
    {
      R3crossSO3_t().random(qout);
    }

    template <class ConfigL_t, class ConfigR_t, class ConfigOut_t>
    void randomConfiguration_impl(const Eigen::MatrixBase<ConfigL_t> & lower,
                                  const Eigen::MatrixBase<ConfigR_t> & upper,
                                  const Eigen::MatrixBase<ConfigOut_t> & qout)
      const
    {
      R3crossSO3_t ().randomConfiguration(lower, upper, qout);
    }

    template <class ConfigL_t, class ConfigR_t>
    static bool isSameConfiguration_impl(const Eigen::MatrixBase<ConfigL_t> & q0,
                                         const Eigen::MatrixBase<ConfigR_t> & q1,
                                         const Scalar & prec)
    {
      return R3crossSO3_t().isSameConfiguration(q0, q1, prec);
    }
  }; // struct SpecialEuclideanOperationTpl<3>

} // namespace se3

#endif // ifndef __se3_special_euclidean_operation_hpp__
