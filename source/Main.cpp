#include "plugin.h"
#include "CCamFollowVehicle.h"
#include "CCamera.h"
#include "CHud.h"

using namespace plugin;

class CenteredVehicleCamIV {
public:
    static inline injector::hook_back<void(__fastcall*)(rage::Matrix44*, void*, void*)> hbCopyMatFront;
    static void __fastcall CopyMatFront(rage::Matrix44* mat, void*, void* arg2) {
        hbCopyMatFront.fun(mat, 0, arg2);

        auto playa = FindPlayerPed(0);
        auto vehicle = FindPlayerVehicle(0);
        if (!vehicle)
            vehicle = CPed::GetVehiclePedWouldEnter(playa, playa->m_pMatrix->up, 0);

        if (vehicle && vehicle->m_nVehicleType != VEHICLETYPE_BIKE) {
            mat->pos += mat->right * 0.5f;
            mat->pos += mat->at * 0.25f;
            mat->pos -= mat->up * 0.25f;
        }
    }

    static inline injector::hook_back<void(__fastcall*)(rage::Matrix44*, void*, void*)> hbCopyMatBehind;
    static void __fastcall CopyMatBehind(rage::Matrix44* mat, void*, void* arg2) {
        hbCopyMatBehind.fun(mat, 0, arg2);

        auto playa = FindPlayerPed(0);
        auto vehicle = FindPlayerVehicle(0);
        if (!vehicle)
            vehicle = CPed::GetVehiclePedWouldEnter(playa, playa->m_pMatrix->up, 0);

        if (vehicle && vehicle->m_nVehicleType != VEHICLETYPE_BIKE) {
            mat->pos -= mat->right * 0.5f;
        }
    }

    CenteredVehicleCamIV() {
        static auto pattern = plugin::pattern::get("E8 ? ? ? ? 80 A7 ? ? ? ? ? 80 A7 ? ? ? ? ? 80 7C 24");
        hbCopyMatFront.fun = injector::GetBranchDestination(pattern).get();
        injector::MakeCALL(pattern, CopyMatFront);

        pattern = plugin::pattern::get("E8 ? ? ? ? 5F B0 01 5E 8B E5 5D C2 14 00");
        hbCopyMatBehind.fun = injector::GetBranchDestination(pattern).get();
        injector::MakeCALL(pattern, CopyMatBehind);
    }
} centeredVehicleCamIV;
