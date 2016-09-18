/*
# Copyright Disney Enterprises, Inc.  All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License
# and the following modification to it: Section 6 Trademarks.
# deleted and replaced with:
#
# 6. Trademarks. This License does not grant permission to use the
# trade names, trademarks, service marks, or product names of the
# Licensor and its affiliates, except as required for reproducing
# the content of the NOTICE file.
#
# You may obtain a copy of the License at
# http://www.apache.org/licenses/LICENSE-2.0

# Adapted to C++ by Miles Macklin 2016

*/

#include "maths.h"

const float PI = 3.14159265358979323846;

float sqr(float x) { return x*x; }

float SchlickFresnel(float u)
{
    float m = Clamp(1-u, 0.0f, 1.0f);
    float m2 = m*m;
    return m2*m2*m; // pow(m,5)
}

float GTR1(float NDotH, float a)
{
    if (a >= 1) return 1/PI;
    float a2 = a*a;
    float t = 1 + (a2-1)*NDotH*NDotH;
    return (a2-1) / (PI*log(a2)*t);
}

float GTR2(float NDotH, float a)
{
    float a2 = a*a;
    float t = 1 + (a2-1)*NDotH*NDotH;
    return a2 / (PI * t*t);
}

float GTR2_aniso(float NDotH, float HDotX, float HDotY, float ax, float ay)
{
    return 1 / ( PI * ax*ay * sqr( sqr(HDotX/ax) + sqr(HDotY/ay) + NDotH*NDotH ));
}

float smithG_GGX(float NDotv, float alphaG)
{
    float a = alphaG*alphaG;
    float b = NDotv*NDotv;
    return 1/(NDotv + sqrt(a + b - a*b));
}

Vec3 mon2lin(Vec3 x)
{
    return Vec3(pow(x[0], 2.2), pow(x[1], 2.2), pow(x[2], 2.2));
}

inline Color BRDF(const Material& mat, const Vec3& P, const Vec3& N, const Vec3& V, const Vec3& L)
{
    float NDotL = Dot(N,L);
    float NDotV = Dot(N,V);
    if (NDotL < 0 || NDotV < 0) return Color(0);

    Vec3 H = Normalize(L+V);
    float NDotH = Dot(N,H);
    float LDotH = Dot(L,H);

    Vec3 Cdlin = mon2lin(Vec3(mat.color));
    float Cdlum = .3*Cdlin[0] + .6*Cdlin[1]  + .1*Cdlin[2]; // luminance approx.

    Vec3 Ctint = Cdlum > 0.0f ? Cdlin/Cdlum : Vec3(1); // normalize lum. to isolate hue+sat
    Vec3 Cspec0 = Lerp(mat.specular*.08*Lerp(Vec3(1), Ctint, mat.specularTint), Cdlin, mat.metallic);
    Vec3 Csheen = Lerp(Vec3(1), Ctint, mat.sheenTint);

    // Diffuse fresnel - go from 1 at normal incidence to .5 at grazing
    // and mix in diffuse retro-reflection based on roughness
    float FL = SchlickFresnel(NDotL), FV = SchlickFresnel(NDotV);
    float Fd90 = 0.5 + 2.0f * LDotH*LDotH * mat.roughness;
    float Fd = Lerp(1.0f, Fd90, FL) * Lerp(1.0f, Fd90, FV);

    // Based on Hanrahan-Krueger brdf approximation of isotropic bssrdf
    // 1.25 scale is used to (roughly) preserve albedo
    // Fss90 used to "flatten" retroreflection based on roughness
    float Fss90 = LDotH*LDotH*mat.roughness;
    float Fss = Lerp(1.0f, Fss90, FL) * Lerp(1.0f, Fss90, FV);
    float ss = 1.25 * (Fss * (1.0f / (NDotL + NDotV) - .5) + .5);

    // specular
    //float aspect = sqrt(1-mat.anisotropic*.9);
    //float ax = Max(.001f, sqr(mat.roughness)/aspect);
    //float ay = Max(.001f, sqr(mat.roughness)*aspect);
    //float Ds = GTR2_aniso(NDotH, Dot(H, X), Dot(H, Y), ax, ay);
    float a = Max(0.001f, sqr(mat.roughness));
    float Ds = GTR2(NDotH, a);
    float FH = SchlickFresnel(LDotH);
    Vec3 Fs = Lerp(Cspec0, Vec3(1), FH);
    float roughg = sqr(mat.roughness*.5+.5);
    float Gs = smithG_GGX(NDotL, roughg) * smithG_GGX(NDotV, roughg);

    // sheen
    Vec3 Fsheen = FH * mat.sheen * Csheen;

    // clearcoat (ior = 1.5 -> F0 = 0.04)
    float Dr = GTR1(NDotH, Lerp(.1,.001, mat.clearcoatGloss));
    float Fr = Lerp(.04f, 1.0f, FH);
    float Gr = smithG_GGX(NDotL, .25) * smithG_GGX(NDotV, .25);

    Vec3 out = ((1/PI) * Lerp(Fd, ss, mat.subsurface)*Cdlin + Fsheen)
        * (1-mat.metallic)
        + Gs*Fs*Ds + .25*mat.clearcoat*Gr*Fr*Dr;

    return Vec4(out, 0.0f);
}