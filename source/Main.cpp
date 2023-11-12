#include "plugin.h"
#include "debugmenu_public.h"

DebugMenuAPI gDebugMenuAPI;

using namespace plugin;

class CenteredVehicleCamIV {
public:
    static inline float fRightMult = 0.0f;
    static inline float fForwardMult = 0.0f;
    static inline float fUpMult = 0.0f;
    static inline bool bCamShake = true;
    static inline float fCamShakeForce = 0.0f;
    static inline uint32_t nCamShakeStart = 0;
    static inline float fShakeIntensity = 0.0f;

    static bool IsVehicleTypeOffCenter(CVehicle const* veh) {
        uint32_t type = veh->m_nVehicleType;
        return type == VEHICLETYPE_AUTOMOBILE || type == VEHICLETYPE_PLANE || type == VEHICLETYPE_HELI;
    }

    static void ShakeCam(float strength) {
        float f = fCamShakeForce - (CTimer::GetTimeInMilliseconds() - nCamShakeStart) / 1000.0f;
        if (plugin::Clamp(f, 0.0f, 2.0f) < strength) {
            nCamShakeStart = CTimer::GetTimeInMilliseconds();
            fCamShakeForce = strength;
        }
    }

    static void PerformShakeAtPos(rage::Vector4* pos) {
        float shakeStrength = fCamShakeForce - 0.28f * (CTimer::GetTimeInMilliseconds() - nCamShakeStart) / 1000.0f;
        shakeStrength = plugin::Clamp(shakeStrength, 0.0f, 2.0f);
        int shakeRand = plugin::Random();
        float shakeOffset = shakeStrength * 0.1f;

        pos->x += shakeOffset * ((shakeRand & 0xF) - 7);
        pos->y += shakeOffset * (((shakeRand & 0xF0) >> 4) - 7);
        pos->z += shakeOffset * (((shakeRand & 0xF00) >> 8) - 7);
    }

    static inline injector::hook_back<void(__fastcall*)(rage::Matrix44*, void*, void*)> hbCopyMatFront;
    static void __fastcall CopyMatFront(rage::Matrix44* mat, void*, void* arg2) {
        hbCopyMatFront.fun(mat, 0, arg2);

        auto playa = FindPlayerPed(0);
        auto vehicle = FindPlayerVehicle(0);
        if (!vehicle)
            vehicle = CPed::GetVehiclePedWouldEnter(playa, playa->m_pMatrix->up, 0);

        if (vehicle && IsVehicleTypeOffCenter(vehicle)) {
            mat->pos += mat->right * fRightMult;
            mat->pos += mat->at * fUpMult;
            mat->pos -= mat->up * fForwardMult;
        }

        if (bCamShake)
            PerformShakeAtPos(&mat->pos);   
    }

    static inline injector::hook_back<void(__fastcall*)(rage::Matrix44*, void*, void*)> hbCopyMatBehind;
    static void __fastcall CopyMatBehind(rage::Matrix44* mat, void*, void* arg2) {
        hbCopyMatBehind.fun(mat, 0, arg2);

        auto playa = FindPlayerPed(0);
        auto vehicle = FindPlayerVehicle(0);
        if (!vehicle)
            vehicle = CPed::GetVehiclePedWouldEnter(playa, playa->m_pMatrix->up, 0);

        if (vehicle && IsVehicleTypeOffCenter(vehicle)) {
            mat->pos -= mat->right * fRightMult;
        }

        if (bCamShake)
            PerformShakeAtPos(&mat->pos);
    }

    static void ReadSettings() {
        config_file config(true, false);
        fRightMult = config["fRightMult"].asFloat(0.5f);
        fForwardMult = config["fForwardMult"].asFloat(0.2f);
        fUpMult = config["fUpMult"].asFloat(0.2f);
        bCamShake = config["bCamShake"].asBool(true);
        fShakeIntensity = config["fShakeIntensity"].asFloat(0.035f);
    }

    static void SaveSettings() {
        config_file config(true, true);
        config << "; Centered Vehicle Cam for GTA IV The Complete Edition.";
        config << "; Values below are multipliers for each direction that will be used for the final transform.";
        config["fRightMult"] = fRightMult;
        config["fForwardMult"] = fForwardMult;
        config["fUpMult"] = fUpMult;
        config << "; Set this to enable VCS like cam shake at high speed and it's intensity.";
        config["bCamShake"] = bCamShake;
        config["fShakeIntensity"] = fShakeIntensity;

        config.setUseAlignment(false);
        config.setUsePrecision(true);
        config.save();
    }

    CenteredVehicleCamIV() {
        static auto pattern = plugin::GetGlobalAddress(plugin::pattern::Get("E8 ? ? ? ? 80 A7 ? ? ? ? ? 80 A7 ? ? ? ? ? 80 7C 24", 0));
        hbCopyMatFront.fun = injector::GetBranchDestination(pattern).get();
        injector::MakeCALL(pattern, CopyMatFront);

        pattern = plugin::GetGlobalAddress(plugin::pattern::Get("E8 ? ? ? ? 5F B0 01 5E 8B E5 5D C2 14 00", 0));
        hbCopyMatBehind.fun = injector::GetBranchDestination(pattern).get();
        injector::MakeCALL(pattern, CopyMatBehind);

        if (DebugMenuLoad()) {
            DebugMenuAddCmd("Centered Vehicle Cam", "Reload ini settings", ReadSettings);
            DebugMenuAddFloat32("Centered Vehicle Cam", "fRightMult", &fRightMult, nullptr, 0.1f, -10.0f, 10.0f);
            DebugMenuAddFloat32("Centered Vehicle Cam", "fForwardMult", &fForwardMult, nullptr, 0.1f, -10.0f, 10.0f);
            DebugMenuAddFloat32("Centered Vehicle Cam", "fUpMult", &fUpMult, nullptr, 0.1f, -10.0f, 10.0f);
            DebugMenuAddVarBool8("Centered Vehicle Cam", "bCamShake", &bCamShake, nullptr);
            DebugMenuAddFloat32("Centered Vehicle Cam", "fCamShakeIntensity", &fShakeIntensity, nullptr, 0.001f, 0.0f, 1.0f);
            DebugMenuAddCmd("Centered Vehicle Cam", "Save ini settings", SaveSettings);
        }

        plugin::Events::initGameEvent += [] {
            ReadSettings();
        };

        plugin::Events::gameProcessEvent += []() {
            if (!bCamShake)
                return;

            auto vehicle = FindPlayerVehicle(0);
            if (vehicle) {
                rage::Vector3 velocity;
                vehicle->GetVelocity(&velocity);

                float speed = velocity.Magnitude() / 30.0f;
                if (speed > 0.75f)
                    ShakeCam(speed * fShakeIntensity);
            }
        };
    }
} centeredVehicleCamIV;
