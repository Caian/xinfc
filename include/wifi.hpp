/*
 * Copyright (C) 2024 Caian Benedicto <caianbene@gmail.com>
 *
 * This file is part of xinfc.
 *
 * xinfc is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * xinfc is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with xinfc.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

namespace xinfc {

struct wifi_lenghts
{
    static const unsigned int ssid_min = 2;
    static const unsigned int ssid_max = 28;
    static const unsigned int pass_min = 8;
    static const unsigned int pass_max = 63;
};

enum wifi_crypt
{
    wifi_crypt_none     = 0x01,
    wifi_crypt_wep      = 0x02,
    wifi_crypt_tkip     = 0x04,
    wifi_crypt_aes      = 0x08,
    wifi_crypt_tkip_aes = wifi_crypt_tkip | wifi_crypt_aes,
};

enum wifi_auth
{
    wifi_auth_open              = 0x01,
    wifi_auth_wpa_personal      = 0x02,
    wifi_auth_shared            = 0x04,
    wifi_auth_wpa_enterprise    = 0x08,
    wifi_auth_wpa2_enterprise   = 0x10,
    wifi_auth_wpa2_personal     = 0x20,
    wifi_auth_wpa_wpa2_personal = wifi_auth_wpa_personal | wifi_auth_wpa2_personal,
};

struct wifi_str
{
    // From https://openwrt.org/docs/guide-user/network/wifi/basic#encryption_modes
    static const char* none()                // no authentication none
    {
        return "none";
    }

    static const char* sae()                 // WPA3 Personal (SAE) CCMP
    {
        return "sae";
    }

    static const char* sae_mixed()           // WPA2/WPA3 Personal (PSK/SAE) mixed mode CCMP
    {
        return "sae-mixed";
    }

    static const char* psk2_tkip_ccmp()      // WPA2 Personal (PSK) TKIP, CCMP
    {
        return "psk2+tkip+ccmp";
    }

    static const char* psk2_tkip_aes()       // WPA2 Personal (PSK) TKIP, CCMP
    {
        return "psk2+tkip+aes";
    }

    static const char* psk2_tkip()           // WPA2 Personal (PSK) TKIP
    {
        return "psk2+tkip";
    }

    static const char* psk2_ccmp()           // WPA2 Personal (PSK) CCMP
    {
        return "psk2+ccmp";
    }

    static const char* psk2_aes()            // WPA2 Personal (PSK) CCMP
    {
        return "psk2+aes";
    }

    static const char* psk2()                // WPA2 Personal (PSK) CCMP
    {
        return "psk2";
    }

    static const char* psk_tkip_ccmp()       // WPA Personal (PSK) TKIP, CCMP
    {
        return "psk+tkip+ccmp";
    }

    static const char* psk_tkip_aes()        // WPA Personal (PSK) TKIP, CCMP
    {
        return "psk+tkip+aes";
    }

    static const char* psk_tkip()            // WPA Personal (PSK) TKIP
    {
        return "psk+tkip";
    }

    static const char* psk_ccmp()            // WPA Personal (PSK) CCMP
    {
        return "psk+ccmp";
    }

    static const char* psk_aes()             // WPA Personal (PSK) CCMP
    {
        return "psk+aes";
    }

    static const char* psk()                 // WPA Personal (PSK) CCMP
    {
        return "psk";
    }

    static const char* psk_mixed_tkip_ccmp() // WPA/WPA2 Personal (PSK) mixed mode TKIP, CCMP
    {
        return "psk-mixed+tkip+ccmp";
    }

    static const char* psk_mixed_tkip_aes()  // WPA/WPA2 Personal (PSK) mixed mode TKIP, CCMP
    {
        return "psk-mixed+tkip+aes";
    }

    static const char* psk_mixed_tkip()      // WPA/WPA2 Personal (PSK) mixed mode TKIP
    {
        return "psk-mixed+tkip";
    }

    static const char* psk_mixed_ccmp()      // WPA/WPA2 Personal (PSK) mixed mode CCMP
    {
        return "psk-mixed+ccmp";
    }

    static const char* psk_mixed_aes()       // WPA/WPA2 Personal (PSK) mixed mode CCMP
    {
        return "psk-mixed+aes";
    }

    static const char* psk_mixed()           // WPA/WPA2 Personal (PSK) mixed mode CCMP
    {
        return "psk-mixed";
    }

    static const char* wep()                 // defaults to "open system" authentication aka wep+open
    {
        return "wep";
    }

    static const char* wep_open()            // "open system" authentication
    {
        return "wep+open";
    }

    static const char* wep_shared()          // "shared key" authentication
    {
        return "wep+shared";
    }

    static const char* wpa3()                // WPA3 Enterprise CCMP
    {
        return "wpa3";
    }

    static const char* wpa3_mixed()          // WPA3/WPA2 Enterprise CCMP
    {
        return "wpa3-mixed";
    }

    static const char* wpa2_tkip_ccmp()      // WPA2 Enterprise TKIP, CCMP
    {
        return "wpa2+tkip+ccmp";
    }

    static const char* wpa2_tkip_aes()       // WPA2 Enterprise TKIP, CCMP
    {
        return "wpa2+tkip+aes";
    }

    static const char* wpa2_ccmp()           // WPA2 Enterprise CCMP
    {
        return "wpa2+ccmp";
    }

    static const char* wpa2_aes()            // WPA2 Enterprise CCMP
    {
        return "wpa2+aes";
    }

    static const char* wpa2()                // WPA2 Enterprise CCMP
    {
        return "wpa2";
    }

    static const char* wpa2_tkip()           // WPA2 Enterprise TKIP
    {
        return "wpa2+tkip";
    }

    static const char* wpa_tkip_ccmp()       // WPA Enterprise TKIP, CCMP
    {
        return "wpa+tkip+ccmp";
    }

    static const char* wpa_tkip_aes()        // WPA Enterprise TKIP, AES
    {
        return "wpa+tkip+aes";
    }

    static const char* wpa_ccmp()            // WPA Enterprise CCMP
    {
        return "wpa+ccmp";
    }

    static const char* wpa_aes()             // WPA Enterprise CCMP
    {
        return "wpa+aes";
    }

    static const char* wpa_tkip()            // WPA Enterprise TKIP
    {
        return "wpa+tkip";
    }

    static const char* wpa()                 // WPA Enterprise CCMP
    {
        return "wpa";
    }

    static const char* wpa_mixed_tkip_ccmp() // WPA/WPA2 Enterprise mixed mode TKIP, CCMP
    {
        return "wpa-mixed+tkip+ccmp";
    }

    static const char* wpa_mixed_tkip_aes()  // WPA/WPA2 Enterprise mixed mode TKIP, CCMP
    {
        return "wpa-mixed+tkip+aes";
    }

    static const char* wpa_mixed_tkip()      // WPA/WPA2 Enterprise mixed mode TKIP
    {
        return "wpa-mixed+tkip";
    }

    static const char* wpa_mixed_ccmp()      // WPA/WPA2 Enterprise mixed mode CCMP
    {
        return "wpa-mixed+ccmp";
    }

    static const char* wpa_mixed_aes()       // WPA/WPA2 Enterprise mixed mode CCMP
    {
        return "wpa-mixed+aes";
    }

    static const char* wpa_mixed()           // WPA/WPA2 Enterprise mixed mode CCMP
    {
        return "wpa-mixed";
    }

    static const char* owe()                 // Opportunistic Wireless Encryption (OWE) CCMP
    {
        return "owe";
    }
};

}
