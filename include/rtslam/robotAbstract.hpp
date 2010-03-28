/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * Copyright RIA-LAAS/CNRS, 2010
 *+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 *
 * File:      robotAbstract.h
 * Project:   RT-Slam
 * Author:    Joan Sola
 *
 * Version control
 * ===============
 *
 *  $Id$
 *
 * Description
 * ============
 *
 *
 * ++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

/**
 * \file robotAbstract.hpp
 * \author: jsola@laas.fr
 *
 * File defining the abstract robot class
 *
 * \ingroup rtslam
 */

#ifndef __RobotAbstract_H__
#define __RobotAbstract_H__

/* --------------------------------------------------------------------- */
/* --- INCLUDE --------------------------------------------------------- */
/* --------------------------------------------------------------------- */

#include <jmath/jblas.hpp>

#include "rtslam/rtSlam.hpp"
#include "rtslam/gaussian.hpp"
#include "rtslam/mapObject.hpp"
// include parents
#include "rtslam/mapAbstract.hpp"
#include "rtslam/mapObject.hpp"

namespace jafar {
	namespace rtslam {
		using namespace std;


		//  Forward declarations of children
		class SensorAbstract;


		/** Base class for all Gaussian control vectors defined in the module rtslam.
		 * \author jsola@laas.fr
		 *
		 * The Control class is mainly a Gaussian with a time interval value. It represents discrete-time control vectors.
		 * Mean and covariances are interpreted as follows:
		 * - The mean is considered the deterministic part of the control.
		 * - The covariances matrix encodes the random character of the perturbation.
		 *
		 * In case the control and perturbation values want to be specified in continuous-time,
		 * this class incorporates private members for storing the continuous values,
		 * and also methods for the conversion to discrete-time.
		 *
		 * @ingroup rtslam
		 */
		class Control: public Gaussian {
			private:
				vec x_ct; ///< continuous-time control vector
				sym_mat P_ct; ///< continuous-time covariances matrix

			public:
				double dt;
				Control(const size_t _size) :
					Gaussian(_size) {
					dt = 1.0;
				}
				Control(const Gaussian & c) :
					Gaussian(c) {
					dt = 1.0;
				}
				Control(const Gaussian & c, const double _dt) :
					Gaussian(c), dt(_dt) {
				}
				template<class SymMat>
				void set_P_continuous(SymMat & _P_ct) {
					JFR_ASSERT(_P_ct.size1() == size(), "Matrix sizes mismatch.");
					P_ct.resize(size(), size());
					P_ct = _P_ct;
				}
				template<class V>
				void set_x_continuous(V & _x_ct) {
					JFR_ASSERT(_x_ct.size() == size(), "Vector sizes mismatch.");
					x_ct.resize(size());
					x_ct = _x_ct;
				}
				/**
				 * Discrete perturbation from continuous specification.
				 * - The white, Gaussian random values integrate with the square root of dt. Their variance integrates linearly with dt:
				 *		- P = _P_ct * _dt
				 *
				 * This function takes covariances from the internal variables of the class (which is often constant).
				 * \param _dt the time interval to integrate.
				 */
				void convert_P_from_continuous(double _dt) {
					JFR_ASSERT(P_ct.size1() == size(), "Continuous-time covariance not yet initialized.");
					P(P_ct * _dt); // perturbation is random => variance is linear with time
				}
				/**
				 * Discrete perturbation from continuous specification.
				 * - The white, Gaussian random values integrate with the square root of dt. Their variance integrates linearly with dt:
				 *		- P = _P_ct * _dt
				 *
				 * \param _P_ct continuous-time perturbation covariances matrix.
				 * \param _dt the time interval to integrate.
				 */
				void convert_P_from_continuous(sym_mat & _P_ct, double _dt) {
					JFR_ASSERT(_P_ct.size1() == size(), "Matrix sizes mismatch.");
					set_P_continuous(_P_ct);
					P(P_ct * _dt); // perturbation is random => variance is linear with time
				}
				/**
				 * Discrete control and perturbation from continuous specifications.
				 * - The deterministic values integrate with time normally, linearly with dt:
				 * 		- x = _x_ct * _dt
				 * - The white, Gaussian random values integrate with the square root of dt. Their variance integrates linearly with dt:
				 *		- P = _P_ct * _dt
				 *
				 * This function takes mean and covariances from the internal variables of the class (which are often constant).
				 * \param _dt the time interval to integrate.
				 */
				void convert_from_continuous(double _dt) {
					JFR_ASSERT(x_ct.size() == size(), "Continuous-time values not yet initialized.");
					x(x_ct * _dt); // control is deterministic => mean is linear with time
					P(P_ct * _dt); // perturbation is random => variance is linear with time
					dt = _dt;
				}
				/**
				 * Discrete control and perturbation from continuous specifications.
				 * - The deterministic values integrate with time normally, linearly with dt:
				 * 		- x = Ct.x * _dt
				 * - The white, Gaussian random values integrate with the square root of dt. Their variance integrates linearly with dt:
				 *		- P = Ct.P * _dt
				 *
				 * \param Ct a continuous-time Gaussian holding deterministic mean and the covariances matrix of the random part.
				 * \param _dt the time interval to integrate.
				 */
				void convert_from_continuous(Gaussian & Ct, double _dt) {
					JFR_ASSERT(Ct.size() == size(), "Sizes mismatch");
					set_P_continuous(Ct.P());
					set_x_continuous(Ct.x());
					convert_from_continuous(_dt);
				}
		};


		/** Base class for all robots defined in the module
		 * rtslam.
		 *
		 * \ingroup rtslam
		 */
		class RobotAbstract: public MapObject {

				friend ostream& operator <<(ostream & s, jafar::rtslam::RobotAbstract & rob);

			public:


				/**
				 * Remote constructor from remote map and size of state and control vectors.
				 * \param _map the map.
				 * \param _size_state the size of the robot state vector
				 * \param _size_control the size of the control vector
				 */
				RobotAbstract(MapAbstract & _map, const size_t _size_state, const size_t _size_control);

				// Mandatory virtual destructor.
				virtual ~RobotAbstract() {
				}

				/**
				 * Constant perturbation flag.
				 * Flag for indicating that the state perturbation Q is constant and should not be computed at each iteration.
				 *
				 * In case this flag wants to be set to \c true , the user must consider computing the constant \a Q immediately after construction time.
				 * This can be done in two ways:
				 * - use a function member \b setup(..args..) in the derived class to compute the Jacobian XNEW_control,
				 *   and enter the appropriate control.P() value.
				 * 	 Then call computeStatePerturbation().
				 * - use a function member \b setup(..args..) in the derived class to enter the matrix Q directly.
				 */
				bool constantPerturbation;

				map_ptr_t slamMap; ///< parent map
				sensors_ptr_set_t sensors; ///<	A set of sensors

				Gaussian pose; ///< Robot Gaussian pose
				Control control; ///< Control Gaussian vector

				jblas::mat XNEW_x; ///< Jacobian wrt state
				jblas::mat XNEW_control; ///< Jacobian wrt control
				jblas::sym_mat Q; ///< Perturbation matrix in state space, Q = XNEW_control * control.P * trans(XNEW_control);

				static size_t size_control() {
					return 0;
				}

				void linkToSensor(sensor_ptr_t _senPtr); ///< Link to sensor
				void linkToMap(map_ptr_t _mapPtr); ///<       Link to map

				/**
				 * Acquire control structure
				 */
				void set_control(const Control & _control) {
					control = _control;
				}

				/**
				 * Move one step ahead, affect SLAM filter.
				 * This function updates the full state and covariances matrix of the robot plus the cross-variances with all other map objects.
				 */
				virtual void move();

				/**
				 * Move one step ahead, affect SLAM filter.
				 * This function updates the full state and covariances matrix of the robot plus the cross-variances with all other map objects.
				 */
				inline void move(const Control & _control) {
					set_control(_control);
					move();
				}

				/**
				 * Move one step ahead, affect SLAM filter.
				 * This function updates the full state and covariances matrix of the robot plus the cross-variances with all other map objects.
				 */
				template<class V>
				inline void move(V & _u) {
					JFR_ASSERT(_u.size() == control.size(), "robotAbstract.hpp: move: wrong control size.");
					control.x(_u);
					move();
				}

				/**
				 * Compute robot process noise \a Q in state space.
				 * This function is called by move() at each iteration if constantPerturbation is \b false.
				 * It performs the operation:
				 * - Q = XNEW_control * control.P() * XNEW_control',
				 *
				 * where XNEW_control and control.P() must have been already computed.
				 */
				void computeStatePerturbation();

				/**
				 * Explore all sensors.
				 * This function iterates all the sensors in the robot and calls the main sensor operations.
				 */
				void exploreSensors();

			protected:

				/**
				 * Move one step ahead.
				 *
				 * Implement this function in every derived class.
				 *
				 * This function predicts the robot state one step of length \a _dt ahead in time,
				 * according to the current state _x, the control input \a _u and the time interval \a _dt.
				 * It computes the new state and the convenient Jacobian matrices.
				 *
				 * \param _x the current state vector
				 * \param _u the control vector
				 * \param _dt the time interval
				 * \param _xnew the new state
				 * \param _XNEW_x the Jacobian of \a _xnew wrt \a _x
				 * \param _XNEW_u the Jacobian of \a _xnew wrt \a _u
				 */
				virtual void move_func(const vec & _x, const vec & _u, const double _dt, vec & _xnew, mat & _XNEW_x, mat & _XNEW_u) = 0;

				/**
				 * Move one step ahead, use object members as data.
				 */
				inline void move_func() {
					vec x = state.x();
					vec u = control.x();
					move_func(x, u, control.dt, x, XNEW_x, XNEW_control);
				}

		};

	}
}

#endif // #ifndef __RobotAbstract_H__
/*
 * Local variables:
 * mode: c++
 * indent-tabs-mode: t
 * tab-width: 2
 * c-basic-offset: 2
 * End:
 */
