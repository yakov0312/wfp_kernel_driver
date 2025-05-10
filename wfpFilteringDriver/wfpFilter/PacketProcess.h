#pragma once
#include "Resources.h"


//callout function for udp layer
void processPacketUDP(const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues, void* layerData, const void* classifyContext, const FWPS_FILTER3* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut);
//callout function for tcp layer
void processPacketTCP(const FWPS_INCOMING_VALUES0* inFixedValues, const FWPS_INCOMING_METADATA_VALUES0* inMetaValues, void* layerData, const void* classifyContext, const FWPS_FILTER3* filter, UINT64 flowContext, FWPS_CLASSIFY_OUT0* classifyOut);
NTSTATUS CalloutNotify(FWPS_CALLOUT_NOTIFY_TYPE notifyType, const GUID* filterKey, FWPS_FILTER* filter);