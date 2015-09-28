/**
 * @author Ivan Gagis <igagis@gmail.com>
 */

#pragma once


#ifdef DEBUG
#	include <iostream>
#endif

#include <utki/debug.hpp>

#include "../config.hpp"


namespace morda{


//forward declarations
template <class T> class Vector2;
template <class T> class Matrix4;
template <class T> class Quaternion;



/**
 * @brief Three-dimensional vector.
 */
template <class T> class Vector3{
public:

	/**
	 * @brief First vector component.
	 */
	T x;
	
	/**
	 * @brief Second vector component.
	 */
	T y;
	
	/**
	 * @brief Third vector component.
     */
	T z;

	/**
	 * @brief Get number of vector components.
     * @return Number of vector components.
     */
	size_t size()const{
		return 3;
	}
	
	/**
	 * @brief Default constructor.
	 * Default constructor does not initialize vector components to any values.
	 */
	Vector3()noexcept{}//default constructor

	/**
	 * @brief Constructor.
	 * Initializes vector components to given values.
     * @param x - value for first vector component.
     * @param y - value for second vector component.
     * @param z - value for third vector component.
     */
	Vector3(T x, T y, T z)noexcept :
			x(x),
			y(y),
			z(z)
	{}

	//copy constructor will be generated by compiler

	/**
	 * @brief Constructor.
	 * Initializes all vector components to a given value.
     * @param num - value to initialize all vector components with.
     */
	Vector3(T num)noexcept :
			x(num),
			y(num),
			z(num)
	{}

	/**
	 * @brief Constructor.
	 * Initializes components to a given values.
	 * @param vec - 2d vector to use for initialization of first two vector components.
	 * @param z - value to use for initialization of 3rd vector component.
	 */
	Vector3(const Vector2<T>& vec, T z = 0)noexcept;

	
	
	template <class TT> explicit Vector3(const Vector3<TT>& v) :
			x(v.x),
			y(v.y),
			z(v.z)
	{}
	
	
	
	/**
	 * @brief Access vector component.
     * @param i - component index to access, must be from 0 to 2.
     * @return Reference to the requested vector component.
     */
	T& operator[](unsigned i)noexcept{
		ASSERT(i < 3)
		ASSERT( &((&this->x)[0]) == &this->x)
		ASSERT( &((&this->x)[1]) == &this->y)
		ASSERT( &((&this->x)[2]) == &this->z)
		return (&this->x)[i];
	}

	/**
	 * @brief Access vector component.
	 * Constant version of operator[].
     * @param i - component index to access, must be from 0 to 2.
     * @return constant reference to the requested vector component.
     */
	const T& operator[](unsigned i)const noexcept{
		ASSERT(i < 3)
		ASSERT( &((&this->x)[0]) == &this->x)
		ASSERT( &((&this->x)[1]) == &this->y)
		ASSERT( &((&this->x)[2]) == &this->z)
		return (&this->x)[i];
	}

	//NOTE: operator=() will be generated by compiler.

	/**
	 * @brief Assign from 2d vector.
	 * Assigns first 2 components of this vector from components of given 2d vector.
	 * Third component of this vector is assigned a value of 0.
	 * @param vec - 2d vector to assign first two components from.
	 * @return Reference to this vector object.
	 */
	Vector3& operator=(const Vector2<T>& vec)noexcept;

	/**
	 * @brief Assign a number.
	 * Sets all 3 components of this vector to a given number.
     * @param num - number to use for assignment.
     * @return Reference to this vector object.
     */
	Vector3& operator=(T num)noexcept{
		this->x = num;
		this->y = num;
		this->z = num;
		return (*this);
	}
	
	/**
	 * @brief Set all vector components to given value.
	 * @param val - value to set vector components to.
	 * @return Reference to this vector object.
	 */
	Vector3& SetTo(T val)noexcept{
		this->x = val;
		this->y = val;
		this->z = val;
		return (*this);
	}

	/**
	 * @brief Add and assign.
	 * Adds corresponding components of a given vector to first two components of this vector and assigns
	 * the result back to this vector components.
	 * @param vec - 2d vector to use for addition.
	 * @return Reference to this vector object.
	 */
	Vector3& operator+=(const Vector2<T>& vec)noexcept;

	/**
	 * @brief Add and assign.
	 * Adds given vector to this vector and assigns result back to this vector.
	 * @param vec - vector to add.
	 * @return Reference to this vector object.
	 */
	Vector3& operator+=(const Vector3& vec)noexcept{
		this->x += vec.x;
		this->y += vec.y;
		this->z += vec.z;
		return (*this);
	}

	/**
	 * @brief Add vector.
	 * Adds this vector and given vector.
	 * @param vec - vector to add.
	 * @return Vector resulting from vector addition.
	 */
	Vector3 operator+(const Vector3& vec)const noexcept{
		return (Vector3(*this) += vec);
	}

	/**
	 * @brief Subtract and assign.
	 * Subtracts given vector from this vector and assigns result back to this vector.
	 * @param vec - vector to subtract.
	 * @return Reference to this vector object.
	 */
	Vector3& operator-=(const Vector3& vec)noexcept{
		this->x -= vec.x;
		this->y -= vec.y;
		this->z -= vec.z;
		return *this;
	}

	/**
	 * @brief Subtract vector.
	 * Subtracts given vector from this vector.
	 * @param vec - vector to subtract.
	 * @return Vector resulting from vector subtraction.
	 */
	Vector3 operator-(const Vector3& vec)const noexcept{
		return (Vector3(*this) -= vec);
	}

	/**
	 * @brief Unary minus.
     * @return Negated vector.
     */
	Vector3 operator-()const noexcept{
		return Vector3(*this).Negate();
	}

	/**
	 * @brief Multiply by scalar and assign.
	 * Multiplies this vector by scalar and assigns result back to this vector.
     * @param num - scalar to multiply by.
     * @return Reference to this vector object.
     */
	Vector3& operator*=(T num)noexcept{
		this->x *= num;
		this->y *= num;
		this->z *= num;
		return (*this);
	}

	/**
	 * @brief Multiply by scalar.
	 * Multiplies this vector by scalar.
     * @param num - scalar to multiply by.
     * @return Vector resulting from multiplication of this vector by scalar.
     */
	Vector3 operator*(T num)const noexcept{
		return (Vector3(*this) *= num);
	}

	/**
	 * @brief Multiply scalar by vector.
	 * @param num - scalar to multiply.
	 * @param vec - vector to multiply by.
	 * @return Vector resulting from multiplication of given scalar by given vector.
	 */
	friend Vector3 operator*(T num, const Vector3& vec)noexcept{
		return vec * num;
	}

	/**
	 * @brief Divide by scalar and assign.
	 * Divide this vector by scalar and assign result back to this vector.
	 * @param num - scalar to divide by.
	 * @return Reference to this vector object.
	 */
	Vector3& operator/=(T num)noexcept{
		ASSERT(num != 0)
		this->x /= num;
		this->y /= num;
		this->z /= num;
		return (*this);
	}

	/**
	 * @brief Divide by scalar.
	 * Divide this vector by scalar.
	 * @param num - scalar to divide by.
	 * @return Vector resulting from division of this vector by scalars.
	 */
	Vector3 operator/(T num)noexcept{
		ASSERT_INFO(num != 0, "Vector3::operator/(): division by 0")
		return (Vector3(*this) /= num);
	}

	/**
	 * @brief Dot product.
     * @param vec -vector to multiply by.
     * @return Dot product of this vector and given vector.
     */
	T operator*(const Vector3& vec)const noexcept{
		return this->x * vec.x
				+ this->y * vec.y
				+ this->z * vec.z;
	}

	/**
	 * @brief Component-wise multiplication.
	 * Performs component-wise multiplication of two vectors.
	 * The result of such operation is also a vector.
     * @param vec - vector to multiply by.
     * @return Vector resulting from component-wise multiplication.
     */
	Vector3 CompMul(const Vector3& vec)const noexcept{
		return Vector3(
				this->x * vec.x,
				this->y * vec.y,
				this->z * vec.z
			);
	}

	/**
	 * @brief Cross product.
     * @param vec - vector to multiply by.
     * @return Vector resulting from the cross product.
     */
	Vector3 operator%(const Vector3& vec)const noexcept{
		return Vector3(
				this->y * vec.z - this->z * vec.y,
				this->z * vec.x - this->x * vec.z,
				this->x * vec.y - this->y * vec.x
			);
	}

	/**
	 * @brief Check if all components of this vector are zero.
     * @return true if all components of this vector are zero.
	 * @return false otherwise.
     */
	bool IsZero()const noexcept{
		return (this->x == 0 && this->y == 0 && this->z == 0);
	}

	/**
	 * @brief Negate this vector.
	 * Negates this vector.
	 * @return Reference to this vector object.
	 */
	Vector3& Negate()noexcept{
		this->x = -this->x;
		this->y = -this->y;
		this->z = -this->z;
		return (*this);
	}

	/**
	 * @brief Calculate power 2 of vector magnitude.
	 * @return Power 2 of this vector magnitude.
	 */
	T MagPow2()const noexcept{
		return utki::pow2(this->x) + utki::pow2(this->y) + utki::pow2(this->z);
	}

	/**
	 * @brief Calculate vector magnitude.
	 * @return Vector magnitude.
	 */
	T Magnitude()const noexcept{
		return ::sqrt(this->MagPow2());
	}

	/**
	 * @brief Normalize this vector.
	 * Normalizes this vector.
	 * If magnitude is 0 then the result is vector (1, 0, 0).
	 * @return Reference to this vector object.
	 */
	Vector3& Normalize()noexcept{
		T mag = this->Magnitude();
		if(mag == 0){
			this->x = 1;
			this->y = 0;
			this->z = 0;
			return *this;
		}
		
		return (*this) /= this->Magnitude();
	}

	/**
	 * @brief Project this vector onto a given vector.
     * @param vec - vector to project onto, it does not have to be normalized.
     * @return Reference to this vector object.
     */
	Vector3& ProjectOnto(const Vector3& vec)noexcept{
		ASSERT(this->MagPow2() != 0)
		(*this) = vec * (vec * (*this)) / vec.MagPow2();
		return (*this);
	}

	/**
	 * @brief Rotate this vector.
	 * Rotate this vector with unit quaternion.
	 * @param q - quaternion which defines the rotation.
	 * @return Reference to this vector object.
	 */
	Vector3<T>& Rotate(const Quaternion<T>& q)noexcept;



#ifdef DEBUG  
	friend std::ostream& operator<<(std::ostream& s, const Vector3<T>& vec){
		s << "(" << vec.x << ", " << vec.y << ", " << vec.z << ")";
		return s;
	}
#endif
};//~class Vector3



}//~namespace



#include "Vector2.hpp"
#include "Quaternion.hpp"
#include "Matrix4.hpp"



namespace morda{

//==========================
// functions implementation
//==========================

template <class T> Vector3<T>::Vector3(const Vector2<T>& vec, T z)noexcept :
		x(vec.x),
		y(vec.y),
		z(z)
{}



template <class T> Vector3<T>& Vector3<T>::operator=(const Vector2<T>& vec)noexcept{
	this->x = vec.x;
	this->y = vec.y;
	this->z = 0;
	return (*this);
}



template <class T> Vector3<T>& Vector3<T>::operator+=(const Vector2<T>& vec)noexcept{
	this->x += vec.x;
	this->y += vec.y;
	return (*this);
}



template <class T> Vector3<T>& Vector3<T>::Rotate(const Quaternion<T>& q)noexcept{
	*this = q.ToMatrix4() * (*this);
	return *this;
}



//=====================
// Convenient typedefs
//=====================

typedef Vector3<float> Vec3f;
static_assert(sizeof(Vec3f) == sizeof(float) * 3, "size mismatch");

typedef Vector3<double> Vec3d;
static_assert(sizeof(Vec3d) == sizeof(double) * 3, "size mismatch");

typedef Vector3<real> Vec3r;
static_assert(sizeof(Vec3r) == sizeof(real) * 3, "size mismatch");

}//~namespace
