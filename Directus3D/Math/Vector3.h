/*
Copyright(c) 2016 Panos Karabelas

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and / or sell
copies of the Software, and to permit persons to whom the Software is furnished
to do so, subject to the following conditions :

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.IN NO EVENT SHALL THE AUTHORS OR
COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#pragma once

//= INCLUDES ==========
#include <string>
#include "MathHelper.h"
//=====================

namespace Directus
{
	namespace Math
	{
		class Matrix;
		class Quaternion;

		class __declspec(dllexport) Vector3
		{
		public:
			// Constructor
			Vector3()
			{
				x = 0;
				y = 0;
				z = 0;
			}

			// Copy-constructor
			Vector3(const Vector3& vector)
			{
				x = vector.x;
				y = vector.y;
				z = vector.z;
			}

			// Construct from coordinates.
			Vector3(float x, float y, float z)
			{
				this->x = x;
				this->y = y;
				this->z = z;
			}

			// Construct from single value.
			Vector3(float v)
			{
				this->x = v;
				this->y = v;
				this->z = v;
			}

			Vector3 DegreesToRadians(const Vector3& v) const { return DegreesToRadians(v.x, v.y, v.z); }

			static Vector3 DegreesToRadians(float x, float y, float z)
			{
				x *= DEG_TO_RAD;
				y *= DEG_TO_RAD;
				z *= DEG_TO_RAD;

				return Vector3(x, y, z);
			}

			static Vector3 RadiansToDegrees(const Vector3& v) { return RadiansToDegrees(v.x, v.y, v.z); }

			static Vector3 RadiansToDegrees(float x, float y, float z)
			{
				x *= RAD_TO_DEG;
				y *= RAD_TO_DEG;
				z *= RAD_TO_DEG;

				return Vector3(x, y, z);
			}

			void Floor()
			{
				x = floorf(x);
				y = floorf(y);
				z = floorf(z);
			}

			//= NORMALIZATION ========================================================================================
			Vector3 Normalized() const
			{
				float factor = Length();
				factor = 1.0f / factor;
				return Vector3(x * factor, y * factor, z * factor);
			}

			void Normalize()
			{
				float factor = Length();
				factor = 1.0f / factor;
				x *= factor;
				y *= factor;
				z *= factor;
			};
			static Vector3 Normalize(const Vector3& v) { return v.Normalized(); }
			//========================================================================================================

			//= DOT PRODUCT ==========================================================================================
			static float Dot(const Vector3& v1, const Vector3& v2) { return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z; }
			float Dot(const Vector3& rhs) const { return x * rhs.x + y * rhs.y + z * rhs.z; }
			//========================================================================================================

			//= CROSS PRODUCT ========================================================================================
			static Vector3 Cross(const Vector3& v1, const Vector3& v2)
			{
				float x = v1.y * v2.z - v2.y * v1.z;
				float y = -(v1.x * v2.z - v2.x * v1.z);
				float z = v1.x * v2.y - v2.x * v1.y;

				return Vector3(x, y, z);
			}
			Vector3 Cross(const Vector3& v2) const { return Cross(*this, v2); }
			//========================================================================================================

			//= LENGTH ===============================================================================================
			static float LengthSquared(const Vector3& v1, const Vector3& v2)
			{
				return (v1.x - v2.x) * (v1.x - v2.x) + (v1.y - v2.y) * (v1.y - v2.y) + (v1.z - v2.z) * (v1.z - v2.z);
			}

			float Length() const { return sqrtf(x * x + y * y + z * z); }
			float LengthSquared() const { return x * x + y * y + z * z; }
			//========================================================================================================

			static Vector3 Transform(const Vector3& vector, const Matrix& matrix);
			Vector3 Absolute() const { return Vector3(abs(x), abs(y), abs(z)); }
			float Volume() const { return x * y * z; }
			std::string ToString() const;

			//= MULTIPLICATION =============================================================
			Vector3 operator*(const Vector3& b) const
			{
				return Vector3(
					x * b.x,
					y * b.y,
					z * b.z
				);
			}

			Vector3 operator*(const Matrix& matrix) const
			{
				return Transform(*this, matrix);
			}

			void operator*=(const Vector3& b)
			{
				x *= b.x;
				y *= b.y;
				z *= b.z;
			}

			Vector3 operator*(const float value) const
			{
				return Vector3(
					x * value,
					y * value,
					z * value
				);
			}

			void operator*=(const float value)
			{
				x *= value;
				y *= value;
				z *= value;
			}

			void operator*=(const Quaternion& q);
			//==============================================================================

			//= ADDITION ===================================================================
			Vector3 operator+(const Vector3& b) const
			{
				return Vector3(
					this->x + b.x,
					this->y + b.y,
					this->z + b.z
				);
			}

			Vector3 operator+(const float value) const
			{
				return Vector3(
					this->x + value,
					this->y + value,
					this->z + value
				);
			}

			void operator+=(const Vector3& b)
			{
				this->x += b.x;
				this->y += b.y;
				this->z += b.z;
			}

			void operator+=(const float value)
			{
				this->x += value;
				this->y += value;
				this->z += value;
			}
			//==============================================================================

			//= SUBTRACTION =============================================================
			Vector3 operator-(const Vector3& b) const
			{
				return Vector3(
					this->x - b.x,
					this->y - b.y,
					this->z - b.z
				);
			}

			Vector3 operator-(const float value)
			{
				return Vector3(
					this->x - value,
					this->y - value,
					this->z - value
				);
			}

			void operator-=(const Vector3& b)
			{
				this->x -= b.x;
				this->y -= b.y;
				this->z -= b.z;
			}
			//==============================================================================

			//= DIVISION =============================================================
			Vector3 operator/(const Vector3& b) const
			{
				return Vector3(
					this->x / b.x,
					this->y / b.y,
					this->z / b.z
				);
			}

			Vector3 operator/(const float value)
			{
				return Vector3(
					this->x / value,
					this->y / value,
					this->z / value
				);
			}

			void operator/=(const Vector3& b)
			{
				this->x /= b.x;
				this->y /= b.y;
				this->z /= b.z;
			}
			//==============================================================================

			//= COMPARISON =============================================================
			bool operator==(const Vector3& b)
			{
				if (x == b.x && y == b.y && z == b.z)
					return true;

				return false;
			}

			bool operator==(const Vector3& b) const
			{
				if (x == b.x && y == b.y && z == b.z)
					return true;

				return false;
			}

			bool operator!=(const Vector3& b)
			{
				if (x != b.x || y != b.y || z != b.z)
					return true;

				return false;
			}

			bool operator!=(const Vector3& b) const
			{
				if (x != b.x || y != b.y || z != b.z)
					return true;

				return false;
			}
			//==============================================================================

			float x;
			float y;
			float z;

			static const Vector3 Zero;
			static const Vector3 Left;
			static const Vector3 Right;
			static const Vector3 Up;
			static const Vector3 Down;
			static const Vector3 Forward;
			static const Vector3 Back;
			static const Vector3 One;
			static const Vector3 Infinity;
			static const Vector3 InfinityNeg;
		};

		inline __declspec(dllexport) Vector3 operator*(float lhs, const Vector3& rhs) { return rhs * lhs; }
		inline __declspec(dllexport) Vector3 operator*(const Matrix& lhs, const Vector3& rhs) { return rhs * lhs; }
	}
}
