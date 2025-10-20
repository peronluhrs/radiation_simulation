#pragma once

// Implémentation simplifiée de GLM pour la démonstration
#ifndef USE_BASIC_MATH

// Si GLM est disponible, l'utiliser
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/norm.hpp>

#else

// Implémentation de base des vecteurs et matrices
#include <cmath>
#include <algorithm>

namespace glm {
    struct vec3 {
        float x, y, z;
        
        vec3() : x(0), y(0), z(0) {}
        vec3(float v) : x(v), y(v), z(v) {}
        vec3(float x_, float y_, float z_) : x(x_), y(y_), z(z_) {}
        
        vec3 operator+(const vec3& v) const { return vec3(x + v.x, y + v.y, z + v.z); }
        vec3 operator-(const vec3& v) const { return vec3(x - v.x, y - v.y, z - v.z); }
        vec3 operator*(float s) const { return vec3(x * s, y * s, z * s); }
    friend vec3 operator*(float s, const vec3& v) { return vec3(s * v.x, s * v.y, s * v.z); }
        vec3 operator/(float s) const { return vec3(x / s, y / s, z / s); }
    friend vec3 operator/(float s, const vec3& v) { return vec3(s / v.x, s / v.y, s / v.z); }
        vec3 operator-() const { return vec3(-x, -y, -z); }
        
        vec3& operator+=(const vec3& v) { x += v.x; y += v.y; z += v.z; return *this; }
        vec3& operator-=(const vec3& v) { x -= v.x; y -= v.y; z -= v.z; return *this; }
        vec3& operator*=(float s) { x *= s; y *= s; z *= s; return *this; }
        
        float& operator[](int i) { return (&x)[i]; }
        const float& operator[](int i) const { return (&x)[i]; }
    };
    
    struct vec2 {
        float x, y;
        vec2() : x(0), y(0) {}
        vec2(float v) : x(v), y(v) {}
        vec2(float x_, float y_) : x(x_), y(y_) {}
    };
    
    struct vec4 {
        float x, y, z, w;
        vec4() : x(0), y(0), z(0), w(0) {}
        vec4(float x_, float y_, float z_, float w_) : x(x_), y(y_), z(z_), w(w_) {}
        vec4(const vec3& v, float w_) : x(v.x), y(v.y), z(v.z), w(w_) {}
        operator vec3() const { return vec3(x, y, z); }
    };
    
    struct quat {
        float w, x, y, z;
        quat() : w(1), x(0), y(0), z(0) {}
        quat(float w_, float x_, float y_, float z_) : w(w_), x(x_), y(y_), z(z_) {}
    };
    
    struct mat4 {
        float m[16];
        
        mat4() {
            for (int i = 0; i < 16; ++i) m[i] = 0;
            m[0] = m[5] = m[10] = m[15] = 1; // Identité
        }
        
        mat4(float f) {
            for (int i = 0; i < 16; ++i) m[i] = 0;
            m[0] = m[5] = m[10] = m[15] = f; // Matrice identité * facteur
        }
        
        vec4 operator[](int i) const {
            return vec4(m[i*4], m[i*4+1], m[i*4+2], m[i*4+3]);
        }
        
        mat4 operator*(const mat4& other) const {
            mat4 result;
            for (int i = 0; i < 4; ++i) {
                for (int j = 0; j < 4; ++j) {
                    result.m[i*4+j] = 0;
                    for (int k = 0; k < 4; ++k) {
                        result.m[i*4+j] += m[i*4+k] * other.m[k*4+j];
                    }
                }
            }
            return result;
        }
        
        vec4 operator*(const vec4& v) const {
            return vec4(
                m[0]*v.x + m[1]*v.y + m[2]*v.z + m[3]*v.w,
                m[4]*v.x + m[5]*v.y + m[6]*v.z + m[7]*v.w,
                m[8]*v.x + m[9]*v.y + m[10]*v.z + m[11]*v.w,
                m[12]*v.x + m[13]*v.y + m[14]*v.z + m[15]*v.w
            );
        }
    };
    
    // Fonctions utilitaires
    inline float dot(const vec3& a, const vec3& b) {
        return a.x * b.x + a.y * b.y + a.z * b.z;
    }
    
    inline vec3 cross(const vec3& a, const vec3& b) {
        return vec3(
            a.y * b.z - a.z * b.y,
            a.z * b.x - a.x * b.z,
            a.x * b.y - a.y * b.x
        );
    }
    
    inline float length(const vec3& v) {
        return std::sqrt(v.x * v.x + v.y * v.y + v.z * v.z);
    }
    
    inline vec3 normalize(const vec3& v) {
        float len = length(v);
        return len > 0 ? v / len : vec3(0, 0, 0);
    }
    
    inline vec3 min(const vec3& a, const vec3& b) {
        return vec3(std::min(a.x, b.x), std::min(a.y, b.y), std::min(a.z, b.z));
    }
    
    inline vec3 max(const vec3& a, const vec3& b) {
        return vec3(std::max(a.x, b.x), std::max(a.y, b.y), std::max(a.z, b.z));
    }
    
    inline vec3 abs(const vec3& v) {
        return vec3(std::abs(v.x), std::abs(v.y), std::abs(v.z));
    }
    
    inline float clamp(float x, float minVal, float maxVal) {
        return std::min(std::max(x, minVal), maxVal);
    }
    
    inline mat4 translate(const mat4& m, const vec3& v) {
        mat4 result = m;
        result.m[3] += v.x;
        result.m[7] += v.y;
        result.m[11] += v.z;
        return result;
    }
    
    inline mat4 scale(const mat4& m, const vec3& v) {
        mat4 result = m;
        result.m[0] *= v.x;
        result.m[5] *= v.y;
        result.m[10] *= v.z;
        return result;
    }
    
    inline mat4 transpose(const mat4& m) {
        mat4 result;
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                result.m[i*4+j] = m.m[j*4+i];
            }
        }
        return result;
    }
    
    // Structure mat3 simple
    struct mat3 {
        float m[9];
        
        mat3() {
            for (int i = 0; i < 9; ++i) m[i] = 0;
            m[0] = m[4] = m[8] = 1;
        }
        
        vec3 operator[](int i) const {
            return vec3(m[i*3], m[i*3+1], m[i*3+2]);
        }
    };
    
    inline quat conjugate(const quat& q) {
        return quat(q.w, -q.x, -q.y, -q.z);
    }
    
    inline mat4 mat4_cast(const quat& q) {
        mat4 result;
        // Conversion quaternion vers matrice (simplifiée)
        result.m[0] = 1 - 2*q.y*q.y - 2*q.z*q.z;
        result.m[1] = 2*q.x*q.y - 2*q.w*q.z;
        result.m[2] = 2*q.x*q.z + 2*q.w*q.y;
        
        result.m[4] = 2*q.x*q.y + 2*q.w*q.z;
        result.m[5] = 1 - 2*q.x*q.x - 2*q.z*q.z;
        result.m[6] = 2*q.y*q.z - 2*q.w*q.x;
        
        result.m[8] = 2*q.x*q.z - 2*q.w*q.y;
        result.m[9] = 2*q.y*q.z + 2*q.w*q.x;
        result.m[10] = 1 - 2*q.x*q.x - 2*q.y*q.y;
        
        return result;
    }
    
    inline quat quat_cast(const mat4& m) {
        // Conversion matrice vers quaternion (simplifiée)
        float trace = m.m[0] + m.m[5] + m.m[10];
        quat q;
        
        if (trace > 0) {
            float s = std::sqrt(trace + 1.0f) * 2;
            q.w = 0.25f * s;
            q.x = (m.m[9] - m.m[6]) / s;
            q.y = (m.m[2] - m.m[8]) / s;
            q.z = (m.m[4] - m.m[1]) / s;
        } else {
            // Cas plus complexes omis pour la simplicité
            q = quat(); // Identité
        }
        
        return q;
    }
}

#endif // USE_BASIC_MATH