// SAUDIO.CPP
// Copyright (c) A.Sobolev 2018
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

#if _MSC_VER >= 1600
#include <mmdeviceapi.h>
#include <endpointvolume.h> 

int SLAPI SGetAudioVolume(int decibels, double * pVolume)
{
	int    ok = 0;
	float current_volume = 0.0f;
	CoInitialize(NULL);
	IMMDeviceEnumerator * p_device_enumerator = NULL;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (void **)&p_device_enumerator);
	if(SUCCEEDED(hr) && p_device_enumerator) {
		IMMDevice * p_default_device = 0;
		hr = p_device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &p_default_device);
		p_device_enumerator->Release();
		p_device_enumerator = NULL;
		if(SUCCEEDED(hr) && p_default_device) {
			IAudioEndpointVolume * p_endpoint_volume = NULL;
			hr = p_default_device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (void **)&p_endpoint_volume);
			p_default_device->Release();
			p_default_device = NULL; 
			if(SUCCEEDED(hr) && p_endpoint_volume) {
				if(decibels) {
					p_endpoint_volume->GetMasterVolumeLevel(&current_volume);
					//printf("Current volume in dB is: %f\n", currentVolume);
				}
				else {
					hr = p_endpoint_volume->GetMasterVolumeLevelScalar(&current_volume);
					//printf("Current volume as a scalar is: %f\n", currentVolume);
				}
				p_endpoint_volume->Release();
				ok = 1;
			}
		}
	}
	CoUninitialize();
	ASSIGN_PTR(pVolume, current_volume);
	return ok;
}
//
// if(decibels) newVolume 
//
int SLAPI SSetAudioVolume(int decibels, double volume)
{
	int    ok = 1;
	CoInitialize(NULL);
	IMMDeviceEnumerator * p_device_enumerator = NULL;
	HRESULT hr = CoCreateInstance(__uuidof(MMDeviceEnumerator), NULL, CLSCTX_INPROC_SERVER, __uuidof(IMMDeviceEnumerator), (LPVOID *)&p_device_enumerator);
	if(SUCCEEDED(hr) && p_device_enumerator) {
		IMMDevice * p_default_device = 0;
		hr = p_device_enumerator->GetDefaultAudioEndpoint(eRender, eConsole, &p_default_device);
		p_device_enumerator->Release();
		p_device_enumerator = NULL;
		if(SUCCEEDED(hr) && p_default_device) {
			IAudioEndpointVolume * p_endpoint_volume = NULL;
			hr = p_default_device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER, NULL, (LPVOID *)&p_endpoint_volume);
			p_default_device->Release();
			p_default_device = NULL; 
			if(SUCCEEDED(hr) && p_endpoint_volume) {
				if(decibels)
					hr = p_endpoint_volume->SetMasterVolumeLevel((float)volume, NULL);
				else
					hr = p_endpoint_volume->SetMasterVolumeLevelScalar((float)volume, NULL); // newVolume [0.0-1.0]
				p_endpoint_volume->Release();
			}
		}
	}
	CoUninitialize();
	return ok;
}

#else

int SLAPI SGetAudioVolume(int decibels, double * pVolume)
{
	int    ok = 0;
	double real_volume = 0.0;
	WAVEOUTCAPSA woc;
	DWORD volume;
	if(waveOutGetDevCapsA(WAVE_MAPPER, &woc, sizeof(woc)) == MMSYSERR_NOERROR) {
		if(woc.dwSupport && WAVECAPS_VOLUME == WAVECAPS_VOLUME) {
			if(waveOutGetVolume((HWAVEOUT)WAVE_MAPPER, &volume) == MMSYSERR_NOERROR) {
				real_volume = ((double)MAX(LOWORD(volume), HIWORD(volume))) / (double)0xffffU;
				ok = 1;
			}
		}
	}
	ASSIGN_PTR(pVolume, real_volume);
	return ok;
}

int SLAPI SSetAudioVolume(int decibels, double volume)
{
	int    ok = 0;
	WAVEOUTCAPSA woc;
	if(waveOutGetDevCapsA(WAVE_MAPPER, &woc, sizeof(woc)) == MMSYSERR_NOERROR) {
		if(woc.dwSupport && WAVECAPS_VOLUME == WAVECAPS_VOLUME) {
			SETMAX(volume, 0.0);
			SETMIN(volume, 1.0);
			uint32 uv = (uint32)(volume * (double)0xffffU);
			if(waveOutSetVolume((HWAVEOUT)WAVE_MAPPER, (uv << 16) | (uv & 0xffffU)) == MMSYSERR_NOERROR) {
				ok = 1;
			}
		}
	}
	return ok;
}

#endif