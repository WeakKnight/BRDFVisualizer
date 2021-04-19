#pragma once
#include "Falcor.h"
using namespace Falcor;

namespace Utils
{
    inline bool CameraMoved(Scene::SharedPtr scene)
    {
        bool cameraMoved = false;
        auto sceneUpdates = scene->getUpdates();
        
        if (is_set(sceneUpdates, Scene::UpdateFlags::CameraPropertiesChanged))
        {
            auto excluded = Camera::Changes::Jitter | Camera::Changes::History;
            auto cameraChanges = scene->getCamera()->getChanges();
            if ((cameraChanges & ~excluded) != Camera::Changes::None)
            {
                cameraMoved = true;
            }
        }

        if (is_set(sceneUpdates, Scene::UpdateFlags::CameraSwitched))
        {
            cameraMoved = true;
        }

        return cameraMoved;
    }

    inline bool MaterialChanged(Scene::SharedPtr scene)
    {
        bool materialChanged = false;
        auto sceneUpdates = scene->getUpdates();
        if (is_set(sceneUpdates, Scene::UpdateFlags::MaterialsChanged))
        {
            materialChanged = true;
        }
        return materialChanged;
    }


    inline std::string ReadFile(const std::string& filepath)
    {
        std::ifstream ifs(filepath.c_str());
        std::string content((std::istreambuf_iterator<char>(ifs)),
            (std::istreambuf_iterator<char>()));
        ifs.close();
        return content;
    }

    inline float halton(uint32_t index, uint32_t base)
    {
        // Reversing digit order in the given base in floating point.
        float result = 0.0f;
        float factor = 1.0f;

        for (; index > 0; index /= base)
        {
            factor /= base;
            result += factor * (index % base);
        }

        return result;
    }

    inline float2 plastic(uint32_t index)
    {
        static float p1 = 0.7548776662466927f;
        static float p2 = 0.5698402909980532f;
        float2 result;
        result.x = fmodf(p1 * float(index),1);
        result.y = fmodf(p2 * float(index),1);
        return result;
    }
   

    inline int2 RandomPointInCircle(float radius, float u, float v)
    {
        // generate a random value between 0 to Radius as the value of Cumulative Distribution Function
        float S = u;
        // S = r2 / R2, choose r based on F
        float r = sqrt(S) * radius;
        float theta = v * float(M_PI) * 2.0f;

        float x = r * cos(theta);
        float y = r * sin(theta);

        return int2(x, y);
    }

    inline float2 RandomPointInCircleFloat(float radius, float u, float v)
    {
        // generate a random value between 0 to Radius as the value of Cumulative Distribution Function
        float S = u;
        // S = r2 / R2, choose r based on F
        float r = sqrt(S) * radius;
        float theta = v * float(M_PI) * 2.0f;

        float x = r * cos(theta);
        float y = r * sin(theta);

        return float2(x, y);
    }
    inline float2 RandomPointInCircleFloatWithOffset(float radius, float u, float v,float offset)
    {
        // generate a random value between 0 to Radius as the value of Cumulative Distribution Function
        float S = u;
        // S = r2 / R2, choose r based on F
        float r = sqrt(S) * radius;
        float theta = (v+ offset) * float(M_PI) * 2.0f;

        float x = r * cos(theta);
        float y = r * sin(theta);

        return float2(x, y);
    }

    inline void SetReverseZMatrix(Camera::SharedPtr camera)
    {
        float aspectRatio = 1.0f / camera->getAspectRatio();
        float focalLength = camera->getFocalLength();
        float frameHeight = camera->getFrameHeight();

        float nearPlane = camera->getNearPlane();
        float farPlane = camera->getFarPlane();

        const float fovY = focalLength == 0.0f ? 0.0f : focalLengthToFovY(focalLength, frameHeight);
        float Y = 1.0f / std::tanf(fovY * 0.5f);
        float X = Y * aspectRatio;
        float Q1 = nearPlane / (farPlane - nearPlane);
        float Q2 = Q1 * farPlane;

        float4x4 result(0.0f);
        result[0][0] = X;
        result[1][1] = Y;
        result[2][2] = Q1;
        result[2][3] = -1.0f;
        result[3][2] = Q2;

        camera->setProjectionMatrix(result);
    }

    inline float Luminance(const float3& col)
    {
        return glm::dot(col, float3(0.299f, 0.587f, 0.114f));
    }

    // TODO: Unify this code with the host-side functions in ColorUtils.h when #175 is solved.
/** Transforms an RGB color in Rec.709 to CIE XYZ.
*/
    inline float3 RGBtoXYZ_Rec709(float3 c)
    {
        static const float3x3 M1 =
        {
            0.4123907992659595, 0.3575843393838780, 0.1804807884018343,
            0.2126390058715104, 0.7151686787677559, 0.0721923153607337,
            0.0193308187155918, 0.1191947797946259, 0.9505321522496608
        };
        return M1 * c;
    }

    /** Transforms an XYZ color to RGB in Rec.709.
    */
    inline float3 XYZtoRGB_Rec709(float3 c)
    {
        static const float3x3 M2 =
        {
            3.240969941904522, -1.537383177570094, -0.4986107602930032,
            -0.9692436362808803, 1.875967501507721, 0.04155505740717569,
            0.05563007969699373, -0.2039769588889765, 1.056971514242878
        };
        return M2 * c;
    }

    inline uint encodeLogLuvHDR(float3 color)
    {
        // Convert RGB to XYZ.
        float3 XYZ = RGBtoXYZ_Rec709(color);

        // Encode log2(Y) over the range [-20,20) in 14 bits (no sign bit).
        // TODO: Fast path that uses the bits from the fp32 representation directly.
        float logY = 409.6f * (log2(XYZ.y) + 20.f); // -inf if Y==0
        uint Le = (uint)clamp(logY, 0.f, 16383.f);

        // Early out if zero luminance to avoid NaN in chroma computation.
        // Note Le==0 if Y < 9.55e-7. We'll decode that as exactly zero.
        if (Le == 0) return 0;

        // Compute chroma (u,v) values by:
        //  x = X / (X + Y + Z)
        //  y = Y / (X + Y + Z)
        //  u = 4x / (-2x + 12y + 3)
        //  v = 9y / (-2x + 12y + 3)
        //
        // These expressions can be refactored to avoid a division by:
        //  u = 4X / (-2X + 12Y + 3(X + Y + Z))
        //  v = 9Y / (-2X + 12Y + 3(X + Y + Z))
        //
        float invDenom = 1.f / (-2.f * XYZ.x + 12.f * XYZ.y + 3.f * (XYZ.x + XYZ.y + XYZ.z));
        float2 uv = float2(4.f, 9.f) * XYZ.xy * invDenom;

        // Encode chroma (u,v) in 9 bits each.
        // The gamut of perceivable uv values is roughly [0,0.62], so scale by 820 to get 9-bit values.
        uint2 uve = (uint2)clamp(820.f * uv, 0.f, 511.f);

        return (Le << 18) | (uve.x << 9) | uve.y;
    }

    /** Decode an RGB color stored in a 32-bit LogLuv HDR format.
        See encodeLogLuvHDR() for details.
    */
    inline float3 decodeLogLuvHDR(uint packedColor)
    {
        // Decode luminance Y from encoded log-luminance.
        uint Le = packedColor >> 18;
        if (Le == 0) return float3(0.f);

        float logY = (float(Le) + 0.5f) / 409.6f - 20.f;
        float Y = pow(2.f, logY);

        // Decode normalized chromaticity xy from encoded chroma (u,v).
        //
        //  x = 9u / (6u - 16v + 12)
        //  y = 4v / (6u - 16v + 12)
        //
        uint2 uve = uint2((packedColor >> 9) & 0x1ff, packedColor & 0x1ff);
        float2 uv = (float2(uve) + 0.5f) / 820.f;

        float invDenom = 1.f / (6.f * uv.x - 16.f * uv.y + 12.f);
        float2 xy = float2(9.f, 4.f) * uv * invDenom;

        // Convert chromaticity to XYZ and back to RGB.
        //  X = Y / y * x
        //  Z = Y / y * (1 - x - y)
        //
        float s = Y / xy.y;
        float3 XYZ = { s * xy.x, Y, s * (1.f - xy.x - xy.y) };

        // Convert back to RGB and clamp to avoid out-of-gamut colors.
        return max(XYZtoRGB_Rec709(XYZ), 0.f);
    }

    inline float decodeLogLuvHDRLuminance(uint packedColor)
    {
        // Decode luminance Y from encoded log-luminance.
        uint Le = packedColor >> 18;
        if (Le == 0) return 0.f;

        float logY = (float(Le) + 0.5f) / 409.6f - 20.f;
        float Y = pow(2.f, logY);

        return Y;
    }

    inline uint packUInt2x16(uint2 v)
    {
        return ((v.y) << 16) | v.x;
    }

    inline uint2 unpackUInt2x16(uint packed)
    {
        return uint2(packed & 0xffff, packed >> 16);
    }
};
